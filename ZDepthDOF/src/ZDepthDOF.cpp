#include "ffgl.h"
#include "gl_ext.h"
#include <cstring>
#include <cmath>
#include <cstdio>

// ── GL extension pointers (definitions) ───────────────────────────────────
PFNGLCREATESHADERPROC             gl_CreateShader            = nullptr;
PFNGLSHADERSOURCEPROC             gl_ShaderSource            = nullptr;
PFNGLCOMPILESHADERPROC            gl_CompileShader           = nullptr;
PFNGLGETSHADERIVPROC              gl_GetShaderiv             = nullptr;
PFNGLGETSHADERINFOLOGPROC         gl_GetShaderInfoLog        = nullptr;
PFNGLDELETESHADERPROC             gl_DeleteShader            = nullptr;
PFNGLCREATEPROGRAMPROC            gl_CreateProgram           = nullptr;
PFNGLATTACHSHADERPROC             gl_AttachShader            = nullptr;
PFNGLLINKPROGRAMPROC              gl_LinkProgram             = nullptr;
PFNGLGETPROGRAMIVPROC             gl_GetProgramiv            = nullptr;
PFNGLGETPROGRAMINFOLOGPROC        gl_GetProgramInfoLog       = nullptr;
PFNGLUSEPROGRAMPROC               gl_UseProgram              = nullptr;
PFNGLDELETEPROGRAMPROC            gl_DeleteProgram           = nullptr;
PFNGLGETUNIFORMLOCATIONPROC       gl_GetUniformLocation      = nullptr;
PFNGLUNIFORM1IPROC                gl_Uniform1i               = nullptr;
PFNGLUNIFORM1FPROC                gl_Uniform1f               = nullptr;
PFNGLUNIFORM2FPROC                gl_Uniform2f               = nullptr;
PFNGLGENVERTEXARRAYSPROC          gl_GenVertexArrays         = nullptr;
PFNGLBINDVERTEXARRAYPROC          gl_BindVertexArray         = nullptr;
PFNGLDELETEVERTEXARRAYSPROC       gl_DeleteVertexArrays      = nullptr;
PFNGLGENBUFFERSPROC               gl_GenBuffers              = nullptr;
PFNGLBINDBUFFERPROC               gl_BindBuffer              = nullptr;
PFNGLBUFFERDATAPROC               gl_BufferData              = nullptr;
PFNGLDELETEBUFFERSPROC            gl_DeleteBuffers           = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC      gl_VertexAttribPointer     = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC  gl_EnableVertexAttribArray = nullptr;
PFNGLACTIVETEXTUREPROC            gl_ActiveTexture           = nullptr;

// ── Shaders ────────────────────────────────────────────────────────────────
static const GLchar* s_vtx = R"GLSL(
#version 410 core
uniform vec2 uMaxUV;
layout(location = 0) in vec2 vPos;
layout(location = 1) in vec2 vUV;
out vec2 texCoord;
void main() {
    gl_Position = vec4(vPos, 0.0, 1.0);
    texCoord = vUV * uMaxUV;
}
)GLSL";

static const GLchar* s_frg = R"GLSL(
#version 410 core
uniform sampler2D uInputTex;
uniform vec2  uRenderSize;
uniform float uFocalY;
uniform float uFocalRange;
uniform float uMaxBlur;
uniform float uLumaWeight;
uniform float uFgBoost;
uniform float uQuality;
uniform float uShowDepth;
in  vec2 texCoord;
out vec4 fragColor;

void main() {
    vec2 uv  = texCoord;
    vec4 src = texture(uInputTex, uv);

    // Depth proxy: Y position (bottom=near, top=far) blended with luminance
    float luma  = dot(src.rgb, vec3(0.2126, 0.7152, 0.0722));
    float depth = mix(uv.y, luma, uLumaWeight);

    if (uShowDepth > 0.5) {
        fragColor = vec4(vec3(depth), 1.0);
        return;
    }

    // Circle of confusion
    float dist = abs(depth - uFocalY);
    float coc  = clamp((dist - uFocalRange) / max(uFocalRange, 0.001), 0.0, 1.0);
    if (depth < uFocalY) coc *= uFgBoost;   // near objects blur more

    float radius = coc * uMaxBlur;
    if (radius < 0.0005) { fragColor = src; return; }

    // Golden-angle spiral bokeh (8–32 samples)
    float aspect = uRenderSize.x / uRenderSize.y;
    vec4  acc    = vec4(0.0);
    int   n      = int(uQuality * 24.0) + 8;

    for (int i = 0; i < 32; i++) {
        if (i >= n) break;
        float fi    = float(i) + 0.5;
        float angle = fi * 2.399963229;
        float r     = sqrt(fi / float(n));
        vec2  off   = vec2(cos(angle) / aspect, sin(angle)) * r * radius;
        acc += texture(uInputTex, clamp(uv + off, 0.0, 1.0));
    }
    fragColor = acc / float(n);
}
)GLSL";

// ── Full-screen quad ───────────────────────────────────────────────────────
static const float  s_quad[] = { -1,-1,0,0,  1,-1,1,0,  1,1,1,1,  -1,1,0,1 };
static const GLuint s_idx[]  = { 0,1,2, 0,2,3 };

// ── Parameter indices ──────────────────────────────────────────────────────
enum { P_FOCAL_Y=0, P_FOCAL_RANGE, P_MAX_BLUR, P_LUMA_WEIGHT,
       P_FG_BOOST, P_QUALITY, P_SHOW_DEPTH, NUM_PARAMS };

static const char*  s_names[]    = { "Focal Plane Y","Sharp Zone","Max Blur",
                                     "Luma to Depth","Near Boost","Quality","Show Depth Map" };
static const float  s_defaults[] = { 0.5f, 0.24f, 0.313f, 0.25f, 0.371f, 0.333f, 0.0f };
static const uint32_t s_types[]  = { FF_TYPE_STANDARD, FF_TYPE_STANDARD, FF_TYPE_STANDARD,
                                     FF_TYPE_STANDARD, FF_TYPE_STANDARD, FF_TYPE_STANDARD,
                                     FF_TYPE_BOOLEAN };

// ── Plugin instance ────────────────────────────────────────────────────────
struct ZDepthDOF {
    // GL resources
    GLuint prog = 0, vao = 0, vbo = 0, ibo = 0;
    GLint  uInputTex=-1, uMaxUV=-1, uRenderSize=-1;
    GLint  uFocalY=-1, uFocalRange=-1, uMaxBlur=-1;
    GLint  uLumaWeight=-1, uFgBoost=-1, uQuality=-1, uShowDepth=-1;
    int    viewW=1920, viewH=1080;

    // Parameters (raw 0-1 FFGL values)
    float p[NUM_PARAMS];

    ZDepthDOF() { memcpy(p, s_defaults, sizeof(p)); }

    // ── Compile one shader stage ───────────────────────────────────────────
    static GLuint CompileStage(GLenum type, const GLchar* src) {
        GLuint s = gl_CreateShader(type);
        gl_ShaderSource(s, 1, &src, nullptr);
        gl_CompileShader(s);
        GLint ok = 0;
        gl_GetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512]; gl_GetShaderInfoLog(s, 512, nullptr, log);
            OutputDebugStringA(log);
            gl_DeleteShader(s);
            return 0;
        }
        return s;
    }

    // ── InitGL ─────────────────────────────────────────────────────────────
    uint32_t InitGL(const FFGLViewportStruct* vp) {
        viewW = (int)vp->width;
        viewH = (int)vp->height;

        if (!GL_LoadExtensions()) return FF_FAIL;

        GLuint vs = CompileStage(GL_VERTEX_SHADER,   s_vtx);
        GLuint fs = CompileStage(GL_FRAGMENT_SHADER, s_frg);
        if (!vs || !fs) { gl_DeleteShader(vs); gl_DeleteShader(fs); return FF_FAIL; }

        prog = gl_CreateProgram();
        gl_AttachShader(prog, vs);
        gl_AttachShader(prog, fs);
        gl_LinkProgram(prog);
        gl_DeleteShader(vs);
        gl_DeleteShader(fs);

        GLint ok = 0;
        gl_GetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) { gl_DeleteProgram(prog); prog = 0; return FF_FAIL; }

        gl_UseProgram(prog);
        uInputTex   = gl_GetUniformLocation(prog, "uInputTex");
        uMaxUV      = gl_GetUniformLocation(prog, "uMaxUV");
        uRenderSize = gl_GetUniformLocation(prog, "uRenderSize");
        uFocalY     = gl_GetUniformLocation(prog, "uFocalY");
        uFocalRange = gl_GetUniformLocation(prog, "uFocalRange");
        uMaxBlur    = gl_GetUniformLocation(prog, "uMaxBlur");
        uLumaWeight = gl_GetUniformLocation(prog, "uLumaWeight");
        uFgBoost    = gl_GetUniformLocation(prog, "uFgBoost");
        uQuality    = gl_GetUniformLocation(prog, "uQuality");
        uShowDepth  = gl_GetUniformLocation(prog, "uShowDepth");
        gl_Uniform1i(uInputTex, 0);
        gl_UseProgram(0);

        // Full-screen quad VAO
        gl_GenVertexArrays(1, &vao);
        gl_BindVertexArray(vao);
        gl_GenBuffers(1, &vbo);
        gl_BindBuffer(GL_ARRAY_BUFFER, vbo);
        gl_BufferData(GL_ARRAY_BUFFER, sizeof(s_quad), s_quad, GL_STATIC_DRAW);
        gl_GenBuffers(1, &ibo);
        gl_BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        gl_BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_idx), s_idx, GL_STATIC_DRAW);
        gl_VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
        gl_EnableVertexAttribArray(0);
        gl_VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
        gl_EnableVertexAttribArray(1);
        gl_BindVertexArray(0);

        return FF_SUCCESS;
    }

    // ── DeInitGL ───────────────────────────────────────────────────────────
    void DeInitGL() {
        if (prog) { gl_DeleteProgram(prog); prog = 0; }
        if (vao)  { gl_DeleteVertexArrays(1, &vao); vao = 0; }
        if (vbo)  { gl_DeleteBuffers(1, &vbo); vbo = 0; }
        if (ibo)  { gl_DeleteBuffers(1, &ibo); ibo = 0; }
    }

    // ── ProcessOpenGL ──────────────────────────────────────────────────────
    uint32_t Process(ProcessOpenGLStruct* pGL) {
        if (!pGL || !pGL->inputTextures || !pGL->inputTextures[0]) return FF_FAIL;
        const auto* tex = pGL->inputTextures[0];

        // Map 0-1 params → physical ranges
        float focalRange = 0.005f + p[P_FOCAL_RANGE] * 0.495f;  // 0.005–0.5
        float maxBlur    = 0.001f + p[P_MAX_BLUR]    * 0.079f;  // 0.001–0.08
        float fgBoost    = 0.500f + p[P_FG_BOOST]    * 3.500f;  // 0.5–4.0

        gl_UseProgram(prog);
        gl_Uniform2f(uMaxUV,      tex->MaxS, tex->MaxT);
        gl_Uniform2f(uRenderSize, (float)viewW, (float)viewH);
        gl_Uniform1f(uFocalY,     p[P_FOCAL_Y]);
        gl_Uniform1f(uFocalRange, focalRange);
        gl_Uniform1f(uMaxBlur,    maxBlur);
        gl_Uniform1f(uLumaWeight, p[P_LUMA_WEIGHT]);
        gl_Uniform1f(uFgBoost,    fgBoost);
        gl_Uniform1f(uQuality,    p[P_QUALITY]);
        gl_Uniform1f(uShowDepth,  p[P_SHOW_DEPTH]);

        gl_ActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->Handle);

        gl_BindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        gl_BindVertexArray(0);

        gl_UseProgram(0);
        return FF_SUCCESS;
    }
};

// ── Instance table (up to 16 simultaneous instances) ──────────────────────
static ZDepthDOF* g_inst[16] = {};
static uint32_t   g_nextSlot  = 1;   // 0 = invalid handle

static ZDepthDOF* FindInst(uint32_t id) {
    if (id == 0 || id > 16) return nullptr;
    return g_inst[id - 1];
}

// ── Plugin info (static, returned by pointer) ──────────────────────────────
static PluginInfoStruct s_info = {
    2, 1,
    {'Z','D','P','F'},
    {'Z','-','D','e','p','t','h',' ','D','O','F',0,0,0,0,0},
    FF_EFFECT,
    0x00010000,
    "Z-Depth DOF: vertical+luma depth proxy, bokeh blur.",
    "custom",
    0, nullptr
};

// ── plugMain — the single DLL export ──────────────────────────────────────
// FFMixed lets us handle both 32-bit values and 64-bit pointers on x64.
static FFMixed MixUInt(uint32_t v) { FFMixed r; r.UIntValue    = v;  return r; }
static FFMixed MixPtr (void*    p) { FFMixed r; r.PointerValue = p;  return r; }

extern "C" __declspec(dllexport)
FFMixed __stdcall plugMain(uint32_t code, FFMixed input) {

    switch (code) {

    // ── Global queries (input is a plain uint32) ───────────────────────────
    case FF_GETINFO:           return MixPtr(&s_info);
    case FF_GETPARAMETERCOUNT: return MixUInt(NUM_PARAMS);

    case FF_GETPARAMETERNAME:
        return (input.UIntValue < NUM_PARAMS)
            ? MixPtr((void*)s_names[input.UIntValue]) : MixUInt(FF_FAIL);

    case FF_GETPARAMETERDEFAULT: {
        if (input.UIntValue >= NUM_PARAMS) return MixUInt(FF_FAIL);
        uint32_t r; memcpy(&r, &s_defaults[input.UIntValue], 4);
        return MixUInt(r);
    }
    case FF_GETPARAMETERTYPE:
        return (input.UIntValue < NUM_PARAMS)
            ? MixUInt(s_types[input.UIntValue]) : MixUInt(FF_FAIL);

    case FF_GETPLUGINCAPS:
        if (input.UIntValue == FF_CAP_PROCESSOPENGL)        return MixUInt(FF_SUPPORTED);
        if (input.UIntValue == FF_CAP_SETTIME)              return MixUInt(FF_NOTSUPPORTED);
        if (input.UIntValue == FF_CAP_MINIMUM_INPUT_FRAMES) return MixUInt(1);
        if (input.UIntValue == FF_CAP_MAXIMUM_INPUT_FRAMES) return MixUInt(1);
        return MixUInt(FF_UNSUPPORTED);

    case FF_INITIALISE:
    case FF_DEINITIALISE:
        return MixUInt(FF_SUCCESS);

    // ── Instance lifecycle ─────────────────────────────────────────────────
    case FF_INITIALISE_V2: {
        uint32_t slot = 0;
        for (uint32_t i = 0; i < 16; i++) { if (!g_inst[i]) { slot = i+1; break; } }
        if (!slot) return MixUInt(FF_FAIL);

        auto* inst = new ZDepthDOF();
        auto* vp   = static_cast<FFGLViewportStruct*>(input.PointerValue);
        if (inst->InitGL(vp) != FF_SUCCESS) { delete inst; return MixUInt(FF_FAIL); }

        g_inst[slot - 1] = inst;
        return MixUInt(slot);
    }

    case FF_DEINITIALISE_V2: {
        uint32_t id = input.UIntValue;
        ZDepthDOF* inst = FindInst(id);
        if (!inst) return MixUInt(FF_FAIL);
        inst->DeInitGL();
        delete inst;
        g_inst[id - 1] = nullptr;
        return MixUInt(FF_SUCCESS);
    }

    // ── Per-instance calls (input.PointerValue → InstanceHandledDataStruct) ─
    case FF_PROCESSOPENGL: {
        auto* ihd  = static_cast<InstanceHandledDataStruct*>(input.PointerValue);
        ZDepthDOF* inst = FindInst(ihd->instanceID);
        if (!inst) return MixUInt(FF_FAIL);
        return MixUInt(inst->Process(static_cast<ProcessOpenGLStruct*>(ihd->dataPointer)));
    }

    case FF_SETPARAMETER: {
        auto* ihd = static_cast<InstanceHandledDataStruct*>(input.PointerValue);
        ZDepthDOF* inst = FindInst(ihd->instanceID);
        if (!inst) return MixUInt(FF_FAIL);
        auto* sp = static_cast<SetParameterStruct*>(ihd->dataPointer);
        if (sp->ParameterNumber >= NUM_PARAMS) return MixUInt(FF_FAIL);
        memcpy(&inst->p[sp->ParameterNumber], &sp->NewParameterValue, 4);
        return MixUInt(FF_SUCCESS);
    }

    case FF_GETPARAMETER: {
        auto* ihd = static_cast<InstanceHandledDataStruct*>(input.PointerValue);
        ZDepthDOF* inst = FindInst(ihd->instanceID);
        if (!inst) return MixUInt(FF_FAIL);
        uint32_t idx = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ihd->dataPointer));
        if (idx >= NUM_PARAMS) return MixUInt(FF_FAIL);
        uint32_t r; memcpy(&r, &inst->p[idx], 4);
        return MixUInt(r);
    }

    default:
        return MixUInt(FF_UNSUPPORTED);
    }
}

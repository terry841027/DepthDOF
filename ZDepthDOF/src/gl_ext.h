#pragma once
// Minimal OpenGL 3.3+ extension loader — replaces GLEW.
// Uses wglGetProcAddress (context must be active when GL_LoadExtensions() is called).

#include <GL/gl.h>

typedef char      GLchar;
typedef ptrdiff_t GLsizeiptr;

// ── Extra constants not in windows gl.h ───────────────────────────────────
#define GL_VERTEX_SHADER             0x8B31
#define GL_FRAGMENT_SHADER           0x8B30
#define GL_COMPILE_STATUS            0x8B81
#define GL_LINK_STATUS               0x8B82
#define GL_INFO_LOG_LENGTH           0x8B84
#define GL_ARRAY_BUFFER              0x8892
#define GL_ELEMENT_ARRAY_BUFFER      0x8893
#define GL_STATIC_DRAW               0x88B4
#define GL_TEXTURE0                  0x84C0

// ── Function pointer types ─────────────────────────────────────────────────
typedef GLuint (APIENTRY* PFNGLCREATESHADERPROC)           (GLenum);
typedef void   (APIENTRY* PFNGLSHADERSOURCEPROC)           (GLuint, GLsizei, const GLchar**, const GLint*);
typedef void   (APIENTRY* PFNGLCOMPILESHADERPROC)          (GLuint);
typedef void   (APIENTRY* PFNGLGETSHADERIVPROC)            (GLuint, GLenum, GLint*);
typedef void   (APIENTRY* PFNGLGETSHADERINFOLOGPROC)       (GLuint, GLsizei, GLsizei*, GLchar*);
typedef void   (APIENTRY* PFNGLDELETESHADERPROC)           (GLuint);
typedef GLuint (APIENTRY* PFNGLCREATEPROGRAMPROC)          (void);
typedef void   (APIENTRY* PFNGLATTACHSHADERPROC)           (GLuint, GLuint);
typedef void   (APIENTRY* PFNGLLINKPROGRAMPROC)            (GLuint);
typedef void   (APIENTRY* PFNGLGETPROGRAMIVPROC)           (GLuint, GLenum, GLint*);
typedef void   (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC)      (GLuint, GLsizei, GLsizei*, GLchar*);
typedef void   (APIENTRY* PFNGLUSEPROGRAMPROC)             (GLuint);
typedef void   (APIENTRY* PFNGLDELETEPROGRAMPROC)          (GLuint);
typedef GLint  (APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)     (GLuint, const GLchar*);
typedef void   (APIENTRY* PFNGLUNIFORM1IPROC)              (GLint, GLint);
typedef void   (APIENTRY* PFNGLUNIFORM1FPROC)              (GLint, GLfloat);
typedef void   (APIENTRY* PFNGLUNIFORM2FPROC)              (GLint, GLfloat, GLfloat);
typedef void   (APIENTRY* PFNGLGENVERTEXARRAYSPROC)        (GLsizei, GLuint*);
typedef void   (APIENTRY* PFNGLBINDVERTEXARRAYPROC)        (GLuint);
typedef void   (APIENTRY* PFNGLDELETEVERTEXARRAYSPROC)     (GLsizei, const GLuint*);
typedef void   (APIENTRY* PFNGLGENBUFFERSPROC)             (GLsizei, GLuint*);
typedef void   (APIENTRY* PFNGLBINDBUFFERPROC)             (GLenum, GLuint);
typedef void   (APIENTRY* PFNGLBUFFERDATAPROC)             (GLenum, GLsizeiptr, const void*, GLenum);
typedef void   (APIENTRY* PFNGLDELETEBUFFERSPROC)          (GLsizei, const GLuint*);
typedef void   (APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC)    (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void   (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void   (APIENTRY* PFNGLACTIVETEXTUREPROC)          (GLenum);

// ── Global function pointer declarations ───────────────────────────────────
extern PFNGLCREATESHADERPROC             gl_CreateShader;
extern PFNGLSHADERSOURCEPROC             gl_ShaderSource;
extern PFNGLCOMPILESHADERPROC            gl_CompileShader;
extern PFNGLGETSHADERIVPROC              gl_GetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC         gl_GetShaderInfoLog;
extern PFNGLDELETESHADERPROC             gl_DeleteShader;
extern PFNGLCREATEPROGRAMPROC            gl_CreateProgram;
extern PFNGLATTACHSHADERPROC             gl_AttachShader;
extern PFNGLLINKPROGRAMPROC              gl_LinkProgram;
extern PFNGLGETPROGRAMIVPROC             gl_GetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC        gl_GetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC               gl_UseProgram;
extern PFNGLDELETEPROGRAMPROC            gl_DeleteProgram;
extern PFNGLGETUNIFORMLOCATIONPROC       gl_GetUniformLocation;
extern PFNGLUNIFORM1IPROC                gl_Uniform1i;
extern PFNGLUNIFORM1FPROC                gl_Uniform1f;
extern PFNGLUNIFORM2FPROC                gl_Uniform2f;
extern PFNGLGENVERTEXARRAYSPROC          gl_GenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC          gl_BindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC       gl_DeleteVertexArrays;
extern PFNGLGENBUFFERSPROC               gl_GenBuffers;
extern PFNGLBINDBUFFERPROC               gl_BindBuffer;
extern PFNGLBUFFERDATAPROC               gl_BufferData;
extern PFNGLDELETEBUFFERSPROC            gl_DeleteBuffers;
extern PFNGLVERTEXATTRIBPOINTERPROC      gl_VertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC  gl_EnableVertexAttribArray;
extern PFNGLACTIVETEXTUREPROC            gl_ActiveTexture;

// ── Loader ─────────────────────────────────────────────────────────────────
inline void* GL_GetProc(const char* name) {
    void* p = (void*)wglGetProcAddress(name);
    // wglGetProcAddress returns 1/2/3/-1 on some drivers to signal "not found"
    if (!p || p == (void*)1 || p == (void*)2 || p == (void*)3 || p == (void*)-1) {
        static HMODULE mod = GetModuleHandleA("opengl32.dll");
        p = mod ? (void*)GetProcAddress(mod, name) : nullptr;
    }
    return p;
}

inline bool GL_LoadExtensions() {
#define LOAD(var, name) \
    var = reinterpret_cast<decltype(var)>(GL_GetProc(#name)); \
    if (!var) return false;

    LOAD(gl_CreateShader,            glCreateShader)
    LOAD(gl_ShaderSource,            glShaderSource)
    LOAD(gl_CompileShader,           glCompileShader)
    LOAD(gl_GetShaderiv,             glGetShaderiv)
    LOAD(gl_GetShaderInfoLog,        glGetShaderInfoLog)
    LOAD(gl_DeleteShader,            glDeleteShader)
    LOAD(gl_CreateProgram,           glCreateProgram)
    LOAD(gl_AttachShader,            glAttachShader)
    LOAD(gl_LinkProgram,             glLinkProgram)
    LOAD(gl_GetProgramiv,            glGetProgramiv)
    LOAD(gl_GetProgramInfoLog,       glGetProgramInfoLog)
    LOAD(gl_UseProgram,              glUseProgram)
    LOAD(gl_DeleteProgram,           glDeleteProgram)
    LOAD(gl_GetUniformLocation,      glGetUniformLocation)
    LOAD(gl_Uniform1i,               glUniform1i)
    LOAD(gl_Uniform1f,               glUniform1f)
    LOAD(gl_Uniform2f,               glUniform2f)
    LOAD(gl_GenVertexArrays,         glGenVertexArrays)
    LOAD(gl_BindVertexArray,         glBindVertexArray)
    LOAD(gl_DeleteVertexArrays,      glDeleteVertexArrays)
    LOAD(gl_GenBuffers,              glGenBuffers)
    LOAD(gl_BindBuffer,              glBindBuffer)
    LOAD(gl_BufferData,              glBufferData)
    LOAD(gl_DeleteBuffers,           glDeleteBuffers)
    LOAD(gl_VertexAttribPointer,     glVertexAttribPointer)
    LOAD(gl_EnableVertexAttribArray, glEnableVertexAttribArray)
    LOAD(gl_ActiveTexture,           glActiveTexture)
#undef LOAD
    return true;
}

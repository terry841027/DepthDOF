#pragma once
// Minimal FFGL 2.x protocol — no external SDK required.
// Based on the public FFGL 2.1 spec (MIT-licensed protocol definitions).

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <cstdint>

// ── Return codes ───────────────────────────────────────────────────────────
#define FF_SUCCESS     ((uint32_t)0x00000000)
#define FF_FAIL        ((uint32_t)0xFFFFFFFF)
#define FF_UNSUPPORTED ((uint32_t)0xFFFFFFFE)
#define FF_SUPPORTED   1
#define FF_NOTSUPPORTED 0

// ── Plugin / parameter types ───────────────────────────────────────────────
#define FF_EFFECT           0
#define FF_SOURCE           1
#define FF_TYPE_STANDARD    0x10000000  // 0-1 float knob
#define FF_TYPE_BOOLEAN     0x01000000  // checkbox (0 or 1)

// ── Capability codes ───────────────────────────────────────────────────────
#define FF_CAP_PROCESSOPENGL  17
#define FF_CAP_SETTIME        18

// ── Function codes for plugMain ────────────────────────────────────────────
#define FF_GETINFO              0
#define FF_INITIALISE           1
#define FF_DEINITIALISE         2
#define FF_PROCESSFRAME         3
#define FF_GETPARAMETERCOUNT    4
#define FF_GETPARAMETERNAME     5
#define FF_GETPARAMETERDEFAULT  6
#define FF_GETPARAMETERDISPLAY  7
#define FF_SETPARAMETER         8
#define FF_GETPARAMETER         9
#define FF_GETPLUGINCAPS        10
#define FF_INITIALISE_V2        11
#define FF_DEINITIALISE_V2      12
#define FF_GETEXTENDEDINFO      13
#define FF_GETPARAMETERTYPE     14
#define FF_PROCESSOPENGL        15
#define FF_SETTIME              16

// ── Structs ────────────────────────────────────────────────────────────────
#pragma pack(push, 1)

struct PluginInfoStruct {
    uint32_t APIMajorVersion;
    uint32_t APIMinorVersion;
    uint8_t  UniqueID[4];
    uint8_t  PluginName[16];
    uint32_t PluginType;
    uint32_t PluginVersion;
    uint8_t  Description[64];
    uint8_t  About[64];
    uint32_t FreeFrameExtendedDataSize;
    void*    FreeFrameExtendedDataBlock;
};

struct FFGLViewportStruct {
    uint32_t x, y, width, height;
};

struct FFGLTextureStruct {
    GLuint  Handle;
    GLuint  HardwareWidth;
    GLuint  HardwareHeight;
    GLfloat MaxS;
    GLfloat MaxT;
};

struct ProcessOpenGLStruct {
    uint32_t            numInputTextures;
    FFGLTextureStruct** inputTextures;
};

struct SetParameterStruct {
    uint32_t ParameterNumber;
    uint32_t NewParameterValue; // float reinterpreted as uint32
};

struct InstanceHandledDataStruct {
    uint32_t instanceID;
    void*    dataPointer;
};

#pragma pack(pop)

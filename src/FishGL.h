#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <FishGLConfig.h>

#define FGL_PACKED __attribute__((packed))
#define FGL_DISCARD true
#define FGL_KEEP false

    typedef struct
    {
        float X;
        float Y;
    } FGL_PACKED fglVec2;

    typedef struct
    {
        float X;
        float Y;
        float Z;
    } FGL_PACKED fglVec3;

    typedef struct
    {
        float X;
        float Y;
        float Z;
        float W;
    } FGL_PACKED fglVec4;

    typedef struct
    {
        fglVec3 Row1;
        fglVec3 Row2;
        fglVec3 Row3;
    } FGL_PACKED fglMat3;

    typedef struct
    {
        fglVec4 Row1;
        fglVec4 Row2;
        fglVec4 Row3;
        fglVec4 Row4;
    } FGL_PACKED fglMat4;

    typedef enum
    {
        PixelOrder_Unknown,
        PixelOrder_RGBA,
        PixelOrder_ABGR,
        PixelOrder_RGB_565,
    } PixelOrder;

    // 16 bit 565 RGB, HI and LO bytes are swapped
    typedef union {
        struct
        {
            unsigned int B : 5;
            unsigned int G : 6;
            unsigned int R : 5;
        } FGL_PACKED;

        /*struct
        {
            unsigned int u8_HI : 8;
            unsigned int u8_LO : 8;
        } FGL_PACKED;*/

        unsigned short u16;
    } FGL_PACKED FglColor;

    /*typedef struct __attribute__((packed))
    {
        uint16_t u16;
    } FglColor;*/

    // 16 bit 565 RGB
    // typedef uint16_t FglColor;

    typedef struct
    {
        union {
            void *Memory;
            FglColor *Pixels;
        } FGL_PACKED;

        int32_t Width;
        int32_t Height;

        int32_t Length;
        int32_t PixelCount;
    } FglBuffer;

    typedef struct
    {
        fglVec3 A, B, C;
    } FglTriangle3;

    typedef struct
    {
        fglVec2 A, B, C;
    } FglTriangle2;

    typedef enum
    {
        FglTextureWrap_Clamp,
        FglTextureWrap_BorderColor,
        // FglTextureWrap_Repeat,
    } FglTextureWrap;

    typedef enum
    {
        FglBlendMode_None
    } FglBlendMode;

    typedef enum
    {
        FglShaderType_Vertex,
        FglShaderType_Fragment,
    } FglShaderType;

    typedef struct
    {
        void *VideoMemory;
        int32_t Width;
        int32_t Height;
        int32_t BPP;
        int32_t Stride;
        PixelOrder Order;

        fglMat4 MatProj;
        fglMat4 MatView;
        fglMat4 MatModel;

        FglBlendMode BlendMode;
        FglTextureWrap TextureWrap;

        FglColor BorderColor;
        FglBuffer Textures[FGL_MAX_TEXTURES];

        // FglVaryingIn VarIn[FGL_VARYING_COUNT];
        // FglVarying VarOut[FGL_VARYING_COUNT];

        int32_t VertNum;
        FglShaderType CurShader;

        void *VertexShader;
        void *FragmentShader;
    } FglState;

    typedef bool (*FglVertexFunc)(FglState *State, fglVec3 *Vert);
    typedef bool (*FglFragmentFunc)(FglState *State, fglVec2 UV, FglColor *OutColor);

    // Basics
    FglColor fglColor(uint8_t r, uint8_t g, uint8_t b);
    void fglColorToRGB(FglColor clr, uint8_t *r, uint8_t *g, uint8_t *b);

    // Initialization and state
    FGL_API void fglInit(void *VideoMemory, int32_t Width, int32_t Height, int32_t BPP, int32_t Stride,
                         PixelOrder Order);
    FGL_API FglState *fglGetState();
    FGL_API void fglSetState(FglState *State);

    void fglBeginFrame();
    void fglEndFrame();

    // Shaders
    FGL_API void fglBindShader(void *Shader, FglShaderType ShaderType);

    // Buffer functions and textures
    FGL_API FglBuffer fglCreateBuffer(void *Memory, int32_t Width, int32_t Height);
    FGL_API FglBuffer fglCreateBufferFromPng(void *PngInMemory, int32_t Len);
    FGL_API void fglDisplayToFramebuffer(FglBuffer *Buffer);
    FGL_API void fglClearBuffer(FglBuffer *Buffer, FglColor Clr);
    FGL_API void fglBindTexture(FglBuffer *TextureBuffer, int32_t Slot);

    // Drawing
    FGL_API void fglDrawLine(FglBuffer *Buffer, FglColor Color, int32_t X0, int32_t Y0, int32_t X1, int32_t Y1);
    FGL_API void fglDrawTriangle3(FglBuffer *Buffer, FglColor Color, FglTriangle3 *Tri);
    // FGL_API void fglFillTriangle3(FglBuffer *Buffer, FglColor Color, FglTriangle3 *Tri);
    FGL_API void fglRenderTriangle3(FglBuffer *Buffer, FglTriangle3 *Tri, FglTriangle2 *UV);
    void fglRenderTriangle3v(FglBuffer *Buffer, fglVec3 *vecs, fglVec2 *uvs, const size_t len, fglVec2 *Min, fglVec2 *Max);

    // Math
    fglVec2 fgl_Vec2(float X, float Y);
    fglVec3 fgl_Vec3(float X, float Y, float Z);
    fglVec3 fgl_Vec3_from_Vec2(fglVec2 V2, float Z);

    fglVec2 fgl_Vec2_Min(fglVec2 A, fglVec2 B);
    fglVec2 fgl_Vec2_Max(fglVec2 A, fglVec2 B);

    void fgl_Mul_4x4_4x1(fglMat4 *mat, const fglVec4 vec, fglVec4 *res);
    void fgl_Mul_3x3_3x1(fglMat3 *mat, const fglVec3 vec, fglVec3 *res);
    void fgl_Sub_3x1_3x1(const fglVec3 a, const fglVec3 b, fglVec3 *res);
    void fgl_Transpose_3x3(fglMat3 *mat);
    void fgl_Transpose_4x4(fglMat4 *mat);
    void fgl_Identity_3x3(fglMat3 *mat);
    void fgl_Identity_4x4(fglMat4 *mat);

    fglMat4 fgl_Make_Translate_4x4(fglVec3 tran);
    void fgl_Translate(fglMat4 *mat, fglVec3 tran);
    void fgl_Rotate(fglMat4 *mat, float ang, fglVec3 tran);
    void fgl_Scale(fglMat4 *mat, fglVec3 tran);

    fglVec3 fgl_Cross3(fglVec3 a, fglVec3 b);

#ifdef __cplusplus
}
#endif
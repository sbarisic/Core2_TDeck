#define STB_IMAGE_IMPLEMENTATION

#include <core2.h>

#include <FishGL.h>
#include <FishGLShader.h>

#include <byteswap.h>
#include <esp_dsp.h>

#include <Arduino.h>

// Core2
void core2_st7789_draw_fb_scanline(FglColor *colors, int y);
void core2_st7789_draw_fb_pixel(uint16_t color, int x, int y);

// MATH
// ===================================================================================================================================
// RENDERER
// ===============================================================================================================================

#define fgl_fminf(a, b) ((a) < (b) ? (a) : (b))
#define fgl_fmaxf(a, b) ((a) < (b) ? (b) : (a))

FGL_API FglState RenderState;
FglColor *Scanline;

static void _fglBoundingBox(FglTriangle3 *Tri, fglVec3 *Min, fglVec3 *Max)
{
    Min->X = fgl_fminf(fgl_fminf(Tri->A.X, Tri->B.X), Tri->C.X);
    Min->Y = fgl_fminf(fgl_fminf(Tri->A.Y, Tri->B.Y), Tri->C.Y);
    Min->Z = fgl_fminf(fgl_fminf(Tri->A.Z, Tri->B.Z), Tri->C.Z);

    Max->X = fgl_fmaxf(fgl_fmaxf(Tri->A.X, Tri->B.X), Tri->C.X);
    Max->Y = fgl_fmaxf(fgl_fmaxf(Tri->A.Y, Tri->B.Y), Tri->C.Y);
    Max->Z = fgl_fmaxf(fgl_fmaxf(Tri->A.Z, Tri->B.Z), Tri->C.Z);
}

static FUNC_CONST bool _fglBarycentric(fglVec3 A, fglVec3 B, fglVec3 C, float X, float Y, fglVec3 *Val)
{
    // fglVec3 A = Tri->A;
    // fglVec3 B = Tri->B;
    // fglVec3 C = Tri->C;

    fglVec3 U;
    fglVec3 a;
    fglVec3 b;

    a.X = (C.X) - (A.X);
    a.Y = (B.X) - (A.X);
    a.Z = (A.X) - (float)X;
    // a = fgl_Vec3(C.X - A.X, B.X - A.X, A.X - X);

    b.X = (C.Y) - (A.Y);
    b.Y = (B.Y) - (A.Y);
    b.Z = (A.Y) - (float)Y;
    // b = fgl_Vec3(C.Y - A.Y, B.Y - A.Y, A.Y - Y);

    // glm_vec_cross((float *)&a, (float *)&b, (float *)&U);
    U = fgl_Cross3(a, b);

    if (fabsf(U.Z) < 1)
        return false;

    Val->X = 1.0f - ((U.X + U.Y) / U.Z);
    Val->Y = U.Y / U.Z;
    Val->Z = U.X / U.Z;

    if (Val->X < 0 || Val->Y < 0 || Val->Z < 0)
        return false;

    return true;
}

static void _fglBlend(FglColor Src, FglColor *Dst)
{
    if (RenderState.BlendMode == FglBlendMode_None)
        *Dst = Src;
    else
        *Dst = fglColor(255, 50, 220);
}

void fglInit(void *VideoMemory, int32_t Width, int32_t Height, int32_t BPP, int32_t Stride, PixelOrder Order)
{
    memset(&RenderState, 0, sizeof(FglState));

    RenderState.VideoMemory = VideoMemory;
    RenderState.Width = Width;
    RenderState.Height = Height;
    RenderState.BPP = BPP;
    RenderState.Stride = Stride;
    RenderState.Order = Order;

    RenderState.FragmentShader = NULL;
    RenderState.VertexShader = NULL;

    RenderState.BorderColor = fglColor(255, 255, 255);
    RenderState.TextureWrap = FglTextureWrap_BorderColor;
    RenderState.BlendMode = FglBlendMode_None;

    RenderState.EnableBackfaceCulling = 0;
    RenderState.DepthBuffer = NULL;

    fgl_Identity_4x4(&RenderState.MatModel);
    fgl_Identity_4x4(&RenderState.MatView);
    fgl_Identity_4x4(&RenderState.MatProj);

    Scanline = (FglColor *)malloc(Width * sizeof(FglColor));
}

FglState *fglGetState()
{
    return &RenderState;
}

void fglSetState(FglState *State)
{
    RenderState = *State;
}

static ulong ms;
static ulong frame_counter = 0;

void fglBeginFrame()
{
    RenderState.RenderBounds = fgl_BBox(RenderState.Width, RenderState.Height, 0, 0);
    ms = millis();
}

void fglEndFrame()
{
    RenderState.LastRenderBounds = RenderState.RenderBounds;

    ulong ms_now = millis();
    ulong frame_time = ms_now - ms;
    ms = ms_now;

    RenderState.FrameTime = ms;

    if ((frame_counter++) % 10 == 0)
    {
        dprintf("Frame time: %lu ms - %.2f FPS\n", frame_time, (1.0f / (frame_time / 1000.0f)));
    }
}

// Shaders

void fglBindShader(void *Shader, FglShaderType ShaderType)
{
    if (ShaderType == FglShaderType_Fragment)
    {
        RenderState.FragmentShader = (void *)(FglFragmentFunc)Shader;
    }
    else if (ShaderType == FglShaderType_Vertex)
    {
        RenderState.VertexShader = (void *)(FglVertexFunc)Shader;
    }
}

// Buffers

FglBuffer fglCreateBuffer(void *Memory, int32_t Width, int32_t Height)
{
    FglBuffer Buffer;
    Buffer.Memory = Memory;
    Buffer.Length = Width * Height * sizeof(FglColor);
    Buffer.PixelCount = Width * Height;
    Buffer.Width = Width;
    Buffer.Height = Height;
    return Buffer;
}

FglBuffer fglCreateDepthBuffer(void *Memory, int32_t Width, int32_t Height)
{
    FglBuffer Buffer;
    Buffer.Memory = Memory;
    Buffer.Length = Width * Height * sizeof(uint8_t);
    Buffer.PixelCount = Width * Height;
    Buffer.Width = Width;
    Buffer.Height = Height;
    return Buffer;
}

FglBuffer fglCreateBufferFromPng(void *PngInMemory, int32_t Len)
{
    int32_t X, Y, Comp;
    FglBuffer Buffer;
    stbi_uc *Data;

    memset(&Buffer, 0, sizeof(FglBuffer));

    if ((Data = stbi_load_from_memory((stbi_uc *)PngInMemory, Len, &X, &Y, &Comp, 4)))
    {
        Buffer.Memory = (void *)Data;
        Buffer.Width = X;
        Buffer.Height = Y;
        Buffer.Length = X * Y * 4;
        Buffer.PixelCount = X * Y;
    }

    return Buffer;
}

void fglClearBuffer(FglBuffer *Buffer, FglColor Clr)
{
    for (size_t i = 0; i < Buffer->PixelCount; i++)
        Buffer->Pixels[i] = Clr;
}

void fglClearBufferRect(FglBuffer *Buffer, FglColor Clr, fglBBox Rect)
{
    size_t W = Buffer->Width;
    size_t H = Buffer->Height;

    for (size_t y = 0; y < H; y++)
    {
        for (size_t x = 0; x < W; x++)
        {
            Buffer->Pixels[y * W + x] = Clr;
        }
    }
}

void fglDisplayToFramebuffer(FglBuffer *Buffer)
{
    if (RenderState.BPP == 32 && RenderState.Stride == 0 && RenderState.Order == PixelOrder_RGBA)
    {
        memcpy(RenderState.VideoMemory, Buffer->Memory, Buffer->Length);
        return;
    }

    // TODO: Handle stride, pixel order and pixel size
}

void fglBindTexture(FglBuffer *TextureBuffer, int32_t Slot)
{
    RenderState.Textures[Slot] = *TextureBuffer;
}

void fglBindDepthBuffer(FglBuffer *Buffer)
{
    RenderState.DepthBuffer = Buffer;
}

void fglClearDepthBuffer()
{
    if (RenderState.DepthBuffer != NULL)
    {
        memset(RenderState.DepthBuffer->Memory, 0, RenderState.DepthBuffer->Length);
    }
}

// Drawing

void fglBlitSubTexture(FglBuffer *Buffer, FglBuffer *Tex, fglBBox Src, fglVec2i Dst)
{
    size_t width = Src.Max.X - Src.Min.X;
    size_t height = Src.Max.Y - Src.Min.Y;

    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            int dx = Dst.X + x;
            int dy = Dst.Y + y;
            int idx = dy * Buffer->Width + dx;

            if (idx >= Buffer->PixelCount)
                continue;

            int bx = Src.Min.X + x;
            int by = Src.Min.Y + y;
            int bidx = by * Tex->Width + bx;

            Buffer->Pixels[idx] = Tex->Pixels[bidx];
        }
    }
}

void fglDrawLine(FglBuffer *Buffer, FglColor Color, int32_t X0, int32_t Y0, int32_t X1, int32_t Y1)
{
    bool Steep = false;

    if (abs(X0 - X1) < abs(Y0 - Y1))
    {
        int Tmp = X0;
        X0 = Y0;
        Y0 = Tmp;

        Tmp = X1;
        X1 = Y1;
        Y1 = Tmp;

        Steep = true;
    }

    if (X0 > X1)
    {
        int Tmp = X0;
        X0 = X1;
        X1 = Tmp;

        Tmp = Y0;
        Y0 = Y1;
        Y1 = Tmp;
    }

    int DeltaX = X1 - X0;
    int DeltaY = Y1 - Y0;
    int DeltaError2 = abs(DeltaY) * 2;
    int Error2 = 0;
    int Y = Y0;

    for (int X = X0; X <= X1; X++)
    {
        if (Steep)
        {
            if (X < 0 || X >= Buffer->Height || Y < 0 || Y >= Buffer->Width)
                continue;

            Buffer->Pixels[X * Buffer->Width + Y] = Color;
        }
        else
        {
            if (Y < 0 || Y >= Buffer->Height || X < 0 || X >= Buffer->Width)
                continue;

            Buffer->Pixels[Y * Buffer->Width + X] = Color;
        }

        Error2 += DeltaError2;

        if (Error2 > DeltaX)
        {
            Y += (Y1 > Y0 ? 1 : -1);
            Error2 -= DeltaX * 2;
        }
    }
}

static fglMat3 _createVaryingMat(fglVec3 A, fglVec3 B, fglVec3 C)
{
    fglMat3 Mat;

    Mat.Row1.X = A.X;
    Mat.Row2.X = A.Y;
    Mat.Row3.X = A.Z;

    Mat.Row1.Y = B.X;
    Mat.Row2.Y = B.Y;
    Mat.Row3.Y = B.Z;

    Mat.Row1.Z = C.X;
    Mat.Row2.Z = C.Y;
    Mat.Row3.Z = C.Z;

    return Mat;
}

static FUNC_HOT int _fglRenderTriangle(FglBuffer *Buffer, fglVec3 A, fglVec3 B, fglVec3 C, fglVec2 UVA, fglVec2 UVB,
                                       fglVec2 UVC, fglBBox *BBox)
{
    if (RenderState.VertexShader == NULL || RenderState.FragmentShader == NULL)
        return 0;

    const fglMat3 Mat =
        _createVaryingMat(fgl_Vec3_from_Vec2(UVA, 0), fgl_Vec3_from_Vec2(UVB, 0), fgl_Vec3_from_Vec2(UVC, 0));

    fglVec2i Min;
    fglVec2i Max;
    fglVec3 UV;
    FglColor OutClr;

    uint8_t *DepthBufferMem = NULL;

    FglVertexFunc VertShader = (FglVertexFunc)RenderState.VertexShader;
    RenderState.CurShader = FglShaderType_Vertex;

    RenderState.VertNum = 0;
    if (VertShader(&RenderState, &A) == FGL_DISCARD)
        return 0;

    RenderState.VertNum = 1;
    if (VertShader(&RenderState, &B) == FGL_DISCARD)
        return 0;

    RenderState.VertNum = 2;
    if (VertShader(&RenderState, &C) == FGL_DISCARD)
        return 0;

    // Backface culling
    if (RenderState.EnableBackfaceCulling)
    {
        fglVec3 Cross = fgl_Vec3_Normalize(fgl_Cross3(fgl_Vec3_Sub(C, A), fgl_Vec3_Sub(B, A)));

        if (Cross.Z > 0)
            return 0;
    }

    if (RenderState.DepthBuffer)
    {
        DepthBufferMem = (uint8_t *)RenderState.DepthBuffer->Memory;
    }

    fglBoundingRect3(A, B, C, &Min, &Max);

    // Clamp to framebuffer size
    Min = fgl_Vec2i_Max(Min, fgl_Vec2i(0, 0));
    Max = fgl_Vec2i_Min(Max, fgl_Vec2i(Buffer->Width, Buffer->Height));

    BBox->Min = fgl_Vec2i_Min(BBox->Min, Min);
    BBox->Max = fgl_Vec2i_Max(BBox->Max, Max);

    FglFragmentFunc FragShader = (FglFragmentFunc)RenderState.FragmentShader;
    RenderState.CurShader = FglShaderType_Fragment;

    // ------
    for (size_t y = (size_t)Min.Y; y < Max.Y; y++)
    {
        for (size_t x = (size_t)Min.X; x < Max.X; x++)
        {
            if (x < 0 || x >= Buffer->Width)
                continue;

            if (y < 0 || y >= Buffer->Height)
                continue;

            fglVec3 Barycentric;
            if (_fglBarycentric(A, B, C, x, y, &Barycentric))
            {
                dspm_mult_3x3x1_f32_ae32((const float *)&Mat, (const float *)&Barycentric,
                                         (float *)&UV); // fgl_Mul_3x3_3x1

                int PixelIndex = y * Buffer->Width + x;
                uint8_t Depth = (uint8_t)(fgl_Vec3_Vary(A.Z, B.Z, C.Z, Barycentric) * 255);
                FglColor Pix;

                if (DepthBufferMem != NULL && DepthBufferMem[PixelIndex] > Depth)
                    continue;

                if (FragShader(&RenderState, fgl_Vec2(UV.X, UV.Y), &Pix) == FGL_DISCARD)
                    continue;

                Buffer->Pixels[PixelIndex] = Pix;

                if (DepthBufferMem != NULL)
                    DepthBufferMem[PixelIndex] = Depth;

                //_fglBlend(OutClr, &Buffer->Pixels[y * Buffer->Width + x]);
            }
        }
    }

    return 1;
}

static void _fglRenderTriangleDirect(FglBuffer *Buffer, fglVec3 A, fglVec3 B, fglVec3 C, fglVec2 UVA, fglVec2 UVB,
                                     fglVec2 UVC)
{
    if (RenderState.VertexShader == NULL || RenderState.FragmentShader == NULL)
        return;

    const fglMat3 Mat =
        _createVaryingMat(fgl_Vec3_from_Vec2(UVA, 0), fgl_Vec3_from_Vec2(UVB, 0), fgl_Vec3_from_Vec2(UVC, 0));
    FglVertexFunc VertShader = (FglVertexFunc)RenderState.VertexShader;
    RenderState.CurShader = FglShaderType_Vertex;

    RenderState.VertNum = 0;
    if (VertShader(&RenderState, &A) == FGL_DISCARD)
        return;

    RenderState.VertNum = 1;
    if (VertShader(&RenderState, &B) == FGL_DISCARD)
        return;

    RenderState.VertNum = 2;
    if (VertShader(&RenderState, &C) == FGL_DISCARD)
        return;

    fglVec2 Min, Max;
    fglVec3 UV;
    FglColor OutClr;
    fglBoundingRect2(A, B, C, &Min, &Max);

    FglFragmentFunc FragShader = (FglFragmentFunc)RenderState.FragmentShader;
    RenderState.CurShader = FglShaderType_Fragment;

    // ------
    for (size_t y = (size_t)Min.Y; y < Max.Y; y++)
    {
        for (size_t x = (size_t)Min.X; x < Max.X; x++)
        {
            fglVec3 Barycentric;
            if (_fglBarycentric(A, B, C, x, y, &Barycentric))
            {
                dspm_mult_3x3x1_f32_ae32((const float *)&Mat, (const float *)&Barycentric,
                                         (float *)&UV); // fgl_Mul_3x3_3x1

                /*if (FragShader(&RenderState, *(fglVec2 *)&UV, &Buffer->Pixels[y * Buffer->Width + x]) == FGL_DISCARD)
                    continue;*/

                FglColor clr;
                if (FragShader(&RenderState, *(fglVec2 *)&UV, &clr) == FGL_DISCARD)
                    continue;

                core2_st7789_draw_fb_pixel(clr.u16, x, y);

                //_fglBlend(OutClr, &Buffer->Pixels[y * Buffer->Width + x]);
            }
        }

        // core2_st7789_draw_fb_scanline(Scanline, y);
    }
}

/*void fglRenderTriangle3(FglBuffer *Buffer, FglTriangle3 *TriangleIn, FglTriangle2 *UVsIn)
{
    _fglRenderTriangle(Buffer, TriangleIn->A, TriangleIn->B, TriangleIn->C, UVsIn->A, UVsIn->B, UVsIn->C);
}
//*/

//*
void fglRenderTriangle3v(FglBuffer *Buffer, fglVec3 *vecs, fglVec2 *uvs, const size_t len, fglBBox *BBox)
{
    for (size_t i = 0; i < len; i += 3)
    {
        _fglRenderTriangle(Buffer, vecs[i + 0], vecs[i + 1], vecs[i + 2], uvs[i + 0], uvs[i + 1], uvs[i + 2], BBox);
    }
}
//*/
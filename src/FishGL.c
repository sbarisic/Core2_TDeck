#define STB_IMAGE_IMPLEMENTATION

#include <FishGL.h>
#include <FishGLShader.h>

#include <esp_dsp.h>
#include <byteswap.h>

// Core2
void core2_st7789_draw_fb_scanline(FglColor *colors, int y);
void core2_st7789_draw_fb_pixel(uint16_t color, int x, int y);

// MATH ===================================================================================================================================
// RENDERER ===============================================================================================================================

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

static void _fglBoundingRect(FglTriangle3 *Tri, fglVec2 *Min, fglVec2 *Max)
{
	Min->X = fgl_fminf(fgl_fminf(Tri->A.X, Tri->B.X), Tri->C.X);
	Min->Y = fgl_fminf(fgl_fminf(Tri->A.Y, Tri->B.Y), Tri->C.Y);

	Max->X = fgl_fmaxf(fgl_fmaxf(Tri->A.X, Tri->B.X), Tri->C.X);
	Max->Y = fgl_fmaxf(fgl_fmaxf(Tri->A.Y, Tri->B.Y), Tri->C.Y);
}

static void _fglBoundingRect2(fglVec3 A, fglVec3 B, fglVec3 C, fglVec2 *Min, fglVec2 *Max)
{
	Min->X = fgl_fminf(fgl_fminf(A.X, B.X), C.X);
	Min->Y = fgl_fminf(fgl_fminf(A.Y, B.Y), C.Y);

	Max->X = fgl_fmaxf(fgl_fmaxf(A.X, B.X), C.X);
	Max->Y = fgl_fmaxf(fgl_fmaxf(A.Y, B.Y), C.Y);
}

static bool _fglBarycentric(fglVec3 A, fglVec3 B, fglVec3 C, float X, float Y, fglVec3 *Val)
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

// Shaders

void fglBindShader(void *Shader, FglShaderType ShaderType)
{
	if (ShaderType == FglShaderType_Fragment)
	{
		RenderState.FragmentShader = (FglFragmentFunc)Shader;
	}
	else if (ShaderType == FglShaderType_Vertex)
	{
		RenderState.VertexShader = (FglVertexFunc)Shader;
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

// Drawing

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

static void _fglRenderTriangle(FglBuffer *Buffer, fglVec3 A, fglVec3 B, fglVec3 C, fglVec2 UVA, fglVec2 UVB, fglVec2 UVC)
{
	if (RenderState.VertexShader == NULL || RenderState.FragmentShader == NULL)
		return;

	const fglMat3 Mat = _createVaryingMat(fgl_Vec3_from_Vec2(UVA, 0), fgl_Vec3_from_Vec2(UVB, 0), fgl_Vec3_from_Vec2(UVC, 0));
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
	_fglBoundingRect2(A, B, C, &Min, &Max);

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
				dspm_mult_3x3x1_f32_ae32((const float *)&Mat, (const float *)&Barycentric, (float *)&UV); // fgl_Mul_3x3_3x1

				if (FragShader(&RenderState, *(fglVec2 *)&UV, &Buffer->Pixels[y * Buffer->Width + x]) == FGL_DISCARD)
					continue;

				//_fglBlend(OutClr, &Buffer->Pixels[y * Buffer->Width + x]);
			}
		}
	}
}

static void _fglRenderTriangleDirect(FglBuffer *Buffer, fglVec3 A, fglVec3 B, fglVec3 C, fglVec2 UVA, fglVec2 UVB, fglVec2 UVC)
{
	if (RenderState.VertexShader == NULL || RenderState.FragmentShader == NULL)
		return;

	const fglMat3 Mat = _createVaryingMat(fgl_Vec3_from_Vec2(UVA, 0), fgl_Vec3_from_Vec2(UVB, 0), fgl_Vec3_from_Vec2(UVC, 0));
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
	_fglBoundingRect2(A, B, C, &Min, &Max);

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
				dspm_mult_3x3x1_f32_ae32((const float *)&Mat, (const float *)&Barycentric, (float *)&UV); // fgl_Mul_3x3_3x1

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
void fglRenderTriangle3v(FglBuffer *Buffer, fglVec3 *vecs, fglVec2 *uvs, const size_t len)
{
	for (size_t i = 0; i < len; i += 3)
		_fglRenderTriangle(Buffer, vecs[i + 0], vecs[i + 1], vecs[i + 2], uvs[i + 0], uvs[i + 1], uvs[i + 2]);
}
//*/
#define STB_IMAGE_IMPLEMENTATION

#include <FishGL.h>
#include <FishGLShader.h>

#include <esp_dsp.h>
#include <byteswap.h>

#define fgl_fminf(a, b) ((a) < (b) ? (a) : (b))
#define fgl_fmaxf(a, b) ((a) < (b) ? (b) : (a))

FGL_API FglState RenderState;

void fglBoundingBox(FglTriangle3 *Tri, fglVec3 *Min, fglVec3 *Max)
{
	Min->X = fgl_fminf(fgl_fminf(Tri->A.X, Tri->B.X), Tri->C.X);
	Min->Y = fgl_fminf(fgl_fminf(Tri->A.Y, Tri->B.Y), Tri->C.Y);
	Min->Z = fgl_fminf(fgl_fminf(Tri->A.Z, Tri->B.Z), Tri->C.Z);

	Max->X = fgl_fmaxf(fgl_fmaxf(Tri->A.X, Tri->B.X), Tri->C.X);
	Max->Y = fgl_fmaxf(fgl_fmaxf(Tri->A.Y, Tri->B.Y), Tri->C.Y);
	Max->Z = fgl_fmaxf(fgl_fmaxf(Tri->A.Z, Tri->B.Z), Tri->C.Z);
}

void fglBoundingRect(FglTriangle3 *Tri, fglVec2 *Min, fglVec2 *Max)
{
	Min->X = fgl_fminf(fgl_fminf(Tri->A.X, Tri->B.X), Tri->C.X);
	Min->Y = fgl_fminf(fgl_fminf(Tri->A.Y, Tri->B.Y), Tri->C.Y);

	Max->X = fgl_fmaxf(fgl_fmaxf(Tri->A.X, Tri->B.X), Tri->C.X);
	Max->Y = fgl_fmaxf(fgl_fmaxf(Tri->A.Y, Tri->B.Y), Tri->C.Y);
}

bool fglBarycentric(FglTriangle3 *Tri, int32_t X, int32_t Y, fglVec3 *Val)
{
	fglVec3 U;
	fglVec3 a;
	fglVec3 b;

	a.X = (Tri->C.X) - (Tri->A.X);
	a.Y = (Tri->B.X) - (Tri->A.X);
	a.Z = (Tri->A.X) - (float)X;

	b.X = (Tri->C.Y) - (Tri->A.Y);
	b.Y = (Tri->B.Y) - (Tri->A.Y);
	b.Z = (Tri->A.Y) - (float)Y;

	//glm_vec_cross((float *)&a, (float *)&b, (float *)&U);
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

void fglBlend(FglColor Src, FglColor *Dst)
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

/*
void fglDrawTriangle3(FglBuffer *Buffer, FglColor Color, FglTriangle3 *Tri)
{
	 fglDrawLine(Buffer, Color, (int32_t)Tri->A[XElement], (int32_t)Tri->A[YElement], (int32_t)Tri->B[XElement], (int32_t)Tri->B[YElement]);
	 fglDrawLine(Buffer, Color, (int32_t)Tri->A[XElement], (int32_t)Tri->A[YElement], (int32_t)Tri->C[XElement], (int32_t)Tri->C[YElement]);
	 fglDrawLine(Buffer, Color, (int32_t)Tri->B[XElement], (int32_t)Tri->B[YElement], (int32_t)Tri->C[XElement], (int32_t)Tri->C[YElement]);
}*/

void fglFillTriangle3(FglBuffer *Buffer, FglColor Color, FglTriangle3 *Tri)
{
	fglVec2 Min, Max;
	fglVec3 V;
	fglBoundingRect(Tri, &Min, &Max);

	for (size_t y = (size_t)Min.Y; y < Max.Y; y++)
		for (size_t x = (size_t)Min.X; x < Max.X; x++)
		{
			if (fglBarycentric(Tri, x, y, &V))
				Buffer->Pixels[y * Buffer->Width + x] = Color;
		}
}

fglMat3 createVaryingMat(fglVec3 A, fglVec3 B, fglVec3 C)
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

void fglRenderTriangle3(FglBuffer *Buffer, FglTriangle3 *TriangleIn, FglTriangle2 *UVsIn)
{
	if (RenderState.FragmentShader == NULL)
		return;

	fglMat3 Mat = createVaryingMat(fgl_Vec3_from_Vec2(UVsIn->A, 0), fgl_Vec3_from_Vec2(UVsIn->B, 0), fgl_Vec3_from_Vec2(UVsIn->C, 0));
	FglTriangle3 Tri = *TriangleIn;

	if (RenderState.VertexShader != NULL)
	{
		FglVertexFunc VertShader = (FglVertexFunc)RenderState.VertexShader;
		RenderState.CurShader = FglShaderType_Vertex;

		RenderState.VertNum = 0;
		if (VertShader(&RenderState, &Tri.A) == FGL_DISCARD)
			return;

		RenderState.VertNum = 1;
		if (VertShader(&RenderState, &Tri.B) == FGL_DISCARD)
			return;

		RenderState.VertNum = 2;
		if (VertShader(&RenderState, &Tri.C) == FGL_DISCARD)
			return;
	}

	fglVec2 Min, Max;
	fglVec3 UV;
	FglColor OutClr;
	fglBoundingRect(&Tri, &Min, &Max);

	FglFragmentFunc FragShader = (FglFragmentFunc)RenderState.FragmentShader;
	RenderState.CurShader = FglShaderType_Fragment;

	for (size_t y = (size_t)Min.Y; y < Max.Y; y++)
		for (size_t x = (size_t)Min.X; x < Max.X; x++)
		{
			fglVec3 Barycentric;
			if (fglBarycentric(&Tri, x, y, &Barycentric))
			{
				fgl_Mul_3x3_3x1(&Mat, Barycentric, &UV);

				if (FragShader(&RenderState, *(fglVec2 *)&UV, &OutClr) == FGL_DISCARD)
					continue;

				fglBlend(OutClr, &Buffer->Pixels[y * Buffer->Width + x]);
			}
		}
}

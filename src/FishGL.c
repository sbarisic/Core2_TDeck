#define FGL_IMPLEMENTATION
#include <FishGL.h>
#include <FishGLShader.h>

#include <esp_dsp.h>
#include <byteswap.h>

#define fgl_fminf(a, b) ((a) < (b) ? (a) : (b))
#define fgl_fmaxf(a, b) ((a) < (b) ? (b) : (a))

// void fglBoundingBox(FglTriangle3 *Tri, vec3 Min, vec3 Max);
// void fglBoundingRect(FglTriangle3 *Tri, vec2 Min, vec2 Max);
// bool fglBarycentric(FglTriangle3 *Tri, int32_t X, int32_t Y, vec3 Val);
void fglBlend(FglColor Src, FglColor *Dst);

FGL_API FglState RenderState;

void fgl_Mul_4x4_4x1(const mat4 mat, const fglVec4 vec, fglVec4 *res)
{
	// vec4 temp_res;
	dspm_mult_4x4x1_f32_ae32((const float *)mat, (const float *)&vec, (float *)res);

	// glm_vec4_copy(temp_res, res);
}

void fgl_Mul_3x3_3x1(const mat3 mat, const fglVec3 vec, fglVec3 *res)
{
	mat3 mat2;
	glm_mat3_copy((float *)mat, mat2);
	glm_mat3_transpose(mat2);

	dspm_mult_3x3x1_f32_ae32((const float *)mat2, (const float *)&vec, (float *)res);
}

void fgl_Sub_3x1_3x1(const vec3 a, const vec3 b, vec3 res)
{
	dsps_sub_f32_ae32(a, b, res, 3, 1, 1, 1);
}

/*fglVec3 fgl_Sub_3x1_3x1(const fglVec3 a, const fglVec3 b)
{
	fglVec3 res = { 0 };
	dsps_add_f32_ae32(&a, &b, &res, 3, 1, 1, 1);
	return res;
}*/

FglColor fglColor(uint8_t r, uint8_t g, uint8_t b)
{
	FglColor clr = {.R = r >> 3, .G = g >> 2, .B = b >> 3};
	clr.u16 = __bswap16(clr.u16);
	return clr;
}

void fglColorToRGB(FglColor clr, uint8_t *r, uint8_t *g, uint8_t *b)
{
	clr.u16 = __bswap16(clr.u16);
	*r = clr.R << 3;
	*g = clr.G << 2;
	*b = clr.B << 3;
}

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

	glm_vec_cross((float *)&a, (float *)&b, (float *)&U);

	if (fabsf(U.Z) < 1)
		return false;

	Val->X = 1.0f - ((U.X + U.Y) / U.Z);
	Val->Y = U.Y / U.Z;
	Val->Z = U.X / U.Z;

	if (Val->X < 0 || Val->Y < 0 || Val->Z < 0)
		return false;

	return true;
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

void fglDrawTriangle3(FglBuffer *Buffer, FglColor Color, FglTriangle3 *Tri)
{
	// fglDrawLine(Buffer, Color, (int32_t)Tri->A[XElement], (int32_t)Tri->A[YElement], (int32_t)Tri->B[XElement], (int32_t)Tri->B[YElement]);
	// fglDrawLine(Buffer, Color, (int32_t)Tri->A[XElement], (int32_t)Tri->A[YElement], (int32_t)Tri->C[XElement], (int32_t)Tri->C[YElement]);
	// fglDrawLine(Buffer, Color, (int32_t)Tri->B[XElement], (int32_t)Tri->B[YElement], (int32_t)Tri->C[XElement], (int32_t)Tri->C[YElement]);
}

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

void fglRenderTriangle3(FglBuffer *Buffer, FglTriangle3 *TriangleIn, FglTriangle2 *UVsIn)
{
	if (UVsIn != NULL)
	{
		memcpy(RenderState.VarIn[0].A.Vec, &UVsIn->A, sizeof(vec2));
		memcpy(RenderState.VarIn[0].B.Vec, &UVsIn->B, sizeof(vec2));
		memcpy(RenderState.VarIn[0].C.Vec, &UVsIn->C, sizeof(vec2));
	}

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
	fglBoundingRect(&Tri, &Min, &Max);

	for (size_t y = (size_t)Min.Y; y < Max.Y; y++)
		for (size_t x = (size_t)Min.X; x < Max.X; x++)
		{
			fglVec3 Barycentric;
			if (fglBarycentric(&Tri, x, y, &Barycentric))
			{
				// glm_mat3_mulv((vec3 *)&RenderState.VarIn[0].Mat, Barycentric, (float *)&RenderState.VarOut[0].Vec3);

				if (RenderState.FragmentShader != NULL)
				{
					for (size_t i = 0; i < FGL_VARYING_COUNT; i++)
					{
						fgl_Mul_3x3_3x1(RenderState.VarIn[i].Mat, Barycentric, RenderState.VarOut[i].Vec);
						// glm_mat3_mulv((vec3 *)&(RenderState.VarIn[i].Mat), Barycentric, (float *)&(RenderState.VarOut[i].Vec3));
					}

					FglColor OutClr;
					FglFragmentFunc FragShader = (FglFragmentFunc)RenderState.FragmentShader;
					RenderState.CurShader = FglShaderType_Fragment;

					if (FragShader(&RenderState, *(fglVec2 *)RenderState.VarOut[0].Vec, &OutClr) == FGL_DISCARD)
						continue;

					fglBlend(OutClr, &Buffer->Pixels[y * Buffer->Width + x]);
				}
				else
				{
					fgl_Mul_3x3_3x1(RenderState.VarIn[0].Mat, Barycentric, RenderState.VarOut[0].Vec);
					fglBlend(fglShaderSampleTextureUV(&RenderState.Textures[0], *(fglVec2 *)RenderState.VarOut[0].Vec), &Buffer->Pixels[y * Buffer->Width + x]);
				}
			}
		}
}

void fglBlend(FglColor Src, FglColor *Dst)
{
	if (RenderState.BlendMode == FglBlendMode_None)
		*Dst = Src;
	else
		*Dst = fglColor(255, 50, 220);
}
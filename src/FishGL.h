#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <FishGLConfig.h>

#define XElement 0
#define YElement 1
#define ZElement 2
#define WElement 3

#define FGL_DISCARD true
#define FGL_KEEP false

#define FGL_PACKED __attribute__((packed))

#define COPY_VEC2(dst, src)            \
	do                                 \
	{                                  \
		dst[XElement] = src[XElement]; \
		dst[YElement] = src[YElement]; \
	} while (0)

	typedef struct
	{
		float X;
		float Y;
	} fglVec2;

	typedef struct
	{
		float X;
		float Y;
		float Z;
	} fglVec3;

	typedef struct
	{
		float X;
		float Y;
		float Z;
		float W;
	} fglVec4;

	typedef enum
	{
		PixelOrder_Unknown,
		PixelOrder_RGBA,
		PixelOrder_ABGR,
		PixelOrder_RGB_565,
	} PixelOrder;

	// 16 bit 565 RGB, HI and LO bytes are swapped
	typedef union
	{
		struct
		{
			unsigned int R : 5;
			unsigned int G : 6;
			unsigned int B : 5;
		} FGL_PACKED;

		struct
		{
			unsigned int u8_LO : 8;
			unsigned int u8_HI : 8;
		} FGL_PACKED;

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
		union
		{
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

	// These are not vec3 and vec2 because of padding
	typedef union
	{
		float Vec[3];
	} FglVarying;

	typedef union
	{
		mat3 Mat;

		struct
		{
			FglVarying A;
			FglVarying B;
			FglVarying C;
		};
	} FglVaryingIn;

	typedef struct
	{
		void *VideoMemory;
		int32_t Width;
		int32_t Height;
		int32_t BPP;
		int32_t Stride;
		PixelOrder Order;

		mat4 MatProj;
		mat4 MatView;
		mat4 MatModel;

		FglBlendMode BlendMode;
		FglTextureWrap TextureWrap;

		FglColor BorderColor;
		FglBuffer Textures[FGL_MAX_TEXTURES];

		FglVaryingIn VarIn[FGL_VARYING_COUNT];
		FglVarying VarOut[FGL_VARYING_COUNT];

		int32_t VertNum;
		FglShaderType CurShader;

		void *VertexShader;
		void *FragmentShader;
	} FglState;

	typedef bool (*FglVertexFunc)(FglState *State, fglVec3* Vert);
	typedef bool (*FglFragmentFunc)(FglState *State, fglVec2 UV, FglColor *OutColor);

	// Basics
	FglColor fglColor(uint8_t r, uint8_t g, uint8_t b);
	void fglColorToRGB(FglColor clr, uint8_t *r, uint8_t *g, uint8_t *b);

	// Initialization and state
	FGL_API void fglInit(void *VideoMemory, int32_t Width, int32_t Height, int32_t BPP, int32_t Stride, PixelOrder Order);
	FGL_API FglState *fglGetState();
	FGL_API void fglSetState(FglState *State);

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
	FGL_API void fglFillTriangle3(FglBuffer *Buffer, FglColor Color, FglTriangle3 *Tri);
	FGL_API void fglRenderTriangle3(FglBuffer *Buffer, FglTriangle3 *Tri, FglTriangle2 *UV);

	// Math
	void fgl_Mul_4x4_4x1(const mat4 mat, const fglVec4 vec, fglVec4* res);

#ifdef __cplusplus
}
#endif
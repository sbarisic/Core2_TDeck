#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <FishGL.h>
#include <FishGLConfig.h>

FGL_API FGL_INLINE FglColor fglShaderSampleTexture(FglBuffer* TextureBuffer, int32_t X, int32_t Y);
FGL_API FGL_INLINE FglColor fglShaderSampleTextureUV(FglBuffer* TextureBuffer, vec2 UV);
FGL_API FGL_INLINE FglVarying* fglShaderGetVarying(int32_t Num);

#ifdef __cplusplus
}
#endif

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <FishGL.h>
#include <FishGLConfig.h>

FGL_API FglColor fglShaderSampleTexture(FglBuffer* TextureBuffer, int32_t X, int32_t Y);
FGL_API FglColor fglShaderSampleTextureUV(FglBuffer* TextureBuffer, fglVec2 UV);
FGL_API FglVarying* fglShaderGetVarying(int32_t Num);

#ifdef __cplusplus
}
#endif

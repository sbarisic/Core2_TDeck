#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
//#include <cglm/cglm.h>

#define FGL_MAX_TEXTURES 8
// #define FGL_VARYING_COUNT 3 // Must be more or equal to 1

#define FGL_API


#ifdef FGL_MALLOC
#define STBI_MALLOC FGL_MALLOC
#endif
#ifdef FGL_REALLOC
#define STBI_REALLOC FGL_REALLOC
#endif
#ifdef FGL_FREE
#define STBI_FREE FGL_FREE
#endif

#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#include <stb_image.h>
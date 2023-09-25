#include <FishGL.h>
#include <FishGLShader.h>

FglColor fglShaderSampleTexture(FglBuffer *TextureBuffer, int32_t X, int32_t Y)
{
	FglState *RenderState = fglGetState();

	if (X < 0 || X >= TextureBuffer->Width || Y < 0 || Y >= TextureBuffer->Height)
	{
		if (RenderState->TextureWrap == FglTextureWrap_BorderColor)
		{
			return RenderState->BorderColor;
		}
		else if (RenderState->TextureWrap == FglTextureWrap_Clamp)
		{
			if (X < 0)
			{
				X = 0;
			}
			else if (X >= TextureBuffer->Width)
			{
				X = TextureBuffer->Width - 1;
			}

			if (Y < 0)
			{
				Y = 0;
			}
			else if (Y >= TextureBuffer->Height)
			{
				Y = TextureBuffer->Height - 1;
			}
		}
		else
			return fglColor(255, 0, 0);
	}

	return TextureBuffer->Pixels[Y * TextureBuffer->Width + X];
}

FglColor fglShaderSampleTextureUV(FglBuffer *TextureBuffer, fglVec2 UV)
{
	return fglShaderSampleTexture(TextureBuffer, (int32_t)(UV.X * TextureBuffer->Width), (int32_t)(UV.Y * TextureBuffer->Height));
}

/*FglVarying *fglShaderGetVarying(int32_t Num)
{
	FglState *RenderState = fglGetState();

	if (RenderState->CurShader == FglShaderType_Vertex)
		return &((FglVarying *)&RenderState->VarIn[Num])[RenderState->VertNum];
	else if (RenderState->CurShader == FglShaderType_Fragment)
		return &(RenderState->VarOut[Num]);

	return NULL;
}*/
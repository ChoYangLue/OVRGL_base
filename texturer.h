#pragma once

#include "GL/CAPI_GLE.h"
#include "Extras/OVR_Math.h"
#include "OVR_CAPI_GL.h"
#include <assert.h>
#include <vector>


using namespace OVR;


DWORD GetColor(int Red, int Green, int Blue, int Alpha = 255);

//---------------------------------------------------------------------------------------
struct DepthBuffer
{
	GLuint        texId;

	DepthBuffer(Sizei size)
	{
		glGenTextures(1, &texId);
		glBindTexture(GL_TEXTURE_2D, texId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		GLenum internalFormat = GL_DEPTH_COMPONENT24;
		GLenum type = GL_UNSIGNED_INT;
		if (GLE_ARB_depth_buffer_float)
		{
			internalFormat = GL_DEPTH_COMPONENT32F;
			type = GL_FLOAT;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.w, size.h, 0, GL_DEPTH_COMPONENT, type, NULL);
	}
	~DepthBuffer()
	{
		if (texId)
		{
			glDeleteTextures(1, &texId);
			texId = 0;
		}
	}
};

//--------------------------------------------------------------------------
struct TextureBuffer
{
	GLuint              texId;
	GLuint              fboId;
	Sizei               texSize;

	TextureBuffer(bool rendertarget, Sizei size, int mipLevels, unsigned char * data);
	TextureBuffer(bool rendertarget, const std::string& path, int mipLevels);

	~TextureBuffer();

	Sizei GetSize() const;

	void SetAndClearRenderSurface(DepthBuffer* dbuffer);

	void UnsetRenderSurface();

	// âÊëúÇì«Ç›çûÇﬁ
	bool CreateTexture(const std::string& path);
};

#include "texturer.h"

// 画像を扱う
#define STB_IMAGE_IMPLEMENTATION
#include "Common/stb_image.h"

#ifndef VALIDATE
#define VALIDATE(x, msg) if (!(x)) { MessageBoxA(NULL, (msg), "OculusRoomTiny", MB_ICONERROR | MB_OK); exit(-1); }
#endif

#ifndef OVR_DEBUG_LOG
#define OVR_DEBUG_LOG(x)
#endif


DWORD GetColor(int Red, int Green, int Blue, int Alpha)
{
	return ((Alpha & 0xff) << 24) + ((Red & 0xff) << 16) + ((Green & 0xff) << 8) + (Blue & 0xff);
}

TextureBuffer::TextureBuffer(bool rendertarget, Sizei size, int mipLevels, unsigned char * data) :
	texId(0),
	fboId(0),
	texSize(0, 0)
{
	texSize = size;

	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);

	if (rendertarget)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, texSize.w, texSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	if (mipLevels > 1)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	if (rendertarget)
	{
		glGenFramebuffers(1, &fboId);
	}
}

TextureBuffer::TextureBuffer(bool rendertarget, const std::string& path, int mipLevels=0) :
	texId(0),
	fboId(0),
	texSize(0, 0)
{
	// テクスチャを１つ生成
	glGenTextures(1, &texId);

	// テクスチャを拘束
	// NOTICE 以下テクスチャに対する命令は拘束したテクスチャに対して実行される
	glBindTexture(GL_TEXTURE_2D, texId);

	// 画像を読み込む
	int comp;
	unsigned char *data = stbi_load(path.c_str(), &texSize.w, &texSize.h, &comp, 0);

	// アルファの有無でデータ形式が異なる
	GLint type = (comp == 3) ? GL_RGB : GL_RGBA;

	// 画像データをOpenGLへ転送
	glTexImage2D(GL_TEXTURE_2D, 0, type, texSize.w, texSize.h, 0, type, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);

	// 表示用の細々とした設定
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	if (mipLevels > 1)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	if (rendertarget)
	{
		glGenFramebuffers(1, &fboId);
	}
}

TextureBuffer::~TextureBuffer()
{
	if (texId)
	{
		glDeleteTextures(1, &texId);
		texId = 0;
	}
	if (fboId)
	{
		glDeleteFramebuffers(1, &fboId);
		fboId = 0;
	}
}

Sizei TextureBuffer::GetSize() const { return texSize; }

void TextureBuffer::SetAndClearRenderSurface(DepthBuffer* dbuffer)
{
	VALIDATE(fboId, "Texture wasn't created as a render target");

	GLuint curTexId = texId;

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dbuffer->texId, 0);

	glViewport(0, 0, texSize.w, texSize.h);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_FRAMEBUFFER_SRGB);
}

void TextureBuffer::UnsetRenderSurface()
{
	VALIDATE(fboId, "Texture wasn't created as a render target");

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
}

bool TextureBuffer::CreateTexture(const std::string& path)
{
	// テクスチャを１つ生成
	glGenTextures(1, &texId);

	// テクスチャを拘束
	// NOTICE 以下テクスチャに対する命令は拘束したテクスチャに対して実行される
	glBindTexture(GL_TEXTURE_2D, texId);

	// 画像を読み込む
	int comp;
	unsigned char *data = stbi_load(path.c_str(), &texSize.w, &texSize.h, &comp, 0);

	// アルファの有無でデータ形式が異なる
	GLint type = (comp == 3) ? GL_RGB : GL_RGBA;

	// 画像データをOpenGLへ転送
	glTexImage2D(GL_TEXTURE_2D, 0, type, texSize.w, texSize.h, 0, type, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);

	// 表示用の細々とした設定
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	return true;
}
#pragma once

#include "GL/CAPI_GLE.h"
#include "Extras/OVR_Math.h"
#include "OVR_CAPI_GL.h"
#include <assert.h>
#include <vector>
#include "texturer.h"

using namespace OVR;

#ifndef VALIDATE
#define VALIDATE(x, msg) if (!(x)) { MessageBoxA(NULL, (msg), "OculusRoomTiny", MB_ICONERROR | MB_OK); exit(-1); }
#endif

#ifndef OVR_DEBUG_LOG
#define OVR_DEBUG_LOG(x)
#endif


//------------------------------------------------------------------------------
struct ShaderFill
{
	GLuint            program;
	TextureBuffer   * texture;

	ShaderFill(GLuint vertexShader, GLuint pixelShader, TextureBuffer* _texture);
	~ShaderFill();
};


//----------------------------------------------------------------
struct VertexBuffer
{
	GLuint    buffer;

	VertexBuffer(void* vertices, size_t size)
	{
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	}
	~VertexBuffer()
	{
		if (buffer)
		{
			glDeleteBuffers(1, &buffer);
			buffer = 0;
		}
	}
};

//----------------------------------------------------------------
struct IndexBuffer
{
	GLuint    buffer;

	IndexBuffer(void* indices, size_t size)
	{
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
	}
	~IndexBuffer()
	{
		if (buffer)
		{
			glDeleteBuffers(1, &buffer);
			buffer = 0;
		}
	}
};


//---------------------------------------------------------------------------
struct Vertex
{
	Vector3f  Pos;
	DWORD     C;
	float     U, V;
	Vector3f  Norm;
};

struct Model
{
	Vector3f        Pos;
	Quatf           Rot;
	Vector3f		Scale;
	Matrix4f        Mat;
	int             numVertices, numIndices;
	std::vector<Vertex>          Vertices;
	std::vector<GLushort>        Indices;
	ShaderFill    * Fill;
	VertexBuffer  * vertexBuffer;
	IndexBuffer   * indexBuffer;

	Model(Vector3f pos, ShaderFill * fill);
	~Model();

	Matrix4f& GetMatrix();

	void AddVertex(const Vertex& v);
	void AddIndex(GLushort a);

	void AllocateBuffers();

	void FreeBuffers();

	void AddSolidColorBox(float x1, float y1, float z1, float x2, float y2, float z2, DWORD c);

	void Render(Matrix4f view, Matrix4f proj);
};

/*
struct DirectionalLight
{
	Vector3f Pos;
};
int CreateDirLightHandle(Vector3f Direction);
*/

int MV1SetPosition(Model& MHandle, Vector3f Position);

Vector3f MV1GetPosition(Model& MHandle);

int MV1SetScale(Model& MHandle, Vector3f Scale);

Vector3f MV1GetScale(Model& MHandle);

int MV1SetRotationXYZ(Model& MHandle, Vector3f Rotate);

Vector3f MV1GetRotationXYZ(Model& MHandle);


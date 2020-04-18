#pragma once

#include "./Common/Win32_GLAppUtil.h"

class Scene
{
public:
	Scene() : numModels(0) {}
	Scene(bool includeIntensiveGPUobject) :numModels(0) { m_includeIntensiveGPUobject = includeIntensiveGPUobject; Init(includeIntensiveGPUobject); }
	~Scene() { Release(); }

	void Add(Model * n);
	void Render(Matrix4f view, Matrix4f proj);

	GLuint CreateShader(GLenum type, const GLchar* src);

	void Init(int includeIntensiveGPUobject);
	void Release();

	Model &getModelByIndex(int index);

private:
	bool m_includeIntensiveGPUobject = false;
	int     numModels;
	Model * Models[10];
};
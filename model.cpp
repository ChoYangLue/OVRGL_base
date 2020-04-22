
#include "model.h"


ShaderFill::ShaderFill(GLuint vertexShader, GLuint pixelShader, TextureBuffer* _texture)
{
	texture = _texture;

	program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, pixelShader);

	glLinkProgram(program);

	glDetachShader(program, vertexShader);
	glDetachShader(program, pixelShader);

	GLint r;
	glGetProgramiv(program, GL_LINK_STATUS, &r);
	if (!r)
	{
		GLchar msg[1024];
		glGetProgramInfoLog(program, sizeof(msg), 0, msg);
		OVR_DEBUG_LOG(("Linking shaders failed: %s\n", msg));
	}
}

ShaderFill::~ShaderFill()
{
	if (program)
	{
		glDeleteProgram(program);
		program = 0;
	}
	if (texture)
	{
		delete texture;
		texture = nullptr;
	}
}


Vector3f faceNormal(Vector3f v0, Vector3f  v1, Vector3f  v2) {
	Vector3f n(0,0,0);

	// 頂点を結ぶベクトルを算出
	Vector3f  vec1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
	Vector3f  vec2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);

	// ベクトル同士の外積
	n.x = vec1.y * vec2.z - vec1.z * vec2.y;
	n.y = vec1.z * vec2.x - vec1.x * vec2.z;
	n.z = vec1.x * vec2.y - vec1.y * vec2.x;

	return n;
}

Model::Model(Vector3f pos, ShaderFill * fill) :
	numVertices(0),
	numIndices(0),
	Pos(pos),
	Rot(),
	Scale(Vector3f(1, 1, 1)),
	Mat(),
	Fill(fill),
	vertexBuffer(nullptr),
	indexBuffer(nullptr)
{}
Model::~Model() { FreeBuffers(); }

Matrix4f& Model::GetMatrix()
{
	Mat = Matrix4f(Rot);
	Mat = Matrix4f::Translation(Pos) * Mat; // 位置行列ｘ回転行列
	return Mat;
}

void Model::AddVertex(const Vertex& v)
{
	Vertices.push_back(v);
	numVertices++;
}
void Model::AddIndex(GLushort a)
{
	Indices.push_back(a);
	numIndices++;
}

void Model::AllocateBuffers()
{
	vertexBuffer = new VertexBuffer(&Vertices[0], numVertices * sizeof(Vertices[0]));
	indexBuffer = new IndexBuffer(&Indices[0], numIndices * sizeof(Indices[0]));
}

void Model::FreeBuffers()
{
	delete vertexBuffer; vertexBuffer = nullptr;
	delete indexBuffer; indexBuffer = nullptr;
}

void Model::AddSolidColorBox(float x1, float y1, float z1, float x2, float y2, float z2, DWORD c)
{
	Vector3f Vert[][3] =
	{
		Vector3f(x1, y2, z1), Vector3f(z1, x1),Vector3f(x1, y2, z1),  Vector3f(x2, y2, z1), Vector3f(z1, x2),Vector3f(x1, y2, z1),
		Vector3f(x2, y2, z2), Vector3f(z2, x2),Vector3f(x1, y2, z1),  Vector3f(x1, y2, z2), Vector3f(z2, x1),Vector3f(x1, y2, z1),
		Vector3f(x1, y1, z1), Vector3f(z1, x1),Vector3f(x1, y2, z1),  Vector3f(x2, y1, z1), Vector3f(z1, x2),Vector3f(x1, y2, z1),
		Vector3f(x2, y1, z2), Vector3f(z2, x2),Vector3f(x1, y2, z1),  Vector3f(x1, y1, z2), Vector3f(z2, x1),Vector3f(x1, y2, z1),
		Vector3f(x1, y1, z2), Vector3f(z2, y1),Vector3f(x1, y2, z1),  Vector3f(x1, y1, z1), Vector3f(z1, y1),Vector3f(x1, y2, z1),
		Vector3f(x1, y2, z1), Vector3f(z1, y2),Vector3f(x1, y2, z1),  Vector3f(x1, y2, z2), Vector3f(z2, y2),Vector3f(x1, y2, z1),
		Vector3f(x2, y1, z2), Vector3f(z2, y1),Vector3f(x1, y2, z1),  Vector3f(x2, y1, z1), Vector3f(z1, y1),Vector3f(x1, y2, z1),
		Vector3f(x2, y2, z1), Vector3f(z1, y2),Vector3f(x1, y2, z1),  Vector3f(x2, y2, z2), Vector3f(z2, y2),Vector3f(x1, y2, z1),
		Vector3f(x1, y1, z1), Vector3f(x1, y1),Vector3f(x1, y2, z1),  Vector3f(x2, y1, z1), Vector3f(x2, y1),Vector3f(x1, y2, z1),
		Vector3f(x2, y2, z1), Vector3f(x2, y2),Vector3f(x1, y2, z1),  Vector3f(x1, y2, z1), Vector3f(x1, y2),Vector3f(x1, y2, z1),
		Vector3f(x1, y1, z2), Vector3f(x1, y1),Vector3f(x1, y2, z1),  Vector3f(x2, y1, z2), Vector3f(x2, y1),Vector3f(x1, y2, z1),
		Vector3f(x2, y2, z2), Vector3f(x2, y2),Vector3f(x1, y2, z1),  Vector3f(x1, y2, z2), Vector3f(x1, y2),Vector3f(x1, y2, z1)
	};

	GLushort CubeIndices[] =
	{
		0, 1, 3, 3, 1, 2,
		5, 4, 6, 6, 4, 7,
		8, 9, 11, 11, 9, 10,
		13, 12, 14, 14, 12, 15,
		16, 17, 19, 19, 17, 18,
		21, 20, 22, 22, 20, 23
	};

	std::vector<Vector3f> CubeNormals;

	for (int v = 0; v < 6 * 4; v+=3) {
		Vector3f nrm = faceNormal(Vert[CubeIndices[v]][0], Vert[CubeIndices[v+1]][0], Vert[CubeIndices[v+2]][0]);
		Vert[CubeIndices[v]][2] = nrm;
		Vert[CubeIndices[v+1]][2] = nrm;
		Vert[CubeIndices[v+2]][2] = nrm;
	}

	for (int i = 0; i < sizeof(CubeIndices) / sizeof(CubeIndices[0]); ++i)
		AddIndex(CubeIndices[i] + GLushort(numVertices));

	// Generate a quad for each box face
	for (int v = 0; v < 6 * 4; v++)
	{
		// Make vertices, with some token lighting
		Vertex vvv;
		vvv.Pos = Vert[v][0];
		vvv.U = Vert[v][1].x;
		vvv.V = Vert[v][1].y;
		float dist1 = (vvv.Pos - Vector3f(-2, 4, -2)).Length();
		float dist2 = (vvv.Pos - Vector3f(3, 4, -3)).Length();
		float dist3 = (vvv.Pos - Vector3f(-4, 3, 25)).Length();
		int   bri = rand() % 160;
		float B = ((c >> 16) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
		float G = ((c >> 8) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
		float R = ((c >> 0) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
		vvv.C = (c & 0xff000000) +
			((R > 255 ? 255 : DWORD(R)) << 16) +
			((G > 255 ? 255 : DWORD(G)) << 8) +
			(B > 255 ? 255 : DWORD(B));
		//vvv.Norm = Vert[v][2];
		vvv.Norm = Vector3f(0,0,0);
		AddVertex(vvv);
	}
}

void Model::Render(Matrix4f view, Matrix4f proj)
{
	Matrix4f combined = proj * view * GetMatrix();

	glUseProgram(Fill->program);
	glUniform1i(glGetUniformLocation(Fill->program, "Texture0"), 0);
	glUniformMatrix4fv(glGetUniformLocation(Fill->program, "matWVP"), 1, GL_TRUE, (FLOAT*)&combined);

	GLfloat se[3];
	se[0] = 0.0f;
	se[1] = 0.0f;
	se[2] = 0.0f;
	glUniform3f(glGetUniformLocation(Fill->program, "LightDirection"), se[0], se[1], se[2]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Fill->texture->texId);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->buffer);

	GLuint posLoc = glGetAttribLocation(Fill->program, "Position");
	GLuint colorLoc = glGetAttribLocation(Fill->program, "Color");
	GLuint uvLoc = glGetAttribLocation(Fill->program, "TexCoord");
	GLuint normLoc = glGetAttribLocation(Fill->program, "Normal");

	glEnableVertexAttribArray(posLoc);
	glEnableVertexAttribArray(colorLoc);
	glEnableVertexAttribArray(uvLoc);
	glEnableVertexAttribArray(normLoc);

	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, Pos));
	glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, C));
	glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, U));
	glVertexAttribPointer(normLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, Norm));

	glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, NULL);

	glDisableVertexAttribArray(posLoc);
	glDisableVertexAttribArray(colorLoc);
	glDisableVertexAttribArray(uvLoc);
	glDisableVertexAttribArray(normLoc);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glUseProgram(0);
}



















int MV1SetPosition(Model& MHandle, Vector3f Position)
{
	MHandle.Pos = Position;
	return 0;
}

Vector3f MV1GetPosition(Model& MHandle)
{
	return MHandle.Pos;
}

int MV1SetScale(Model& MHandle, Vector3f Scale)
{
	MHandle.Scale = Scale;
	return 0;
}

Vector3f MV1GetScale(Model& MHandle)
{
	return MHandle.Scale;
}

int MV1SetRotationXYZ(Model& MHandle, Vector3f Rotate)
{
	// 尚、回転の順番は x軸回転 → y軸回転 → z軸回転 です。
	Quatf RotationX(Vector3f(1, 0, 0), Rotate.x);
	Quatf RotationY(Vector3f(0, 1, 0), Rotate.y);
	Quatf RotationZ(Vector3f(0, 0, 1), Rotate.z);

	MHandle.Rot = RotationX * RotationY*RotationZ;

	return 0;
}

Vector3f MV1GetRotationXYZ(Model& MHandle)
{
	double q0q0 = MHandle.Rot.x * MHandle.Rot.x;
	double q0q1 = MHandle.Rot.x * MHandle.Rot.y;
	double q0q2 = MHandle.Rot.x * MHandle.Rot.z;
	double q0q3 = MHandle.Rot.x * MHandle.Rot.w;
	double q1q1 = MHandle.Rot.y * MHandle.Rot.y;
	double q1q2 = MHandle.Rot.y * MHandle.Rot.z;
	double q1q3 = MHandle.Rot.y * MHandle.Rot.w;
	double q2q2 = MHandle.Rot.z * MHandle.Rot.z;
	double q2q3 = MHandle.Rot.z * MHandle.Rot.w;
	double q3q3 = MHandle.Rot.w * MHandle.Rot.w;
	Vector3f Rote;
	Rote.x = (float) atan2(2.0 * (q2q3 + q0q1), q0q0 - q1q1 - q2q2 + q3q3);
	Rote.y = (float) asin(2.0 * (q0q2 - q1q3));
	Rote.z = (float) atan2(2.0 * (q1q2 + q0q3), q0q0 + q1q1 - q2q2 - q3q3);
	return Rote;
}
#include "scene.h"
#include "obj_loader.h"

void Scene::Add(Model * n)
{
	Models[numModels++] = n;
}

void Scene::Render(Matrix4f view, Matrix4f proj)
{
	for (int i = 0; i < numModels; ++i)
		Models[i]->Render(view, proj);
}

GLuint Scene::CreateShader(GLenum type, const GLchar* src)
{
	GLuint shader = glCreateShader(type);

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint r;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &r);
	if (!r)
	{
		GLchar msg[1024];
		glGetShaderInfoLog(shader, sizeof(msg), 0, msg);
		if (msg[0]) {
			OVR_DEBUG_LOG(("Compiling shader failed: %s\n", msg));
		}
		return 0;
	}

	return shader;
}

void Scene::Init(int includeIntensiveGPUobject)
{
	static const GLchar* VertexShaderSrc =
		"#version 150\n"
		"uniform mat4 matWVP;\n"
		"uniform mat4  model;\n"
		"in      vec4 Position;\n"
		"in      vec4 Color;\n"
		"in      vec2 TexCoord;\n"
		"in      vec3 Normal;\n"
		"out     vec2 oTexCoord;\n"
		"out     vec4 oColor;\n"
		"out     vec3 oNormal;\n"
		"out	 vec3 FragPos;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = (matWVP * Position);\n"
		"   oTexCoord   = TexCoord;\n"
		"	oNormal = Normal;\n"
		"	FragPos = vec3(model * Position);\n"
		"   oColor.rgb  = pow(Color.rgb, vec3(2.2));\n"   // convert from sRGB to linear
		"   oColor.a    = Color.a;\n"
		"}\n";

	static const char* FragmentShaderSrc =
		"#version 150\n"
		"uniform sampler2D Texture0;\n"
		"uniform vec3 lightPos;\n"
		"in      vec4      oColor;\n"
		"in      vec2      oTexCoord;\n"
		"in      vec3	   oNormal;\n"
		"in		 vec3	   FragPos;\n"
		"out     vec4      FragColor;\n"
		"void main()\n"
		"{\n"
		"   float ambientStrength = 0.4;\n"
		"   vec3 lightColor = vec3(1.0, 1.0, 1.0);\n"
		"   vec3 ambient = ambientStrength * lightColor;\n"
		"	vec3 norm = normalize(oNormal);\n"
		"	vec3 lightDir = normalize(lightPos - FragPos);\n"
		"	float diff = max(dot(norm, lightDir), 0.0);\n"
		"	vec3 diffuse = diff * lightColor;\n"
		"	vec3 result = (ambient + diffuse) *oColor.rgb * texture2D(Texture0, oTexCoord).xyz;\n"
		"	FragColor = vec4(result, oColor.a);\n"
		"}\n";

	GLuint    vshader = CreateShader(GL_VERTEX_SHADER, VertexShaderSrc);
	GLuint    fshader = CreateShader(GL_FRAGMENT_SHADER, FragmentShaderSrc);

	// Make textures
	ShaderFill * grid_material[5];
	for (int k = 0; k < 4; ++k)
	{
		static DWORD tex_pixels[256 * 256];
		for (int j = 0; j < 256; ++j)
		{
			for (int i = 0; i < 256; ++i)
			{
				if (k == 0) tex_pixels[j * 256 + i] = (((i >> 7) ^ (j >> 7)) & 1) ? 0xffb4b4b4 : 0xff505050;// floor
				if (k == 1) tex_pixels[j * 256 + i] = (((j / 4 & 15) == 0) || (((i / 4 & 15) == 0) && ((((i / 4 & 31) == 0) ^ ((j / 4 >> 4) & 1)) == 0)))
					? 0xff3c3c3c : 0xffb4b4b4;// wall
				if (k == 2) tex_pixels[j * 256 + i] = (i / 4 == 0 || j / 4 == 0) ? 0xff505050 : 0xffb4b4b4;// ceiling
				if (k == 3) tex_pixels[j * 256 + i] = 0xffffffff;// blank
			}
		}
		TextureBuffer * generated_texture = new TextureBuffer(false, Sizei(256, 256), 4, (unsigned char *)tex_pixels);
		grid_material[k] = new ShaderFill(vshader, fshader, generated_texture);
	}

	//TextureBuffer * generated_texture = new TextureBuffer(false, "Assets/obj/j7-g01.png", 0);
	TextureBuffer * generated_texture = new TextureBuffer(false, "Assets/obj/testobj.png", 0);
	grid_material[4] = new ShaderFill(vshader, fshader, generated_texture);

	glDeleteShader(vshader);
	glDeleteShader(fshader);


	

	// Construct geometry
	Model * m = new Model(Vector3f(0, 0, 0), grid_material[2]);  // Moving box
	m->AddSolidColorBox(0, 0, 0, +1.0f, +1.0f, 1.0f, 0xff404040);
	m->AllocateBuffers();
	Add(m);

	m = new Model(Vector3f(0, 0, 0), grid_material[4]);  // Controller box
	m->AddSolidColorBox(0, 0, 0, +0.3f, +0.3f, 0.3f, GetColor(255,255,255) );
	m->AllocateBuffers();
	Add(m);

	m = new Model(Vector3f(0, 1.5f, 0), grid_material[4]);
	loadOBJ(*m, "Assets/obj/stestobj.obj");
	//loadOBJ(*m, "Assets/obj/j7-gn.obj");
	m->AllocateBuffers();
	Add(m);

	m = new Model(Vector3f(0, 0, 0), grid_material[1]);  // Walls
	m->AddSolidColorBox(-10.1f, 0.0f, -20.0f, -10.0f, 4.0f, 20.0f, 0xff808080); // Left Wall
	m->AddSolidColorBox(-10.0f, -0.1f, -20.1f, 10.0f, 4.0f, -20.0f, 0xff808080); // Back Wall
	m->AddSolidColorBox(10.0f, -0.1f, -20.0f, 10.1f, 4.0f, 20.0f, 0xff808080); // Right Wall
	m->AllocateBuffers();
	Add(m);

	if (includeIntensiveGPUobject)
	{
		m = new Model(Vector3f(0, 0, 0), grid_material[0]);  // Floors
		for (float depth = 0.0f; depth > -3.0f; depth -= 0.1f)
			m->AddSolidColorBox(9.0f, 0.5f, -depth, -9.0f, 3.5f, -depth, 0x10ff80ff); // Partition
		m->AllocateBuffers();
		Add(m);
	}

	m = new Model(Vector3f(0, 0, 0), grid_material[0]);  // Floors
	m->AddSolidColorBox(-10.0f, -0.1f, -20.0f, 10.0f, 0.0f, 20.1f, 0xff808080); // Main floor
	m->AddSolidColorBox(-15.0f, -6.1f, 18.0f, 15.0f, -6.0f, 30.0f, 0xff808080); // Bottom floor
	m->AllocateBuffers();
	Add(m);

	m = new Model(Vector3f(0, 0, 0), grid_material[2]);  // Ceiling
	m->AddSolidColorBox(-10.0f, 4.0f, -20.0f, 10.0f, 4.1f, 20.1f, 0xff808080);
	m->AllocateBuffers();
	Add(m);

	m = new Model(Vector3f(0, 0, 0), grid_material[3]);  // Fixtures & furniture
	m->AddSolidColorBox(9.5f, 0.75f, 3.0f, 10.1f, 2.5f, 3.1f, 0xff383838);   // Right side shelf// Verticals
	m->AddSolidColorBox(9.5f, 0.95f, 3.7f, 10.1f, 2.75f, 3.8f, 0xff383838);   // Right side shelf
	m->AddSolidColorBox(9.55f, 1.20f, 2.5f, 10.1f, 1.30f, 3.75f, 0xff383838); // Right side shelf// Horizontals
	m->AddSolidColorBox(9.55f, 2.00f, 3.05f, 10.1f, 2.10f, 4.2f, 0xff383838); // Right side shelf
	m->AddSolidColorBox(5.0f, 1.1f, 20.0f, 10.0f, 1.2f, 20.1f, 0xff383838);   // Right railing   
	m->AddSolidColorBox(-10.0f, 1.1f, 20.0f, -5.0f, 1.2f, 20.1f, 0xff383838);   // Left railing  
	for (float f = 5.0f; f <= 9.0f; f += 1.0f)
	{
		m->AddSolidColorBox(f, 0.0f, 20.0f, f + 0.1f, 1.1f, 20.1f, 0xff505050);// Left Bars
		m->AddSolidColorBox(-f, 1.1f, 20.0f, -f - 0.1f, 0.0f, 20.1f, 0xff505050);// Right Bars
	}
	m->AddSolidColorBox(-1.8f, 0.8f, 1.0f, 0.0f, 0.7f, 0.0f, 0xff505000); // Table
	m->AddSolidColorBox(-1.8f, 0.0f, 0.0f, -1.7f, 0.7f, 0.1f, 0xff505000); // Table Leg 
	m->AddSolidColorBox(-1.8f, 0.7f, 1.0f, -1.7f, 0.0f, 0.9f, 0xff505000); // Table Leg 
	m->AddSolidColorBox(0.0f, 0.0f, 1.0f, -0.1f, 0.7f, 0.9f, 0xff505000); // Table Leg 
	m->AddSolidColorBox(0.0f, 0.7f, 0.0f, -0.1f, 0.0f, 0.1f, 0xff505000); // Table Leg 
	m->AddSolidColorBox(-1.4f, 0.5f, -1.1f, -0.8f, 0.55f, -0.5f, 0xff202050); // Chair Set
	m->AddSolidColorBox(-1.4f, 0.0f, -1.1f, -1.34f, 1.0f, -1.04f, 0xff202050); // Chair Leg 1
	m->AddSolidColorBox(-1.4f, 0.5f, -0.5f, -1.34f, 0.0f, -0.56f, 0xff202050); // Chair Leg 2
	m->AddSolidColorBox(-0.8f, 0.0f, -0.5f, -0.86f, 0.5f, -0.56f, 0xff202050); // Chair Leg 2
	m->AddSolidColorBox(-0.8f, 1.0f, -1.1f, -0.86f, 0.0f, -1.04f, 0xff202050); // Chair Leg 2
	m->AddSolidColorBox(-1.4f, 0.97f, -1.05f, -0.8f, 0.92f, -1.10f, 0xff202050); // Chair Back high bar

	for (float f = 3.0f; f <= 6.6f; f += 0.4f)
		m->AddSolidColorBox(-3, 0.0f, f, -2.9f, 1.3f, f + 0.1f, 0xff404040); // Posts

	m->AllocateBuffers();
	Add(m);
}

void Scene::Release()
{
	while (numModels-- > 0)
		delete Models[numModels];
}

Model &Scene::getModelByIndex(int index)
{
	return *Models[index];
}
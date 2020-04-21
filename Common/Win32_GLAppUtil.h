/************************************************************************************
 Filename    :   Win32_GLAppUtil.h
 Content     :   OpenGL and Application/Window setup functionality for RoomTiny
 Created     :   October 20th, 2014
 Author      :   Tom Heath
 Copyright   :   Copyright 2014 Oculus, LLC. All Rights reserved.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 *************************************************************************************/

#ifndef OVR_Win32_GLAppUtil_h
#define OVR_Win32_GLAppUtil_h

#include "GL/CAPI_GLE.h"
#include "Extras/OVR_Math.h"
#include "OVR_CAPI_GL.h"
#include <assert.h>
#include <vector>
#include "../texturer.h"

using namespace OVR;

#ifndef VALIDATE
    #define VALIDATE(x, msg) if (!(x)) { MessageBoxA(NULL, (msg), "OculusRoomTiny", MB_ICONERROR | MB_OK); exit(-1); }
#endif

#ifndef OVR_DEBUG_LOG
    #define OVR_DEBUG_LOG(x)
#endif


//-------------------------------------------------------------------------------------------
struct OGL
{
    static const bool       UseDebugContext = false;

    HWND                    Window;
    HDC                     hDC;
    HGLRC                   WglContext;
    OVR::GLEContext         GLEContext;
    bool                    Running;
    bool                    Key[256];
    int                     WinSizeW;
    int                     WinSizeH;
    GLuint                  fboId;
    HINSTANCE               hInstance;

    static LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        OGL *p = reinterpret_cast<OGL *>(GetWindowLongPtr(hWnd, 0));
        switch (Msg)
        {
        case WM_KEYDOWN:
            p->Key[wParam] = true;
            break;
        case WM_KEYUP:
            p->Key[wParam] = false;
            break;
        case WM_DESTROY:
            p->Running = false;
            break;
        default:
            return DefWindowProcW(hWnd, Msg, wParam, lParam);
        }
        if ((p->Key['Q'] && p->Key[VK_CONTROL]) || p->Key[VK_ESCAPE])
        {
            p->Running = false;
        }
        return 0;
    }

    OGL() :
        Window(nullptr),
        hDC(nullptr),
        WglContext(nullptr),
        GLEContext(),
        Running(false),
        WinSizeW(0),
        WinSizeH(0),
        fboId(0),
        hInstance(nullptr)
    {
		// Clear input
		for (int i = 0; i < sizeof(Key) / sizeof(Key[0]); ++i)
            Key[i] = false;
    }

    ~OGL()
    {
        ReleaseDevice();
        CloseWindow();
    }

    bool InitWindow(HINSTANCE hInst, LPCWSTR title)
    {
        hInstance = hInst;
        Running = true;

        WNDCLASSW wc;
        memset(&wc, 0, sizeof(wc));
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = WindowProc;
        wc.cbWndExtra = sizeof(struct OGL *);
        wc.hInstance = GetModuleHandleW(NULL);
        wc.lpszClassName = L"ORT";
        RegisterClassW(&wc);

        // adjust the window size and show at InitDevice time
        Window = CreateWindowW(wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, 0, 0, hInstance, 0);
        if (!Window) return false;

        SetWindowLongPtr(Window, 0, LONG_PTR(this));

        hDC = GetDC(Window);

        return true;
    }

    void CloseWindow()
    {
        if (Window)
        {
            if (hDC)
            {
                ReleaseDC(Window, hDC);
                hDC = nullptr;
            }
            DestroyWindow(Window);
            Window = nullptr;
            UnregisterClassW(L"OGL", hInstance);
        }
    }

    // Note: currently there is no way to get GL to use the passed pLuid
    bool InitDevice(int vpW, int vpH, const LUID* /*pLuid*/, bool windowed = true)
    {
        UNREFERENCED_PARAMETER(windowed);

        WinSizeW = vpW;
        WinSizeH = vpH;

        RECT size = { 0, 0, vpW, vpH };
        AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, false);
        const UINT flags = SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW;
        if (!SetWindowPos(Window, nullptr, 0, 0, size.right - size.left, size.bottom - size.top, flags))
            return false;

        PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARBFunc = nullptr;
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARBFunc = nullptr;
        {
            // First create a context for the purpose of getting access to wglChoosePixelFormatARB / wglCreateContextAttribsARB.
            PIXELFORMATDESCRIPTOR pfd;
            memset(&pfd, 0, sizeof(pfd));
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
            pfd.cColorBits = 32;
            pfd.cDepthBits = 16;
            int pf = ChoosePixelFormat(hDC, &pfd);
            VALIDATE(pf, "Failed to choose pixel format.");

            VALIDATE(SetPixelFormat(hDC, pf, &pfd), "Failed to set pixel format.");

            HGLRC context = wglCreateContext(hDC);
            VALIDATE(context, "wglCreateContextfailed.");
            VALIDATE(wglMakeCurrent(hDC, context), "wglMakeCurrent failed.");

            wglChoosePixelFormatARBFunc = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARBFunc = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
            assert(wglChoosePixelFormatARBFunc && wglCreateContextAttribsARBFunc);

            wglDeleteContext(context);
        }

        // Now create the real context that we will be using.
        int iAttributes[] =
        {
            // WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_COLOR_BITS_ARB, 32,
            WGL_DEPTH_BITS_ARB, 16,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0, 0
        };

        float fAttributes[] = { 0, 0 };
        int   pf = 0;
        UINT  numFormats = 0;

        VALIDATE(wglChoosePixelFormatARBFunc(hDC, iAttributes, fAttributes, 1, &pf, &numFormats),
            "wglChoosePixelFormatARBFunc failed.");

        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(pfd));
        VALIDATE(SetPixelFormat(hDC, pf, &pfd), "SetPixelFormat failed.");

        GLint attribs[16];
        int   attribCount = 0;
        if (UseDebugContext)
        {
            attribs[attribCount++] = WGL_CONTEXT_FLAGS_ARB;
            attribs[attribCount++] = WGL_CONTEXT_DEBUG_BIT_ARB;
        }

        attribs[attribCount] = 0;

        WglContext = wglCreateContextAttribsARBFunc(hDC, 0, attribs);
        VALIDATE(wglMakeCurrent(hDC, WglContext), "wglMakeCurrent failed.");

        OVR::GLEContext::SetCurrentContext(&GLEContext);
        GLEContext.Init();

        glGenFramebuffers(1, &fboId);

        glEnable(GL_DEPTH_TEST);
        glFrontFace(GL_CW);
        glEnable(GL_CULL_FACE);

        if (UseDebugContext && GLE_ARB_debug_output)
        {
            glDebugMessageCallbackARB(DebugGLCallback, NULL);
            if (glGetError())
            {
                OVR_DEBUG_LOG(("glDebugMessageCallbackARB failed."));
            }

            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

            // Explicitly disable notification severity output.
            glDebugMessageControlARB(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
        }

        return true;
    }

    bool HandleMessages(void)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return Running;
    }

    void Run(bool (*MainLoop)(bool retryCreate))
    {
        while (HandleMessages())
        {
            // true => we'll attempt to retry for ovrError_DisplayLost
            if (!MainLoop(true))
                break;
            // Sleep a bit before retrying to reduce CPU load while the HMD is disconnected
            Sleep(10);
        }
    }

    void ReleaseDevice()
    {
        if (fboId)
        {
            glDeleteFramebuffers(1, &fboId);
            fboId = 0;
        }
        if (WglContext)
        {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(WglContext);
            WglContext = nullptr;
        }
        GLEContext.Shutdown();
    }

    static void GLAPIENTRY DebugGLCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
    {
        UNREFERENCED_PARAMETER(source);
        UNREFERENCED_PARAMETER(type);
        UNREFERENCED_PARAMETER(id);
        UNREFERENCED_PARAMETER(severity);
        UNREFERENCED_PARAMETER(length);
        UNREFERENCED_PARAMETER(message);
        UNREFERENCED_PARAMETER(userParam);
        OVR_DEBUG_LOG(("Message from OpenGL: %s\n", message));
    }
};

// Global OpenGL state
static OGL Platform;

//------------------------------------------------------------------------------
struct ShaderFill
{
    GLuint            program;
    TextureBuffer   * texture;

    ShaderFill(GLuint vertexShader, GLuint pixelShader, TextureBuffer* _texture)
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

    ~ShaderFill()
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

/*
DWORD GetColor(int Red, int Green, int Blue, int Alpha=255)
{
	return ((Red & 0xff) << 24) + ((Green & 0xff) << 16) + ((Blue & 0xff) << 8) + (Alpha & 0xff);
}
*/

//---------------------------------------------------------------------------
struct Vertex
{
	Vector3f  Pos;
	DWORD     C;
	float     U, V;
	//Vector3f  Norm;
};

struct Model
{
    Vector3f        Pos;
    Quatf           Rot;
    Matrix4f        Mat;
    int             numVertices, numIndices;
    std::vector<Vertex>          Vertices;
    std::vector<GLushort>        Indices;
    ShaderFill    * Fill;
    VertexBuffer  * vertexBuffer;
    IndexBuffer   * indexBuffer;

    Model(Vector3f pos, ShaderFill * fill) :
        numVertices(0),
        numIndices(0),
        Pos(pos),
        Rot(),
        Mat(),
        Fill(fill),
        vertexBuffer(nullptr),
        indexBuffer(nullptr)
    {}
    ~Model() { FreeBuffers(); }

    Matrix4f& GetMatrix()
    {
        Mat = Matrix4f(Rot);
        Mat = Matrix4f::Translation(Pos) * Mat; // à íuçsóÒÇòâÒì]çsóÒ
        return Mat;
    }

	void AddVertex(const Vertex& v)
	{
		Vertices.push_back(v); 
		numVertices++; 
	}
    void AddIndex(GLushort a)
	{
		Indices.push_back(a);
		numIndices++;
	}

    void AllocateBuffers()
    {
        vertexBuffer = new VertexBuffer(&Vertices[0], numVertices * sizeof(Vertices[0]));
        indexBuffer = new IndexBuffer(&Indices[0], numIndices * sizeof(Indices[0]));
    }

    void FreeBuffers()
    {
        delete vertexBuffer; vertexBuffer = nullptr;
        delete indexBuffer; indexBuffer = nullptr;
    }

    void AddSolidColorBox(float x1, float y1, float z1, float x2, float y2, float z2, DWORD c)
    {
        Vector3f Vert[][2] =
        {
            Vector3f(x1, y2, z1), Vector3f(z1, x1), Vector3f(x2, y2, z1), Vector3f(z1, x2),
            Vector3f(x2, y2, z2), Vector3f(z2, x2), Vector3f(x1, y2, z2), Vector3f(z2, x1),
            Vector3f(x1, y1, z1), Vector3f(z1, x1), Vector3f(x2, y1, z1), Vector3f(z1, x2),
            Vector3f(x2, y1, z2), Vector3f(z2, x2), Vector3f(x1, y1, z2), Vector3f(z2, x1),
            Vector3f(x1, y1, z2), Vector3f(z2, y1), Vector3f(x1, y1, z1), Vector3f(z1, y1),
            Vector3f(x1, y2, z1), Vector3f(z1, y2), Vector3f(x1, y2, z2), Vector3f(z2, y2),
            Vector3f(x2, y1, z2), Vector3f(z2, y1), Vector3f(x2, y1, z1), Vector3f(z1, y1),
            Vector3f(x2, y2, z1), Vector3f(z1, y2), Vector3f(x2, y2, z2), Vector3f(z2, y2),
            Vector3f(x1, y1, z1), Vector3f(x1, y1), Vector3f(x2, y1, z1), Vector3f(x2, y1),
            Vector3f(x2, y2, z1), Vector3f(x2, y2), Vector3f(x1, y2, z1), Vector3f(x1, y2),
            Vector3f(x1, y1, z2), Vector3f(x1, y1), Vector3f(x2, y1, z2), Vector3f(x2, y1),
            Vector3f(x2, y2, z2), Vector3f(x2, y2), Vector3f(x1, y2, z2), Vector3f(x1, y2)
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
            float G = ((c >>  8) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
            float R = ((c >>  0) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
            vvv.C = (c & 0xff000000) +
                ((R > 255 ? 255 : DWORD(R)) << 16) +
                ((G > 255 ? 255 : DWORD(G)) << 8) +
                 (B > 255 ? 255 : DWORD(B));
            AddVertex(vvv);
        }
    }

    void Render(Matrix4f view, Matrix4f proj)
    {
        Matrix4f combined = proj * view * GetMatrix();

        glUseProgram(Fill->program);
        glUniform1i(glGetUniformLocation(Fill->program, "Texture0"), 0);
        glUniformMatrix4fv(glGetUniformLocation(Fill->program, "matWVP"), 1, GL_TRUE, (FLOAT*)&combined);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Fill->texture->texId);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->buffer);

        GLuint posLoc = glGetAttribLocation(Fill->program, "Position");
        GLuint colorLoc = glGetAttribLocation(Fill->program, "Color");
        GLuint uvLoc = glGetAttribLocation(Fill->program, "TexCoord");

        glEnableVertexAttribArray(posLoc);
        glEnableVertexAttribArray(colorLoc);
        glEnableVertexAttribArray(uvLoc);

        glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, Pos));
        glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, C));
        glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)OVR_OFFSETOF(Vertex, U));

        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, NULL);

        glDisableVertexAttribArray(posLoc);
        glDisableVertexAttribArray(colorLoc);
        glDisableVertexAttribArray(uvLoc);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glUseProgram(0);
    }
};

#endif // OVR_Win32_GLAppUtil_h

#include "renderer.h"

#include "parse.h"

#include <imgui.h>
#include "gui/imgui_impl_glfw.h"
#include "gui/imgui_impl_opengl3.h"

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

inline static void _callback_key(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		default:
			break;
		}
	}
}

const uint32_t auxiliaryWidth = 250;

Renderer::Renderer(const std::string& filename)
{
	Parse(filename, *this);
	std::cout << "Parse Done\n";
}

void Renderer::InitializeGUI()
{	
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        assert(false);
        exit(-1);
    }

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(
        static_cast<int>(m_buffer->m_width + auxiliaryWidth), static_cast<int>(std::max(500, m_buffer->m_height)),
        "Renderer", NULL, NULL);
    if (m_window == NULL) {
        assert(false);
        exit(-1);
    }
        
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        assert(false);
        exit(-1);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
}

void Renderer::Draw()
{
    uint32_t w = m_buffer->m_width, h = m_buffer->m_height;
    sRGB* sRGBBuffer = m_buffer->GetsRGBBuffer();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawPixels(
        static_cast<GLsizei>(w), static_cast<GLsizei>(h),
        GL_RGB, GL_FLOAT, sRGBBuffer);
}

void Renderer::Render()
{    
    ImVec4 clear_color = ImVec4(0.62f, 0.87f, 1.00f, 1.00f);

    // Initialize GUI
	InitializeGUI();
	//Start rendering
	m_integrator->Start();

	//Display loop for the ongoing or completed render
	glfwSetWindowTitle(m_window, "Rendering...");
    bool debugRay = false, debugKDTree = false;
	while (!glfwWindowShouldClose(m_window)) {
        // Initialize window color
        //glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();        

        // Auxiliary windows
        bool clearDebugBuffer = false;
        {
            ImGui::SetNextWindowPos(ImVec2(m_buffer->m_width, 0));
            ImGui::SetNextWindowSize(ImVec2(auxiliaryWidth, 200));
            ImGui::Begin("Overview");
            std::string integratorString = m_integrator->ToString();
            std::string sceneString = m_scene->ToString();
            ImGui::Text((integratorString + "\n\n" + sceneString).c_str());
            ImGui::End();
        }
        {
            ImGui::SetNextWindowPos(ImVec2(m_buffer->m_width, 200));
            ImGui::SetNextWindowSize(ImVec2(auxiliaryWidth, 200));
            ImGui::Begin("Debug");
            int px, py;
            ImGuiIO& io = ImGui::GetIO();
            if (ImGui::IsMousePosValid()) {
                px = io.MousePos.x;
                py = io.MousePos.y;
            }
            ImGui::Checkbox(fmt::format("Debug ray : ({0}, {1})", px, py).c_str(), &debugRay);
            ImGui::Checkbox(fmt::format("Debug kd-tree").c_str(), &debugKDTree);
            clearDebugBuffer = ImGui::Button("Clear debug buffer");
            ImGui::End();
        }
        {
            //ImGui::ShowDemoWindow();
        }

        // Renderer core
        Draw();
        // Render done
        if (!m_integrator->IsRendering()) {
            m_integrator->Stop();
            m_integrator->Wait();
            std::string title("Render done in ");
            title += m_integrator->m_timer.ToString();
            glfwSetWindowTitle(m_window, title.c_str());

            // Debug
            DebugRecord debugRec;
            if (debugRay) {
                ImGuiIO& io = ImGui::GetIO();
                if (ImGui::IsMousePosValid()) {
                    if (ImGui::IsMouseClicked(0)) {
                        debugRec.SetDebugRay(Float2(io.MousePos.x, io.MousePos.y));
                    }
                }
            }            
            if (debugKDTree) {
                debugRec.m_debugKDTree = true;
            }
            if (clearDebugBuffer) {
                m_buffer->ClearDebugBuffer();
            }
            m_integrator->Debug(debugRec);
        }

        // Rendering
        ImGui::Render();
        //int display_w, display_h;
        //glfwGetFramebufferSize(window, &display_w, &display_h);
        //glViewport(0, 0, display_w, display_h);        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(m_window);        
	}

	//Stop the renderer if it hasn't been already.
	m_integrator->Stop();
	m_integrator->Wait();
	glfwSetWindowTitle(m_window, "Render done");

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

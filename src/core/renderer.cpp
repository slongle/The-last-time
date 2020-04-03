#include "renderer.h"

#include "parse.h"

#include <GLFW/glfw3.h>

#ifdef _DEBUG
inline static void _callback_err_glfw(int /*error*/, char const* description) {
	fprintf(stderr, "Error: %s\n", description);
}
#endif

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

Renderer::Renderer(const std::string& filename)
{
	Parse(filename, *this);
	std::cout << "Parse Done\n";
}

void Renderer::InitializeGUI()
{	
	{
#ifdef _DEBUG
		glfwSetErrorCallback(_callback_err_glfw);
#endif

		glfwInit();

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#ifdef _DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

#ifdef WITH_TRANSPARENT_FRAMEBUFFER
		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
#endif

		m_window = glfwCreateWindow(
			static_cast<int>(m_buffer->m_width), static_cast<int>(m_buffer->m_height),
			"Renderer",
			nullptr,
			nullptr
		);

		glfwSetKeyCallback(m_window, _callback_key);

		glfwMakeContextCurrent(m_window);
#ifdef _DEBUG
		//glDebugMessageCallback(_callback_err_gl, nullptr);
#endif

//glfwSwapInterval(0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void Renderer::Render()
{
	InitializeGUI();
	//Start rendering
	m_integrator->Start();

	//Display loop for the ongoing or completed render
	glfwSetWindowTitle(m_window, "Rendering...");
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();

		Draw();
		if (!m_integrator->IsRendering()) {
			std::string title("Render done in ");
			title += m_integrator->m_timer.ToString();
			glfwSetWindowTitle(m_window, title.c_str());
		}

		glfwSwapBuffers(m_window);
	}

	//Stop the renderer if it hasn't been already.
	m_integrator->Stop();
	m_integrator->Wait();
	glfwSetWindowTitle(m_window, "Render done");

	//Clean up
	glfwDestroyWindow(m_window);

	glfwTerminate();
}

#include "glpch.h"
#include "Application.h"

#include "Log.h"

#include "Input.h"

#include <glfw/glfw3.h>

namespace GLCore {

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	GLuint Application::s_QuadVA = 0;
	GLuint Application::s_QuadVB = 0;
	GLuint Application::s_QuadIB = 0;

	Application::Application(const std::string& name, uint32_t width, uint32_t height)
	{
		if (!s_Instance)
		{
			// Initialize core
			Log::Init();
		}

		GLCORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create({ name, width, height }));
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);

		// Default quad
		{
			glGenVertexArrays(1, &s_QuadVA);
			glBindVertexArray(s_QuadVA);

			float vertices[] = {
				-1.0f, -1.0f,
				 1.0f, -1.0f,
				 1.0f,  1.0f,
				-1.0f,  1.0f
			};

			glGenBuffers(1, &s_QuadVB);
			glBindBuffer(GL_ARRAY_BUFFER, s_QuadVB);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

			uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
			glGenBuffers(1, &s_QuadIB);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_QuadIB);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::Run()
	{
		while (m_Running)
		{
			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerStack)
					layer->OnUpdate(timestep);

				m_ImGuiLayer->Begin();
				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
				m_ImGuiLayer->End();

			}

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		// Clean default quad
		glDeleteVertexArrays(1, &s_QuadVA);
		glDeleteBuffers(1, &s_QuadVB);
		glDeleteBuffers(1, &s_QuadIB);

		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		return false;
	}

}
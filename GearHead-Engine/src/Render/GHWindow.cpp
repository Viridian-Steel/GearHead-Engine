#include "ghpch.hpp"
#include "GHWindow.hpp"
#include "GearHead/Events/ApplicationEvent.hpp"

namespace GearHead{

	GHWindow::GHWindow(unsigned int Width, unsigned int Height, bool vsync, std::string Title){
		m_Data.height		= Height;
		m_Data.width		= Width;
		m_Data.windowName	= Title;
		m_Data.VSync		= vsync;
		init();
	}

	GHWindow::~GHWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void GHWindow::OnUpdate()
	{
		glfwPollEvents();
	}


	void GHWindow::init() {

		if (!s_glfwInitialized) {
			int sucess = glfwInit();
			GEARHEAD_ASSERT(sucess, "Could Not initialized GLFW");
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(m_Data.width, m_Data.height, m_Data.windowName.c_str(), nullptr, nullptr);
		glfwMakeContextCurrent(window);
		glfwSetWindowUserPointer(window, &m_Data);
		WindowResizeEvent e(m_Data.width, m_Data.height);
		GEARHEAD_CORE_INFO("Created Window {0} {1} x {2} Vsync {3}", m_Data.windowName, m_Data.width, m_Data.height, m_Data.VSync);

	}

}

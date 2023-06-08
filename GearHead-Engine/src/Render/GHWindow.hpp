#pragma once
#include "ghpch.hpp"
#include "GearHead/Core.hpp"
#include "GearHead/Events/Event.hpp"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"



namespace GearHead{

	class GEARHEAD_API GHWindow {
	public:
		GHWindow(unsigned int Width, unsigned int Height, bool vsync, std::string Title = "Gearhead_Engine");
		~GHWindow();

		//NOT COPYABLE. Don't need mulitple windows
		GHWindow(const GHWindow&)				= delete;
		GHWindow& operator=(const GHWindow&)	= delete;

		GLFWwindow* GetWindow() { return window; }
		std::string getName() { return m_Data.windowName; }

		void OnUpdate();
		bool IsVSync() { return m_Data.VSync; }

		bool ShouldClose() { return !glfwWindowShouldClose(window); }

		using EventCallbackFn					= std::function<void(Event&)>;

	private:
		void init();

		bool framebufferResized = false;
		bool s_glfwInitialized = false;
		GLFWwindow* window;

		struct WindowData
		{
			std::string windowName;
			unsigned int width, height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;

	};
}

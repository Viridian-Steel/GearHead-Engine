#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VkTypes.hpp"
#include <GearHead/Window.hpp>

namespace GearHead {

	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void push_function(std::function<void()>&& function) {
			deletors.push_back(function);
		}

		void flush() {
			// reverse iterate the deletion queue to execute all the functions
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
				(*it)(); //call the function
			}

			deletors.clear();
		}
	};

	class GEARHEAD_API VkWindow : public Window {
	public:

		VkWindow(const WindowProps& props);

		virtual ~VkWindow();

		void OnUpdate() override;

		void DrawFrame() override;

		int ShouldClose() override { return !glfwWindowShouldClose(_window); } 
		// yes I know it's the wrong operator, but it matters not for right here

		inline unsigned int GetWidth() const override { return mData.props.Width; }
		inline unsigned int GetHeight() const override { return mData.props.Height; }

		inline void SetEventCallback(const EventCallbackFn& callback) override { mData.EventCallback = callback; }
		void SetVSync(bool enabled) override;


		bool IsVSync() const override { return mData.vsync; }

	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

		//Vulkan Inits
		void InitVulkan();
		void InitSwapchain();
		void InitCommands();
		void InitDefaultRenderPass();
		void InitFrameBuffers();
		void InitSyncStructures();
		void InitPipelines();

		//Shader loading
		bool loadShaderModule(const char* filePath, VkShaderModule* outShaderModule);

	private:
		struct WindowData {
			WindowProps props;
			bool vsync;
			EventCallbackFn EventCallback;

			VkExtent2D toVkExtent2D() {
				VkExtent2D e{};
				e.height = props.Height;
				e.width = props.Width;
				return e;
			}
		};

		GLFWwindow* _window;

		WindowData mData;
		DeletionQueue _mainDeletionQueue;

		//Vulkan Stuff
		
		//eh
		int _frameNumber{ 0 };

		//Device 
		VkInstance _instance;
		VkDebugUtilsMessengerEXT _debugMessenger;
		VkPhysicalDevice _chosenGPU;
		VkDevice _device;
		VkSurfaceKHR _surface;

		//Swapchain 
		VkSwapchainKHR _swapchain;
		VkFormat _swapchainImageFormat;
		std::vector<VkImage> _swapchainImages;
		std::vector<VkImageView> _swapchainImageViews;
		int _selectedShader{ 0 };
		//Commands
		VkQueue _graphicsQueue;
		uint32_t _graphicsQueueFamily;
		VkCommandPool _commandPool;
		VkCommandBuffer _mainCommandBuffer;

		//Render Pass
		VkRenderPass _renderPass;
		std::vector<VkFramebuffer> _frameBuffers;

		//Semaphores
		VkSemaphore _presentSemaphore, _renderSemaphore;
		VkFence _renderFence;

		//Pipline Layouts
		VkPipelineLayout _trianglePipelineLayout;

		//Pipelines
		VkPipeline _colorTrianglePipeline;
		VkPipeline _trianglePipeline;

	};
}

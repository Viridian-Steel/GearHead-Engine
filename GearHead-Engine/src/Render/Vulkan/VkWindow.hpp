#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "VkTypes.hpp"
#include <Core/Window.hpp>

#include "VkDescriptors.hpp"

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

	struct ComputePushConstants {
		glm::vec4 data1;
		glm::vec4 data2;
		glm::vec4 data3;
		glm::vec4 data4;
	};

	struct ComputeEffect {
		const char* name;

		VkPipeline pipeline;
		VkPipelineLayout layout;

		ComputePushConstants data;
	};


	struct FrameData {
		VkCommandPool _pool;
		VkCommandBuffer _buffer;
		VkSemaphore _SwapChainSemaphore, _renderSemaphore;
		VkFence _renderFence;
		DeletionQueue _deletionQueue;
	};




	constexpr unsigned int FRAME_OVERLAP = 2U;

	class GEARHEAD_API VkWindow : public Window {
	public:

		VkWindow(const WindowProps& props);

		virtual ~VkWindow();

		void OnUpdate() override;

		void DrawFrame() override;

		int ShouldClose() override { return !glfwWindowShouldClose(_window); } 
		

		inline unsigned int GetWidth() const override { return mData.props.Width; }
		inline unsigned int GetHeight() const override { return mData.props.Height; }
		void SetVSync(bool enabled) override;


		bool IsVSync() const override { return mData.vsync; }

	private:
// Methods
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();

		//Vulkan Inits
		void InitVulkan();
		void InitSwapchain();
		void InitCommands();
		void InitSyncStructures();
		void InitDescriptors();
		void InitPipelines();
		void InitBackgroundPipelines();
		void InitImGUI();


		//Draw Calls
		void DrawBackground(VkCommandBuffer cmd);
		void DrawImGUI(VkCommandBuffer cmd, VkImageView targetImageView) const;


		//Swapchain madness
		void CreateSwapChain(uint32_t width, uint32_t height);
		void DestroySwapChain();
		void RebuildSwapChain();

		//Immediate Sumbit
		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;

// Variables
		struct WindowData {
			WindowProps props;
			bool vsync;

			VkExtent2D toVkExtent2D() const {
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
		VkExtent2D _swapchainExtent;
		
		//Commands
		VkQueue _graphicsQueue;
		uint32_t _graphicsQueueFamily;
		FrameData _frames[FRAME_OVERLAP];

		FrameData& GetCurrentFrame() { return _frames[_frameNumber % FRAME_OVERLAP]; }	

		//Render Pass
		VkRenderPass _renderPass;
		std::vector<VkFramebuffer> _frameBuffers;

		//Descriptor Sets
		DescriptorAllocator GlobalDescriptorAllocator;

		VkDescriptorSet _drawImageDescriptors;
		VkDescriptorSetLayout _drawImageDescriptorLayout;

		//VMA Allocator
		VmaAllocator _allocator;

		AllocatedImage _drawImage;
		VkExtent2D _drawExtent;		

		//Pipelines
		VkPipeline _gradientPipeline;
		VkPipelineLayout _gradientPipelineLayout;

		//ImGUI
		VkFence _immFence;
		VkCommandBuffer _immCommandBuffer;
		VkCommandPool _immCommandPool;

		//ImGUI Editable Prarmeters
		std::vector<ComputeEffect> backgroundEffects;
		int currentBackgroundEffect{ 0 };


	};
}

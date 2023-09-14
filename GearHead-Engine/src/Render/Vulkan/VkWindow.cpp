#include "ghpch.hpp"

#include "VkInit.hpp"
#include "VkWindow.hpp"
#include "VkPipeline.hpp"


//bootstrap library
#include "VkBootstrap.h"													



namespace GearHead {

	static bool s_GLFWInitialized = false;
	static bool s_VKInitialized = false;

	Window* Window::Create(const WindowProps& props) { return new VkWindow(props); }


	VkWindow::VkWindow(const WindowProps& props) { Init(props); }

	void VkWindow::Init(const WindowProps& props){
		mData.props = props;	


		GEARHEAD_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInitialized) {
			GEARHEAD_CORE_ASSERT_FUNC(glfwInit(), "Failed to Initialize GLFW");
			s_GLFWInitialized = true;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		_window = glfwCreateWindow(mData.props.Width, mData.props.Height, mData.props.Title.c_str(), nullptr, nullptr);
		
		glfwMakeContextCurrent(_window);
		glfwSetWindowUserPointer(_window, &mData);
		SetVSync(true);

		//Vulkan

		InitVulkan();
		InitSwapchain();
		InitCommands();
		InitDefaultRenderPass();
		InitFrameBuffers();
		InitSyncStructures();
		InitPipelines();

		s_VKInitialized = true;

	}

	VkWindow::~VkWindow() { Shutdown(); }

	void VkWindow::Shutdown()
	{
		//	Order of Destruction
		//      Command Pool    
		//	     SwapChain    
		//		FrameBuffers <-- perhaps wrong
		//		 RenderPass  <-- perhaps wrong
		//		  Device      
		//		  Surface      
		//		 Debugger
		//		 Instance   
		//	   Window(GLFW)


		if (s_VKInitialized) {

			// make sure the GPU has stopped doing its things
			vkWaitForFences(_device, 1, &_renderFence, true, 1000000000);

			_mainDeletionQueue.flush();

			vkDestroyDevice(_device, nullptr);
			vkDestroySurfaceKHR(_instance, _surface, nullptr);
			vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
			vkDestroyInstance(_instance, nullptr);
		}

		if (s_GLFWInitialized)
		{
			glfwDestroyWindow(_window);
		}

	}

	void VkWindow::InitVulkan()
	{
		vkb::InstanceBuilder builder;

		//make instance
		auto instRet = builder.set_app_name(mData.props.Title.c_str())
			.set_engine_name("GearHead-Engine")
			.request_validation_layers(true)
			.require_api_version(1,2,0)
			.set_debug_callback( // add custom debug callback. need to store the logs
				[] (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
					VkDebugUtilsMessageTypeFlagsEXT messageType,
					const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
					void* pUserData)
				-> VkBool32 {
					auto severity = vkb::to_string_message_severity(messageSeverity);
					auto type = vkb::to_string_message_type(messageType);
					GEARHEAD_CORE_WARN("[{0} {1}] {2}", severity, type, pCallbackData->pMessage);
					return VK_FALSE;
				}
			)
			.build();

		vkb::Instance vkbInstance = instRet.value();
		//store the instance
		_instance = vkbInstance.instance;

		//store the debug messenger
		_debugMessenger = vkbInstance.debug_messenger;

		if (_instance == VK_NULL_HANDLE) {
			GEARHEAD_CORE_CRITICAL("Instance not assigned");
		}

		VkResult res = glfwCreateWindowSurface(_instance, _window, nullptr, &_surface);

		if (res != VK_SUCCESS) {
			GEARHEAD_CORE_ERROR("Failed to create window surface. Error: {0}", static_cast<int>(res));
			
			abort();
		}

		//if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) { 
		//	GEARHEAD_CORE_ERROR("Failed to create window surface"); 
		//	abort(); 
		//}

		vkb::PhysicalDeviceSelector selector{ vkbInstance };

		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 2)
			.set_surface(_surface)
			.select()
			.value();

		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		vkb::Device vkbDevice = deviceBuilder.build().value();

		_device = vkbDevice.device;
		_chosenGPU = physicalDevice.physical_device;

		//grabbing the graphics queue form the devices
		_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		GEARHEAD_CORE_INFO("Using GPU: {0}", physicalDevice.name);
	}

	void VkWindow::InitSwapchain()
	{
		vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};
		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.use_default_format_selection()
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR) //I don't care what the thing says, mailbox is best
			.set_desired_extent(mData.props.Width, mData.props.Height)
			.build()
			.value();

		_swapchain = vkbSwapchain.swapchain;
		_swapchainImages = vkbSwapchain.get_images().value();
		_swapchainImageViews = vkbSwapchain.get_image_views().value();

		_swapchainImageFormat = vkbSwapchain.image_format;

		_mainDeletionQueue.push_function([=]() {
			vkDestroySwapchainKHR(_device, _swapchain, nullptr);
			}
		);


	}

	void VkWindow::InitCommands() {

		//Create the Command Pool

		VkCommandPoolCreateInfo commandPoolInfo = VkInit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		if (vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to create command pool!"); abort(); }

		//Create the Command Buffer

		VkCommandBufferAllocateInfo cmdAllocInfo = VkInit::command_buffer_allocate_info(_commandPool, 1);

		if (vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_mainCommandBuffer) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to create command buffer!"); abort(); }

		_mainDeletionQueue.push_function([=]() {
			vkDestroyCommandPool(_device, _commandPool, nullptr);
			}
		);

	}

	void VkWindow::InitDefaultRenderPass()
	{
		//renderpass uses this color attachment
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format	= _swapchainImageFormat;
		//No MSAA
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		//clear when loaded
		colorAttachment.loadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;
		//store when the renderpass ends
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		//we don't care about stencil
		colorAttachment.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE; 

		//we don't know or care about the starting layout of the attachment
		colorAttachment.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;

		//after the renderpass ends, the image has to be on a layout ready for display
		colorAttachment.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


		VkAttachmentReference colorAttachmentRef = {};
		//attachment number will index into the pAttachments array in the parent renderpass itself
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;


		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

		//connect the color attachment to the info
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		//connect the subpass to the info
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;


		if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to create renderpass"); abort(); };

		_mainDeletionQueue.push_function([=]() {
			vkDestroyRenderPass(_device, _renderPass, nullptr);
			}
		);

	}

	void VkWindow::InitFrameBuffers()
	{
		//create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
		VkFramebufferCreateInfo fb_info = {};
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.pNext = nullptr;

		fb_info.renderPass = _renderPass;
		fb_info.attachmentCount = 1;
		fb_info.width = mData.props.Width;
		fb_info.height = mData.props.Height;
		fb_info.layers = 1;

		//grab how many images we have in the swapchain
		const uint32_t swapchain_imagecount = _swapchainImages.size();
		_frameBuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

		//create framebuffers for each of the swapchain image views
		for (int i = 0; i < swapchain_imagecount; i++) {

			fb_info.pAttachments = &_swapchainImageViews[i];
			if (vkCreateFramebuffer(_device, &fb_info, nullptr, &_frameBuffers[i]) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to create frame buffer {0}", i); abort(); };

			_mainDeletionQueue.push_function([=]() {
				vkDestroyFramebuffer(_device, _frameBuffers[i], nullptr);
				vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
				}
			);

		}


	}

	void VkWindow::InitSyncStructures()
	{
		VkFenceCreateInfo fenceCreateInfo = VkInit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

		if (vkCreateFence(_device, &fenceCreateInfo, nullptr, &_renderFence) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to create fence"); };

		_mainDeletionQueue.push_function([=]() {
			vkDestroyFence(_device, _renderFence, nullptr);
			}
		);

		VkSemaphoreCreateInfo semaphoreCreateInfo = VkInit::semaphore_create_info();

		if (vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_presentSemaphore) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to create present semaphore"); }
		if (vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderSemaphore) != VK_SUCCESS)  { GEARHEAD_CORE_ERROR("Failed to create render semaphore");  }

		_mainDeletionQueue.push_function([=]() {
			vkDestroySemaphore(_device, _presentSemaphore, nullptr);
			vkDestroySemaphore(_device, _renderSemaphore, nullptr);
			}
		);
	}

	void VkWindow::InitPipelines()
	{
		VkShaderModule colortriangleVertShader;
		GEARHEAD_CORE_ASSERT_FUNC(loadShaderModule("./Shaders/coloured_triangle.vert.spv", &colortriangleVertShader), "Error when loading VERT shader");
		GEARHEAD_CORE_INFO("Triangle vertex shader successfully loaded");

		VkShaderModule colortriangleFragShader;
		GEARHEAD_CORE_ASSERT_FUNC(loadShaderModule("./Shaders/coloured_triangle.frag.spv", &colortriangleFragShader), "Error when loading FRAG shader");
		GEARHEAD_CORE_INFO("Triangle fragment shader successfully loaded");

		VkShaderModule triangleVertShader;
		GEARHEAD_CORE_ASSERT_FUNC(loadShaderModule("./Shaders/triangle.vert.spv", &triangleVertShader), "Error when loading VERT shader");
		GEARHEAD_CORE_INFO("Triangle vertex shader successfully loaded");

		VkShaderModule triangleFragShader;
		GEARHEAD_CORE_ASSERT_FUNC(loadShaderModule("./Shaders/triangle.frag.spv", &triangleFragShader), "Error when loading FRAG shader");
		GEARHEAD_CORE_INFO("Triangle fragment shader successfully loaded");

		VkPipelineLayoutCreateInfo pipeline_layout_info = VkInit::pipeline_layout_create_info();

		if (vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout) != VK_SUCCESS) {
			GEARHEAD_CORE_ERROR("Failed to Create Pipeline Layout");
			GEARHEAD_DEBUGBREAK();
		}


		//build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader modules per stage
		PipelineBuilder pipelineBuilder;

		pipelineBuilder._shaderStages.push_back( VkInit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertShader));

		pipelineBuilder._shaderStages.push_back( VkInit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));


		//vertex input controls how to read vertices from vertex buffers. We aren't using it yet
		pipelineBuilder._vertexInputInfo = VkInit::vertex_input_state_create_info();

		//input assembly is the configuration for drawing triangle lists, strips, or individual points.
		//we are just going to draw triangle list
		pipelineBuilder._inputAssembly = VkInit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		//build viewport and scissor from the swapchain extents
		pipelineBuilder._viewport.x = 0.0f;
		pipelineBuilder._viewport.y = 0.0f;
		pipelineBuilder._viewport.width = (float)mData.props.Width;
		pipelineBuilder._viewport.height = (float)mData.props.Height;
		pipelineBuilder._viewport.minDepth = 0.0f;
		pipelineBuilder._viewport.maxDepth = 1.0f;

		pipelineBuilder._scissor.offset = { 0, 0 };
		pipelineBuilder._scissor.extent = mData.toVkExtent2D();

		//configure the rasterizer to draw filled triangles
		pipelineBuilder._rasterizer = VkInit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

		//we don't use multisampling, so just run the default one
		pipelineBuilder._multisampling = VkInit::multisampling_state_create_info();

		//a single blend attachment with no blending and writing to RGBA
		pipelineBuilder._colorBlendAttachment = VkInit::color_blend_attachment_state();

		//use the triangle layout we created
		pipelineBuilder._pipelineLayout = _trianglePipelineLayout;

		//finally build the pipeline
		_trianglePipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

		//clear the shader stages for the builder
		pipelineBuilder._shaderStages.clear();

		//add the other shaders
		pipelineBuilder._shaderStages.push_back( VkInit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, colortriangleVertShader));

		pipelineBuilder._shaderStages.push_back( VkInit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, colortriangleFragShader));

		//build the red triangle pipeline
		_colorTrianglePipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

		//u sure m8
		vkDestroyShaderModule(_device, colortriangleVertShader, nullptr);
		vkDestroyShaderModule(_device, colortriangleFragShader, nullptr);
		vkDestroyShaderModule(_device, triangleFragShader, nullptr);
		vkDestroyShaderModule(_device, triangleVertShader, nullptr);

		_mainDeletionQueue.push_function([=]() {
			//destroy the 2 pipelines we have created
			vkDestroyPipeline(_device, _colorTrianglePipeline, nullptr);
			vkDestroyPipeline(_device, _trianglePipeline, nullptr);

			//destroy the pipeline layout that they use
			vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
			}
		);

	}
	
	bool VkWindow::loadShaderModule(const char* filePath, VkShaderModule* outShaderModule)
	{
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) { 
			GEARHEAD_CORE_ERROR("FILE NOT FOUND");
			return false; 
		}

		size_t fileSize = (size_t)file.tellg();

		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);

		file.read((char*)buffer.data(), fileSize);

		file.close();


		//create a new shader module, using the buffer we loaded
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;

		//codeSize has to be in bytes, so multiply the ints in the buffer by size of int to know the real size of the buffer
		createInfo.codeSize = buffer.size() * sizeof(uint32_t);
		createInfo.pCode = buffer.data();

		//check that the creation goes well.
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			return false;
		}
		*outShaderModule = shaderModule;
		return true;



	}

	void VkWindow::OnUpdate()
	{
		glfwPollEvents();
		if (glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			_selectedShader = (_selectedShader + 1) % 2; //should work

		}
		DrawFrame();
	}

	void VkWindow::DrawFrame()
	{
		if (vkWaitForFences(_device, 1, &_renderFence, true, 1000000000) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to wait for fence"); abort(); };
		if (vkResetFences(_device, 1, &_renderFence) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to reset fence"); abort(); }

		uint32_t swapchainImageIndex;
		if (vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _presentSemaphore, nullptr, &swapchainImageIndex) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to acquire the next swapchain image"); abort(); }
		//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
		if (vkResetCommandBuffer(_mainCommandBuffer, 0) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to reset the command buffer"); abort(); }

		VkCommandBuffer cmd = _mainCommandBuffer;

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.pNext = nullptr;

		cmdBeginInfo.pInheritanceInfo = nullptr;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(cmd, &cmdBeginInfo) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to start the command buffer"); abort(); }

		VkClearValue clearValue;
		float flash = abs(sin(_frameNumber / 120.f));
		clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };

		//start the main renderpass.
		//We will use the clear color from above, and the framebuffer of the index the swapchain gave us. this is not the end of this code
		VkRenderPassBeginInfo rpInfo = {};
		rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpInfo.pNext = nullptr;

		rpInfo.renderPass = _renderPass;
		rpInfo.renderArea.offset.x = 0;
		rpInfo.renderArea.offset.y = 0;
		rpInfo.renderArea.extent = mData.toVkExtent2D();
		rpInfo.framebuffer = _frameBuffers[swapchainImageIndex];

		//connect clear values
		rpInfo.clearValueCount = 1;
		rpInfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		if (_selectedShader == 0)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);
		}
		else
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _colorTrianglePipeline);
		}
		vkCmdDraw(cmd, 3, 1, 0, 0);

		//prepare the submission to the queue.
		//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
		//we will signal the _renderSemaphore, to signal that rendering has finished

		VkSubmitInfo submit = {};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pNext = nullptr;

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		submit.pWaitDstStageMask = &waitStage;

		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &_presentSemaphore;

		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &_renderSemaphore;

		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd;

		vkCmdEndRenderPass(cmd);

		if (vkEndCommandBuffer(cmd) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("Failed to end the command buffer"); }
		// 
		// ok so I can't end the command buffer here because that makes the render pass fail
		// 
		//submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		if (vkQueueSubmit(_graphicsQueue, 1, &submit, _renderFence) != VK_SUCCESS) { GEARHEAD_CORE_ERROR("failed to sumbit the render queue"); abort(); }

		// this will put the image we just rendered into the visible window.
		// we want to wait on the _renderSemaphore for that,
		// as it's necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;

		presentInfo.pSwapchains = &_swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &_renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchainImageIndex;

		if(vkQueuePresentKHR(_graphicsQueue, &presentInfo)!= VK_SUCCESS){ GEARHEAD_CORE_ERROR("failed to sumbit the present image"); abort(); }

		//increase the number of frames drawn
		_frameNumber++;
		
	}



	void VkWindow::SetVSync(bool enabled) //change when vulkan
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		mData.vsync = enabled;
	}
}

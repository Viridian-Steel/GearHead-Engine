#include "ghpch.hpp"

//Internal Includes
#include "VkInit.hpp"
#include "VkWindow.hpp"
#include "VkPipeline.hpp"
#include "VkImages.hpp"

//ImGUI
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

//bootstrap library
#include "VkBootstrap.h"

//VMA
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

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
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		_window = glfwCreateWindow(mData.props.Width, mData.props.Height, mData.props.Title.c_str(), nullptr, nullptr);
		
		glfwMakeContextCurrent(_window);
		glfwSetWindowUserPointer(_window, &mData);

		SetVSync(true);

		//Vulkan

		InitVulkan();
		InitSwapchain();
		InitCommands();
		InitSyncStructures();
		InitDescriptors();
		InitPipelines();
		InitImGUI();

		s_VKInitialized = true;

	}

	VkWindow::~VkWindow() { Shutdown(); }

	void VkWindow::Shutdown()
	{
		//	Order of Destruction
		//      Command Pool    
		//	     SwapChain    
		//		  Device      
		//		  Surface      
		//		 Debugger
		//		 Instance   
		//	   Window(GLFW)


		if (s_VKInitialized) {

			vkDeviceWaitIdle(_device);
			_mainDeletionQueue.flush();

			DestroySwapChain();
			vkDestroySurfaceKHR(_instance, _surface, nullptr);

			vkDestroyDevice(_device, nullptr);
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
			.require_api_version(1,3,0)
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

		GEARHEAD_VKSUCCESS_CHECK(glfwCreateWindowSurface(_instance, _window, nullptr, &_surface));

		VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features.dynamicRendering = true;
		features.synchronization2 = true;

		//vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;

		vkb::PhysicalDeviceSelector selector{ vkbInstance };

		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 3)
			.set_required_features_13(features)
			.set_required_features_12(features12)
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

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = _chosenGPU;
		allocatorInfo.device = _device;
		allocatorInfo.instance = _instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		vmaCreateAllocator(&allocatorInfo, &_allocator);

		_mainDeletionQueue.push_function([&]() { vmaDestroyAllocator(_allocator); });

		GEARHEAD_CORE_INFO("Using GPU: {0}", physicalDevice.name);
	}

	void VkWindow::InitSwapchain()
	{
		CreateSwapChain(mData.props.Width, mData.props.Height);

		//draw image size will match the window
		VkExtent3D drawImageExtent = {
			mData.props.Width,
			mData.props.Height,
			1
		};

		//hardcoding the draw format to 32 bit float
		_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		_drawImage.imageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo rimg_info = VkInit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo = {};
		rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info = VkInit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		GEARHEAD_VKSUCCESS_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

		//add to deletion queues
		_mainDeletionQueue.push_function([=]() {
			vkDestroyImageView(_device, _drawImage.imageView, nullptr);
			vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
			});

	}

	void VkWindow::InitCommands() {

		VkCommandPoolCreateInfo commandPoolInfo = VkInit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		for (int i = 0; i < FRAME_OVERLAP; i++) {
			//Create the Command Pool
			GEARHEAD_VKSUCCESS_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._pool));

			//Create the Command Buffer
			VkCommandBufferAllocateInfo cmdAllocInfo = VkInit::command_buffer_allocate_info(_frames[i]._pool, 1U);
			GEARHEAD_VKSUCCESS_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._buffer));
			_mainDeletionQueue.push_function([=]() {
				vkDestroyCommandPool(_device, _frames[i]._pool, nullptr);
				_frames[i]._deletionQueue.flush();
				}
			);
		}
		GEARHEAD_VKSUCCESS_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immCommandPool));

		//Allocate the command buffer for immediate submits
		VkCommandBufferAllocateInfo alloc = VkInit::command_buffer_allocate_info(_immCommandPool, 1U);

		GEARHEAD_VKSUCCESS_CHECK(vkAllocateCommandBuffers(_device, &alloc, &_immCommandBuffer));

		_mainDeletionQueue.push_function([=]() {
			vkDestroyCommandPool(_device, _immCommandPool, nullptr);
			});



	}

	void VkWindow::InitSyncStructures()
	{
		VkFenceCreateInfo fenceCreateInfo = VkInit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = VkInit::semaphore_create_info();
		for (int i = 0; i < FRAME_OVERLAP; i++) {
			GEARHEAD_VKSUCCESS_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
			GEARHEAD_VKSUCCESS_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._SwapChainSemaphore));
			GEARHEAD_VKSUCCESS_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
			_mainDeletionQueue.push_function([=]() {
				vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
				vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
				vkDestroySemaphore(_device, _frames[i]._SwapChainSemaphore, nullptr);
				});
		}

		GEARHEAD_VKSUCCESS_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));
		_mainDeletionQueue.push_function([=]() { vkDestroyFence(_device, _immFence, nullptr); });

	}

	void VkWindow::InitDescriptors()
	{
		//create a descriptor pool that will hold 10 sets with 1 image each
		std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
		};

		GlobalDescriptorAllocator.init_pool(_device, 10, sizes);

		//make the descriptor set layout for our compute draw
		{
			DescriptorLayoutBuilder builder;
			builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			_drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
		}

		//allocate a descriptor set for our draw image
		_drawImageDescriptors = GlobalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imgInfo.imageView = _drawImage.imageView;

		VkWriteDescriptorSet drawImageWrite = {};
		drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		drawImageWrite.pNext = nullptr;

		drawImageWrite.dstBinding = 0;
		drawImageWrite.dstSet = _drawImageDescriptors;
		drawImageWrite.descriptorCount = 1;
		drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		drawImageWrite.pImageInfo = &imgInfo;

		vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

		//make sure both the descriptor allocator and the new layout get cleaned up properly
		_mainDeletionQueue.push_function([&]() {
			GlobalDescriptorAllocator.destroy_pool(_device);

			vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
			});

	}

	void VkWindow::OnUpdate()
	{
		glfwPollEvents();

		if (glfwGetWindowAttrib(_window, GLFW_ICONIFIED)) {
			isMinimized = true;
		}
		else {
			isMinimized = false;
		}
		
		if (isMinimized) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); return; }

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (ImGui::Begin("Background")) {
			ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];
			ImGui::Text("Selected Effect: ", selected.name);

			ImGui::SliderInt("EffectIndex", &currentBackgroundEffect, 0, backgroundEffects.size() - 1);
			
			ImGui::InputFloat4("data1", (float*)& selected.data.data1);
			ImGui::InputFloat4("data2", (float*)& selected.data.data2);
			ImGui::InputFloat4("data3", (float*)& selected.data.data3);
			ImGui::InputFloat4("data4", (float*)& selected.data.data4);
		}

		ImGui::End();

		//make Imgui calculate internal draw structures
		ImGui::Render();

		DrawFrame();
	}

	void VkWindow::DrawFrame()
	{
		GEARHEAD_VKSUCCESS_CHECK(vkWaitForFences(_device, 1, &GetCurrentFrame()._renderFence, true, 1000000000));

		GetCurrentFrame()._deletionQueue.flush();


		uint32_t swapchainImageIndex;
		GEARHEAD_VKSUCCESS_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, GetCurrentFrame()._SwapChainSemaphore, nullptr, &swapchainImageIndex));

		GEARHEAD_VKSUCCESS_CHECK(vkResetFences(_device, 1, &GetCurrentFrame()._renderFence));


		// now that we are sure that the commands finished executing, we can safely
		// reset the command buffer to begin recording again.
		GEARHEAD_VKSUCCESS_CHECK(vkResetCommandBuffer(GetCurrentFrame()._buffer, 0));

		VkCommandBuffer cmd = GetCurrentFrame()._buffer;

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = VkInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);


		//First Draw

		_drawExtent.width = _drawImage.imageExtent.width;
		_drawExtent.height = _drawImage.imageExtent.height;

		GEARHEAD_VKSUCCESS_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// transition our main draw image into general layout so we can write into it
		// we will overwrite it all so we dont care about what was the older layout
		VkUtil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		DrawBackground(cmd);

		//transition the draw image and the swapchain image into their correct transfer layouts
		VkUtil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		VkUtil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// End First Draw

		//ImGUI Draw


			// execute a copy from the draw image into the swapchain
		VkUtil::copy_image_to_image(cmd, _drawImage.image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

		// set swapchain image layout to Attachment Optimal so we can draw it
		VkUtil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		//draw imgui into the swapchain image
		DrawImGUI(cmd, _swapchainImageViews[swapchainImageIndex]);

		// set swapchain image layout to Present so we can draw it
		VkUtil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		GEARHEAD_VKSUCCESS_CHECK(vkEndCommandBuffer(cmd));

		//End ImGUI Draw

		VkCommandBufferSubmitInfo cmdinfo = VkInit::command_buffer_submit_info(cmd);

		VkSemaphoreSubmitInfo waitInfo = VkInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame()._SwapChainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = VkInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame()._renderSemaphore);

		VkSubmitInfo2 submit = VkInit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

		//submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		GEARHEAD_VKSUCCESS_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, GetCurrentFrame()._renderFence));
		
		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.pSwapchains = &_swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &GetCurrentFrame()._renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchainImageIndex;

		if (vkQueuePresentKHR(_graphicsQueue, &presentInfo) == VK_ERROR_OUT_OF_DATE_KHR) {
			RebuildSwapChain();
			return;
		}

		//increase the number of frames drawn
		_frameNumber++;

	}

	void VkWindow::DrawBackground(VkCommandBuffer cmd)
	{
		ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];
		// bind the background effect pipeling
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

		// bind the descriptor set containing the draw image for the compute pipeline
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

		vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);

	}

	void VkWindow::DrawImGUI(VkCommandBuffer cmd, VkImageView targetImageView) const
	{
		VkRenderingAttachmentInfo colorAttachment = VkInit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderInfo = VkInit::rendering_info(_swapchainExtent, &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRendering(cmd);
	}

	void VkWindow::CreateSwapChain(uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

		_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			//.use_default_format_selection()
			.set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			//use vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		_swapchainExtent = vkbSwapchain.extent;
		//store swapchain and its related images
		_swapchain = vkbSwapchain.swapchain;
		_swapchainImages = vkbSwapchain.get_images().value();
		_swapchainImageViews = vkbSwapchain.get_image_views().value();
	}

	void VkWindow::DestroySwapChain()
	{
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);

		// destroy swapchain resources
		for (int i = 0; i < _swapchainImageViews.size(); i++) {

			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		}
	}

	void VkWindow::RebuildSwapChain()
	{
		vkQueueWaitIdle(_graphicsQueue);

		vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };
		glfwGetFramebufferSize(_window, (int*)&mData.props.Width, (int*)&mData.props.Height);

		vkDestroySwapchainKHR(_device, _swapchain, nullptr);
		vkDestroyImageView(_device, _drawImage.imageView, nullptr);
		vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.use_default_format_selection()
			//use vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(mData.props.Width, mData.props.Height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		//store swapchain and its related images
		_swapchain = vkbSwapchain.swapchain;
		_swapchainImages = vkbSwapchain.get_images().value();
		_swapchainImageViews = vkbSwapchain.get_image_views().value();

		_swapchainImageFormat = vkbSwapchain.image_format;

		//depth image size will match the window
		VkExtent3D drawImageExtent = {
			mData.props.Width,
			mData.props.Height,
			1
		};

		//hardcoding the depth format to 32 bit float
		_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;

		VkImageCreateInfo rimg_info = VkInit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo = {};
		rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info = VkInit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		GEARHEAD_VKSUCCESS_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));
		/* For Later
		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imgInfo.imageView = _drawImage.imageView;

		VkWriteDescriptorSet cameraWrite = VkInit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, _drawImageDescriptors, &imgInfo, 0);

		vkUpdateDescriptorSets(_device, 1, &cameraWrite, 0, nullptr);
		*/


		//add to deletion queues
		_mainDeletionQueue.push_function([&]() {
			vkDestroyImageView(_device, _drawImage.imageView, nullptr);
			vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
			});

	}

	void VkWindow::InitPipelines()
	{
		InitBackgroundPipelines();
	}

	void VkWindow::InitBackgroundPipelines()
	{
		VkPipelineLayoutCreateInfo computeLayout{};
		computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		computeLayout.pNext = nullptr;
		computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
		computeLayout.setLayoutCount = 1;

		VkPushConstantRange pushConstant{};
		pushConstant.offset = 0;
		pushConstant.size = sizeof(ComputePushConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		computeLayout.pPushConstantRanges = &pushConstant;
		computeLayout.pushConstantRangeCount = 1;

		GEARHEAD_VKSUCCESS_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

		VkShaderModule gradientShader;
		if (!VkUtil::load_shader_module("./Shaders/Gradient.comp.spv", _device, &gradientShader))
		{
			GEARHEAD_CORE_ERROR("Failed to load ./Shaders/Gradient.comp.spv");
		}

		VkShaderModule skyShader;
		if (!VkUtil::load_shader_module("./Shaders/sky.comp.spv", _device, &skyShader)) {
			GEARHEAD_CORE_ERROR("failed to load ./Shaders/sky.comp.spv");
		}

		VkPipelineShaderStageCreateInfo stageinfo{};
		stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageinfo.pNext = nullptr;
		stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageinfo.module = gradientShader;
		stageinfo.pName = "main";

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.pNext = nullptr;
		computePipelineCreateInfo.layout = _gradientPipelineLayout;
		computePipelineCreateInfo.stage = stageinfo;

		ComputeEffect gradient = { .name = "gradient",.layout = _gradientPipelineLayout, .data = {} };

		gradient.data.data1 = glm::vec4(1, 0, 0, 1);
		gradient.data.data2 = glm::vec4(0, 0, 1, 1);


		GEARHEAD_VKSUCCESS_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));


		ComputeEffect sky = { .name = "sky", .layout = _gradientPipelineLayout, .data = {} };
		sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

		GEARHEAD_VKSUCCESS_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));

		backgroundEffects.push_back(gradient);
		backgroundEffects.push_back(sky);


		vkDestroyShaderModule(_device, gradientShader, nullptr);
		vkDestroyShaderModule(_device, skyShader, nullptr);

		_mainDeletionQueue.push_function([&]() {
			vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
			vkDestroyPipeline(_device, gradient.pipeline, nullptr);
			vkDestroyPipeline(_device, sky.pipeline, nullptr);
			});

	}

	void VkWindow::InitImGUI()
	{
		//1. Create DescriptorPool for ImGUI
		VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VkDescriptorPool imguiPool;
		GEARHEAD_VKSUCCESS_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

		//2. Initalize ImGUI
		
		//2.1 Initalize Core of ImGUI
		ImGui::CreateContext();
		
		//2.2 Initalize ImGUI for GLFW
		ImGui_ImplGlfw_InitForVulkan(_window, true);

		// this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = _instance;
		init_info.PhysicalDevice = _chosenGPU;
		init_info.Device = _device;
		init_info.Queue = _graphicsQueue;
		init_info.DescriptorPool = imguiPool;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.UseDynamicRendering = true;

		//dynamic rendering parameters for imgui to use
		init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchainImageFormat;

		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info);

		ImGui_ImplVulkan_CreateFontsTexture();

		// add the destroy the imgui created structures
		_mainDeletionQueue.push_function([=]() {
			ImGui_ImplVulkan_Shutdown();
			vkDestroyDescriptorPool(_device, imguiPool, nullptr);
			});

	}

	void VkWindow::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const
	{
		GEARHEAD_VKSUCCESS_CHECK(vkResetFences(_device, 1, &_immFence));
		GEARHEAD_VKSUCCESS_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

		VkCommandBuffer cmd = _immCommandBuffer;

		VkCommandBufferBeginInfo cmdBeginInfo = 
			VkInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		GEARHEAD_VKSUCCESS_CHECK(vkBeginCommandBuffer(_immCommandBuffer, &cmdBeginInfo));

		function(cmd);

		GEARHEAD_VKSUCCESS_CHECK(vkEndCommandBuffer(cmd));
		VkCommandBufferSubmitInfo cmdSubmitInfo = VkInit::command_buffer_submit_info(cmd);
		VkSubmitInfo2 submit = VkInit::submit_info(&cmdSubmitInfo, nullptr, nullptr);

		//submit, then wait for the fence to stop blocking. Fence blocks until the gpu is done;
		GEARHEAD_VKSUCCESS_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));
		GEARHEAD_VKSUCCESS_CHECK(vkWaitForFences(_device, 1, &_immFence, true, 9999999999));

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

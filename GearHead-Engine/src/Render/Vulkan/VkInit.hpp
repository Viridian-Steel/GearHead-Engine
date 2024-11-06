#pragma once

#include "VkTypes.hpp"

namespace VkInit { //used for abstraction, i guess
	VkCommandPoolCreateInfo command_pool_create_info(
		uint32_t queueFamilyIndex, 
		VkCommandPoolCreateFlags flags = 0);

	VkCommandBufferAllocateInfo command_buffer_allocate_info(
		VkCommandPool pool, 
		uint32_t count = 1, 
		VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

	VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags /*= 0*/);

	VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);

	VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);
	
	VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags);

	VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);

	VkSubmitInfo2 submit_info(
		VkCommandBufferSubmitInfo* cmd, 
		VkSemaphoreSubmitInfo* signalSemaphoreInfo,
		VkSemaphoreSubmitInfo* waitSemaphoreInfo);

	VkImageViewCreateInfo imageview_create_info(
		VkFormat format, 
		VkImage image, 
		VkImageAspectFlags aspectFlags);

	VkImageCreateInfo image_create_info(
		VkFormat format, 
		VkImageUsageFlags usageFlags, 
		VkExtent3D extent);

	VkRenderingAttachmentInfo attachment_info( 
		VkImageView view, 
		VkClearValue* clear, 
		VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderingInfo rendering_info(
		VkExtent2D renderExtent, 
		VkRenderingAttachmentInfo* colorAttachment,
		VkRenderingAttachmentInfo* depthAttachment);
}

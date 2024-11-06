#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace GearHead {
	struct AllocatedBuffer {
		VkBuffer _buffer;
		VmaAllocation _allocation;
	};


	struct AllocatedImage {
		VkImage image;
		VkImageView imageView;
		VmaAllocation allocation;
		VkExtent3D imageExtent;
		VkFormat imageFormat;
	};



	struct VertexInputDescription {
		std::vector<VkVertexInputBindingDescription> bindings;
		std::vector<VkVertexInputAttributeDescription> attributes;

		VkPipelineVertexInputStateCreateFlags flags = 0;
	};

}

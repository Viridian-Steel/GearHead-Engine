#include "VkTypes.hpp"
#include "Core/Core.hpp"
#include "ghpch.hpp"
#include "VkInit.hpp"

namespace VkUtil {
	bool load_shader_module(const char* filePath,
		VkDevice device,
		VkShaderModule* outShaderModule);
}


#pragma once

#include <glm/vec3.hpp>
#include <Render/Vulkan/VkTypes.hpp>


namespace GearHead {

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 color;

		static VertexInputDescription get_vertex_description();


	};


}


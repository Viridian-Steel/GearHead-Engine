#pragma once
#include "ghpch.hpp"
#include <Game/Common/Types.hpp>
#include "Render/Vulkan/VkTypes.hpp"

namespace GearHead {
	class GEARHEAD_API Mesh {
	public:
		std::vector<Vertex> _vertices;
		AllocatedBuffer _vertexBuffer;
	};
}

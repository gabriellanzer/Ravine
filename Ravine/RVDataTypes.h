
#ifndef RV_DATATYPES_H
#define RV_DATATYPES_H

//GLM Includes
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

//STD Includes
#include <array>

//Vulkan Includes
#include <vulkan/vulkan.h>


struct RvVertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(RvVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

		//Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(RvVertex, pos);

		//Color
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(RvVertex, color);

		//Texture
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(RvVertex, texCoord);

		//Texture
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(RvVertex, normal);

		return attributeDescriptions;
	}

};

struct RvMeshData
{
	RvVertex*		vertices;
	uint32_t	vertex_count;

	uint32_t*	indices;
	uint32_t	index_count;

	uint32_t*	textureIds;
	uint32_t	textures_count;
};

#endif
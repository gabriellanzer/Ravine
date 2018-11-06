
#ifndef RV_DATATYPES_H
#define RV_DATATYPES_H

//GLM Includes
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

//STD Includes
#include <array>
#include <vector>

//Vulkan Includes
#include <vulkan/vulkan.h>

//Assimp Includes
#include <assimp\matrix4x4.h>


struct RvVertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::uvec4 boneIDs;
	glm::vec4 boneWeights;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(RvVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions = {};

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

		//BoneID
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_UINT;
		attributeDescriptions[4].offset = offsetof(RvVertex, boneIDs);

		//BoneWeight
		attributeDescriptions[5].binding = 0;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[5].offset = offsetof(RvVertex, boneWeights);

		return attributeDescriptions;
	}

	void AddBoneData(uint16_t BoneID, float Weight) {
		for (uint16_t i = 0; i < 4; i++) {
			if (boneWeights[i] == 0.0) {
				boneIDs[i] = BoneID;
				boneWeights[i] = Weight;
				return;
			}
		}
	};

};

struct RvMeshData
{
	RvVertex*	vertices;
	uint32_t	vertex_count;

	uint32_t*	indices;
	uint32_t	index_count;

	uint32_t*	textureIds;
	uint32_t	textures_count;
};

struct BoneInfo
{
	aiMatrix4x4 BoneOffset;
	aiMatrix4x4 FinalTransformation;
};

#endif
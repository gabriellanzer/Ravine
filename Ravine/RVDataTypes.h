
#ifndef RV_DATATYPES_H
#define RV_DATATYPES_H

//GLM Includes
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

//STD Includes
#include <array>
#include <vector>

//Vulkan Includes
#include <vulkan/vulkan.h>

//Assimp Includes
#include <assimp\matrix4x4.h>

#pragma region RvAnimation

struct RvAnimation
{
	aiAnimation* animations;
	//Map<boneId, singleBoneAnimationFrames/*aiNodeAnim*/>
};

struct BoneInfo
{
	aiMatrix4x4 BoneOffset;
	aiMatrix4x4 FinalTransformation;
};

#pragma endregion

#pragma region RvMesh

struct RvVertex {
	glm::vec3 pos;
	glm::vec2 texCoord;
	glm::vec3 normal;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(RvVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

		//Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(RvVertex, pos);

		//Texture Coords
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(RvVertex, texCoord);

		//Normal
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(RvVertex, normal);

		return attributeDescriptions;
	}
};

struct RvMesh
{
	RvVertex*	vertices;
	uint32_t	vertex_count;

	uint32_t*	indices;
	uint32_t	index_count;

	uint32_t*	textureIds;
	uint32_t	textures_count;
};

struct RvVertexColored {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(RvVertexColored);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

		//Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(RvVertexColored, pos);

		//Color
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(RvVertexColored, color);

		//Texture
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(RvVertexColored, texCoord);

		//Normal
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(RvVertexColored, normal);

		return attributeDescriptions;
	}
};

struct RvMeshColored
{
	RvVertexColored*	vertices;
	uint32_t			vertex_count;

	uint32_t*			indices;
	uint32_t			index_count;

	uint32_t*			textureIds;
	uint32_t			textures_count;
};

#pragma endregion

#pragma region RvSkinnedMesh

struct RvSkinnedVertex {
	glm::vec3 pos;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::uvec4 boneIDs;
	glm::vec4 boneWeights;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(RvSkinnedVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

		//Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(RvSkinnedVertex, pos);

		//Texture Coords
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(RvSkinnedVertex, texCoord);

		//Normal
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(RvSkinnedVertex, normal);

		//BoneID
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_UINT;
		attributeDescriptions[3].offset = offsetof(RvSkinnedVertex, boneIDs);

		//BoneWeight
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(RvSkinnedVertex, boneWeights);

		return attributeDescriptions;
	}

	void AddBoneData(uint16_t BoneID, float Weight) {
		// Checking first vec
		for (uint16_t i = 0; i < 4; i++) {
			if (boneWeights[i] == 0.0) {
				boneIDs[i] = BoneID;
				boneWeights[i] = Weight;
				break;
			}
		}
	};

};

struct RvSkinnedMesh
{
	RvSkinnedVertex*	vertices;
	uint32_t			vertex_count;

	uint32_t*			indices;
	uint32_t			index_count;

	uint32_t*			textureIds;
	uint32_t			textures_count;
};

struct RvSkinnedVertexColored {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::uvec4 boneIDs;
	glm::vec4 boneWeights;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(RvSkinnedVertexColored);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions = {};

		//Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(RvSkinnedVertexColored, pos);

		//Color
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(RvSkinnedVertexColored, color);

		//Texture
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(RvSkinnedVertexColored, texCoord);

		//Normal
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(RvSkinnedVertexColored, normal);

		//BoneID
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_UINT;
		attributeDescriptions[4].offset = offsetof(RvSkinnedVertexColored, boneIDs);

		//BoneWeight
		attributeDescriptions[5].binding = 0;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[5].offset = offsetof(RvSkinnedVertexColored, boneWeights);

		return attributeDescriptions;
	}

	void AddBoneData(uint16_t BoneID, float Weight) {
		// Checking first vec
		for (uint16_t i = 0; i < 4; i++) {
			if (boneWeights[i] == 0.0) {
				boneIDs[i] = BoneID;
				boneWeights[i] = Weight;
				break;
			}
		}
	};

};

struct RvSkinnedMeshColored
{
	RvSkinnedVertexColored*	vertices;
	uint32_t				vertex_count;

	uint32_t*				indices;
	uint32_t				index_count;

	uint32_t*				textureIds;
	uint32_t				textures_count;

	aiNode* rootNode;
	uint16_t numBones;

	std::vector<RvAnimation> animations;
	aiMatrix4x4 animGlobalInverseTransform;
	std::map<std::string, uint16_t> boneMapping;
	std::vector<BoneInfo> boneInfo;
};

#pragma endregion

#endif
#ifndef RV_DATATYPES_H
#define RV_DATATYPES_H

//GLM Includes
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
using glm::vec3;
#include <glm/gtc/quaternion.hpp>
using glm::quat;

//EASTL Includes
#include <eastl/array.h>
#include <eastl/vector.h>
#include <eastl/map.h>
#include <eastl/string.h>

//Vulkan Includes
#include "volk.h"

//Assimp Includes
#include <assimp/scene.h>

using eastl::array;
using eastl::vector;
using eastl::map;
using eastl::string;

#pragma region RvAnimation

struct RvAnimation
{
	aiAnimation* aiAnim;
	//Map<boneId, singleBoneAnimationFrames/*aiNodeAnim*/>
};

struct RvBoneInfo
{
	aiMatrix4x4 BoneOffset;
	aiMatrix4x4 FinalTransformation;
};

#pragma endregion

#pragma region RvBaseMesh

template<typename RvVertexType>
struct RvBaseMesh
{
	RvVertexType* vertices;
	uint32_t	vertexCount;

	uint32_t*	indices;
	uint32_t	indexCount;

	//TODO: Move to RvMaterialState
	uint32_t*	textureIds;
	uint32_t	texturesCount;
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

	static array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

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

struct RvMesh : RvBaseMesh<RvVertex>
{

};

#pragma endregion

#pragma region RvMeshColored

struct RvVertexColored {
	glm::vec3 pos;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(RvVertexColored);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

		//Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(RvVertexColored, pos);

		//Texture
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(RvVertexColored, texCoord);

		//Normal
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(RvVertexColored, normal);

		//Color
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(RvVertexColored, color);

		return attributeDescriptions;
	}
};

struct RvMeshColored : RvBaseMesh<RvVertexColored>
{

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

	static array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
		array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

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

struct RvSkinnedMesh : RvBaseMesh<RvSkinnedVertex>
{
	aiNode* rootNode;
	uint16_t numBones = 0;

	vector<RvAnimation*> animations;
	aiMatrix4x4 animGlobalInverseTransform;
	// TODO: REFACTOR MAPPING TO NOT USE STRINGS
	map<string, uint16_t> boneMapping;
	vector<RvBoneInfo> boneInfo;

	//TODO: Move to RvAnimationState
	vector<aiMatrix4x4> boneTransforms;
	uint16_t curAnimId = 0;
};

#pragma endregion

#pragma region RvSkinnedMeshColored

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

	static array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
		array<VkVertexInputAttributeDescription, 6> attributeDescriptions = {};

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

struct RvSkinnedMeshColored : RvBaseMesh<RvSkinnedVertexColored>
{
	aiNode* rootNode;
	uint16_t numBones = 0;

	vector<RvAnimation*> animations;
	aiMatrix4x4 animGlobalInverseTransform;
	// TODO: REFACTOR MAPPING TO NOT USE STRINGS
	map<string, uint16_t> boneMapping;
	vector<RvBoneInfo> boneInfo;

	//TODO: Move to RvAnimationState
	vector<aiMatrix4x4> boneTransforms;
	uint16_t curAnimId = 0;
};

#pragma endregion

#endif

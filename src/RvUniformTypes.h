#ifndef RV_UNIFORMTYPES_H
#define RV_UNIFORMTYPES_H

//GLM Includes
#include "glm/ext/matrix_float4x4.hpp"
#include <glm/mat4x4.hpp>

struct RvGlobalBufferObject {
	glm::mat4 view = glm::mat4();
	glm::mat4 proj;
	glm::vec4 lightColor;
	glm::vec4 camPos;
};

struct RvMaterialBufferObject {
	glm::vec4 customColor;
};

struct RvModelBufferObject {
	glm::mat4 model;
};

struct RvBoneBufferObject {
	glm::mat4 transformMatrixes[128];
};

#endif

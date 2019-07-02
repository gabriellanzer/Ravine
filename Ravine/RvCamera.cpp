#include "RvCamera.h"

//Internal Dependencies
#include <glm\gtc\matrix_transform.hpp>

RvCamera::~RvCamera()
{
}


RvCamera::RvCamera(glm::vec3 camPos, float horizontalRot, float verticalRot)
{
	this->pos = glm::vec4(camPos.x, camPos.y, camPos.z, 0.f);
	horRot = horizontalRot;
	verRot = verticalRot;
}

void RvCamera::Translate(glm::vec4 deltaPos)
{
	pos += deltaPos;
}

glm::quat RvCamera::GetLookRot() const
{
	glm::quat lookRot = glm::vec3(0, 0, 0);
	lookRot = glm::rotate(lookRot, glm::radians(horRot), glm::vec3(0, 1, 0));
	lookRot = glm::rotate(lookRot, glm::radians(verRot), glm::vec3(1, 0, 0));
	return lookRot;
}

glm::mat4 RvCamera::GetViewMatrix() const
{
	return  glm::mat4() * glm::lookAt(glm::vec3(pos),
		glm::vec3(pos) + GetLookRot() * glm::vec3(0, 0, -1),
		glm::vec3(0, 1, 0));
}

#ifndef RV_CAMERA_H
#define RV_CAMERA_H

//GLM Includes
#include <glm\gtc\quaternion.hpp>

class RvCamera
{
public:
	RvCamera(glm::vec3 camPos, float horizontalRot, float verticalRot);
	~RvCamera();

	void Translate(glm::vec4 deltaPos);

	glm::quat GetLookRot() const;
	glm::mat4 GetViewMatrix() const;

	float horRot, verRot;
	glm::vec4 pos;
};

#endif
#ifndef ANIMATION_TOOLS_H
#define ANIMATION_TOOLS_H

//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#include <glm/gtx/quaternion.hpp>

//Assimp Includes
#include <assimp/anim.h>

namespace rvTools
{
	namespace animation
	{
		const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string nodeName);

		aiMatrix4x4 interpolateTranslation(float interpol, float time, float othertime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim);
		aiMatrix4x4 interpolateRotation(float interpol, float time, float othertime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim);
		aiMatrix4x4 interpolateScale(float interpol, float time, float othertime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim);

		uint16_t FindRotation(double AnimationTime, const aiNodeAnim* pNodeAnim);
		uint16_t FindScale(double AnimationTime, const aiNodeAnim* pNodeAnim);
		uint16_t FindPosition(double AnimationTime, const aiNodeAnim* pNodeAnim);
	}
}


#endif

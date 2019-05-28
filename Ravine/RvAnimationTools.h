#ifndef ANIMATION_TOOLS_H
#define ANIMATION_TOOLS_H

//EASTL Includes
#include <EASTL/string.h>

//Assimp Includes
#include <assimp/anim.h>

using eastl::string;

namespace rvTools
{
	namespace animation
	{
		const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const string nodeName);

		aiMatrix4x4 interpolateTranslation(float interpol, float time, float othertime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim);
		aiMatrix4x4 interpolateRotation(float interpol, float time, float othertime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim);
		aiMatrix4x4 interpolateScale(float interpol, float time, float othertime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim);

		uint16_t FindRotation(double AnimationTime, const aiNodeAnim* pNodeAnim);
		uint16_t FindScale(double AnimationTime, const aiNodeAnim* pNodeAnim);
		uint16_t FindPosition(double AnimationTime, const aiNodeAnim* pNodeAnim);
	}
}


#endif

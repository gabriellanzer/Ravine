#ifndef ANIMATION_TOOLS_H
#define ANIMATION_TOOLS_H

//EASTL Includes
#include <eastl/string.h>

//Assimp Includes
#include <assimp/anim.h>

using eastl::string;

namespace rvTools
{
	namespace animation
	{
		const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const string& nodeName);

		aiMatrix4x4 interpolateTranslation(double interpol, double time, double otherTime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim);
		aiMatrix4x4 interpolateRotation(double interpol, double time, double otherTime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim);
		aiMatrix4x4 interpolateScale(double interpol, double time, double otherTime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim);

		uint16_t findRotation(double animationTime, const aiNodeAnim* pNodeAnim);
		uint16_t findScale(double animationTime, const aiNodeAnim* pNodeAnim);
		uint16_t findPosition(double animationTime, const aiNodeAnim* pNodeAnim);
	}
}


#endif

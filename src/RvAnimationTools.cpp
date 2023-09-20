// #include "RvAnimationTools.h"


// namespace rvTools
// {
// 	namespace animation
// 	{
// 		// Find animation for a given node
// 		const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const string& nodeName)
// 		{
// 			for (uint32_t i = 0; i < animation->mNumChannels; i++)
// 			{
// 				const aiNodeAnim* nodeAnim = animation->mChannels[i];
// 				if (string(nodeAnim->mNodeName.data) == nodeName)
// 				{
// 					return nodeAnim;
// 				}
// 			}
// 			return nullptr;
// 		}

// 		aiMatrix4x4 interpolateTranslation(double interpol, double time, double otherTime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim)
// 		{
// 			aiVector3D translation;
// 			aiVector3D otherTranslation;

// 			if (pNodeAnim->mNumPositionKeys == 1)
// 			{
// 				translation = pNodeAnim->mPositionKeys[0].mValue;
// 				aiMatrix4x4 mat;
// 				aiMatrix4x4::Translation(translation, mat);
// 				return mat;
// 			}

// 			if (otherNodeAnim->mNumPositionKeys == 1)
// 			{
// 				otherTranslation = otherNodeAnim->mPositionKeys[0].mValue;
// 				aiMatrix4x4 mat;
// 				aiMatrix4x4::Translation(otherTranslation, mat);
// 				return mat;
// 			}

// 			uint32_t frameIndex = 0;
// 			for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
// 			{
// 				if (time < pNodeAnim->mPositionKeys[i + 1].mTime)
// 				{
// 					frameIndex = i;
// 					break;
// 				}
// 			}

// 			uint32_t otherFrameIndex = 0;
// 			for (uint32_t i = 0; i < otherNodeAnim->mNumPositionKeys - 1; i++)
// 			{
// 				if (otherTime < otherNodeAnim->mPositionKeys[i + 1].mTime)
// 				{
// 					otherFrameIndex = i;
// 					break;
// 				}
// 			}

// 			aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
// 			aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];
// 			float delta = (time - currentFrame.mTime) / (nextFrame.mTime - currentFrame.mTime);
// 			const aiVector3D& start = currentFrame.mValue;
// 			const aiVector3D& end = nextFrame.mValue;
// 			translation = (start + delta * (end - start));

// 			aiVectorKey otherCurrentFrame = otherNodeAnim->mPositionKeys[otherFrameIndex];
// 			aiVectorKey otherNextFrame = otherNodeAnim->mPositionKeys[(otherFrameIndex + 1) % otherNodeAnim->mNumPositionKeys];
// 			float otherDelta = (otherTime - otherCurrentFrame.mTime) / (otherNextFrame.mTime - otherCurrentFrame.mTime);
// 			const aiVector3D& otherStart = otherCurrentFrame.mValue;
// 			const aiVector3D& otherEnd = otherNextFrame.mValue;
// 			if (otherStart == otherEnd) {
// 				aiMatrix4x4 mat;
// 				aiMatrix4x4::Translation(translation, mat);
// 				return mat;
// 			}
// 			otherTranslation = (otherStart + otherDelta * (otherEnd - otherStart));
// 			aiVector3D mixTranslation = (translation + static_cast<float>(interpol) * (otherTranslation - translation));

// 			aiMatrix4x4 mat;
// 			aiMatrix4x4::Translation(mixTranslation, mat);

// 			return mat;
// 		}

// 		aiMatrix4x4 interpolateRotation(double interpol, double time, double otherTime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim)
// 		{
// 			aiQuaternion rotation;
// 			aiQuaternion otherRotation;

// 			if (pNodeAnim->mNumRotationKeys == 1)
// 			{
// 				rotation = pNodeAnim->mRotationKeys[0].mValue;
// 				aiMatrix4x4 mat(rotation.GetMatrix());
// 				return mat;
// 			}
// 			if (otherNodeAnim->mNumRotationKeys == 1)
// 			{
// 				rotation = otherNodeAnim->mRotationKeys[0].mValue;
// 				aiMatrix4x4 mat(rotation.GetMatrix());
// 				return mat;
// 			}

// 			uint32_t frameIndex = 0;
// 			for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
// 			{
// 				if (time < pNodeAnim->mRotationKeys[i + 1].mTime)
// 				{
// 					frameIndex = i;
// 					break;
// 				}
// 			}
// 			uint32_t otherFrameIndex = 0;
// 			for (uint32_t i = 0; i < otherNodeAnim->mNumRotationKeys - 1; i++)
// 			{
// 				if (otherTime < otherNodeAnim->mRotationKeys[i + 1].mTime)
// 				{
// 					otherFrameIndex = i;
// 					break;
// 				}
// 			}

// 			aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
// 			aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];
// 			float delta = (time - currentFrame.mTime) / (nextFrame.mTime - currentFrame.mTime);
// 			const aiQuaternion& start = currentFrame.mValue;
// 			const aiQuaternion& end = nextFrame.mValue;
// 			aiQuaternion::Interpolate(rotation, start, end, delta);
// 			rotation.Normalize();

// 			aiQuatKey otherCurrentFrame = otherNodeAnim->mRotationKeys[otherFrameIndex];
// 			aiQuatKey otherNextFrame = otherNodeAnim->mRotationKeys[(otherFrameIndex + 1) % otherNodeAnim->mNumRotationKeys];
// 			float otherDelta = (otherTime - otherCurrentFrame.mTime) / (otherNextFrame.mTime - otherCurrentFrame.mTime);
// 			const aiQuaternion& otherStart = otherCurrentFrame.mValue;
// 			const aiQuaternion& otherEnd = otherNextFrame.mValue;
// 			if (otherStart == otherEnd) {
// 				aiMatrix4x4 mat(rotation.GetMatrix());
// 				return mat;
// 			}

// 			aiQuaternion::Interpolate(otherRotation, otherStart, otherEnd, otherDelta);
// 			otherRotation.Normalize();

// 			aiQuaternion mixQuaternion;
// 			aiQuaternion::Interpolate(mixQuaternion, rotation, otherRotation, interpol);

// 			aiMatrix4x4 mat(mixQuaternion.GetMatrix());
// 			return mat;
// 		}
		
// 		aiMatrix4x4 interpolateScale(double interpol, double time, double otherTime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim)
// 		{
// 			aiVector3D scale;
// 			aiVector3D otherScale;

// 			if (pNodeAnim->mNumScalingKeys == 1)
// 			{
// 				scale = pNodeAnim->mScalingKeys[0].mValue;
// 				aiMatrix4x4 mat;
// 				aiMatrix4x4::Scaling(scale, mat);
// 				return mat;
// 			}
// 			if (otherNodeAnim->mNumScalingKeys == 1)
// 			{
// 				scale = otherNodeAnim->mScalingKeys[0].mValue;
// 				aiMatrix4x4 mat;
// 				aiMatrix4x4::Scaling(scale, mat);
// 				return mat;
// 			}

// 			uint32_t frameIndex = 0;
// 			for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
// 			{
// 				if (time < pNodeAnim->mScalingKeys[i + 1].mTime)
// 				{
// 					frameIndex = i;
// 					break;
// 				}
// 			}
// 			uint32_t otherFrameIndex = 0;
// 			for (uint32_t i = 0; i < otherNodeAnim->mNumScalingKeys - 1; i++)
// 			{
// 				if (otherTime < otherNodeAnim->mScalingKeys[i + 1].mTime)
// 				{
// 					otherFrameIndex = i;
// 					break;
// 				}
// 			}

// 			aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
// 			aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];
// 			float delta = (time - currentFrame.mTime) / (nextFrame.mTime - currentFrame.mTime);
// 			const aiVector3D& start = currentFrame.mValue;
// 			const aiVector3D& end = nextFrame.mValue;
// 			scale = (start + delta * (end - start));

// 			aiVectorKey otherCurrentFrame = otherNodeAnim->mScalingKeys[otherFrameIndex];
// 			aiVectorKey otherNextFrame = otherNodeAnim->mScalingKeys[(otherFrameIndex + 1) % otherNodeAnim->mNumScalingKeys];
// 			float otherDelta = (otherTime - otherCurrentFrame.mTime) / (otherNextFrame.mTime - otherCurrentFrame.mTime);
// 			const aiVector3D& otherStart = otherCurrentFrame.mValue;
// 			const aiVector3D& otherEnd = otherNextFrame.mValue;
// 			if (otherStart == otherEnd) {
// 				aiMatrix4x4 mat;
// 				aiMatrix4x4::Scaling(scale, mat);
// 				return mat;
// 			}
// 			otherScale = (otherStart + otherDelta * (otherEnd - otherStart));
// 			aiVector3D mixScale = (scale + static_cast<float>(interpol) * (otherScale - scale));

// 			aiMatrix4x4 mat;
// 			aiMatrix4x4::Scaling(mixScale, mat);
// 			return mat;
// 		}

// 		// Returns a 4x4 matrix with interpolated translation between current and next frame
// 		aiMatrix4x4 interpolateTranslation(double time, const aiNodeAnim* pNodeAnim)
// 		{
// 			aiVector3D translation;

// 			if (pNodeAnim->mNumPositionKeys == 1)
// 			{
// 				translation = pNodeAnim->mPositionKeys[0].mValue;
// 			}
// 			else
// 			{
// 				uint32_t frameIndex = 0;
// 				for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
// 				{
// 					if (time < pNodeAnim->mPositionKeys[i + 1].mTime)
// 					{
// 						frameIndex = i;
// 						break;
// 					}
// 				}

// 				aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
// 				aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

// 				float delta = (time - currentFrame.mTime) / (nextFrame.mTime - currentFrame.mTime);

// 				const aiVector3D& start = currentFrame.mValue;
// 				const aiVector3D& end = nextFrame.mValue;

// 				translation = (start + delta * (end - start));
// 			}

// 			aiMatrix4x4 mat;
// 			aiMatrix4x4::Translation(translation, mat);
// 			return mat;
// 		}

// 		// Returns a 4x4 matrix with interpolated rotation between current and next frame
// 		aiMatrix4x4 interpolateRotation(double time, const aiNodeAnim* pNodeAnim)
// 		{
// 			aiQuaternion rotation;

// 			if (pNodeAnim->mNumRotationKeys == 1)
// 			{
// 				rotation = pNodeAnim->mRotationKeys[0].mValue;
// 			}
// 			else
// 			{
// 				uint32_t frameIndex = 0;
// 				for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
// 				{
// 					if (time < pNodeAnim->mRotationKeys[i + 1].mTime)
// 					{
// 						frameIndex = i;
// 						break;
// 					}
// 				}

// 				aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
// 				aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

// 				float delta = (time - currentFrame.mTime) / (nextFrame.mTime - currentFrame.mTime);

// 				const aiQuaternion& start = currentFrame.mValue;
// 				const aiQuaternion& end = nextFrame.mValue;

// 				aiQuaternion::Interpolate(rotation, start, end, delta);
// 				rotation.Normalize();
// 			}

// 			aiMatrix4x4 mat(rotation.GetMatrix());
// 			return mat;
// 		}

// 		// Returns a 4x4 matrix with interpolated scaling between current and next frame
// 		aiMatrix4x4 interpolateScale(double time, const aiNodeAnim* pNodeAnim)
// 		{
// 			aiVector3D scale;

// 			if (pNodeAnim->mNumScalingKeys == 1)
// 			{
// 				scale = pNodeAnim->mScalingKeys[0].mValue;
// 			}
// 			else
// 			{
// 				uint32_t frameIndex = 0;
// 				for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
// 				{
// 					if (time < pNodeAnim->mScalingKeys[i + 1].mTime)
// 					{
// 						frameIndex = i;
// 						break;
// 					}
// 				}

// 				aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
// 				aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

// 				float delta = (time - currentFrame.mTime) / (nextFrame.mTime - currentFrame.mTime);

// 				const aiVector3D& start = currentFrame.mValue;
// 				const aiVector3D& end = nextFrame.mValue;

// 				scale = (start + delta * (end - start));
// 			}

// 			aiMatrix4x4 mat;
// 			aiMatrix4x4::Scaling(scale, mat);
// 			return mat;
// 		}

// 		uint16_t findRotation(double animationTime, const aiNodeAnim * pNodeAnim)
// 		{
// 			for (uint16_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
// 				if (animationTime < pNodeAnim->mRotationKeys[i + 1].mTime) {
// 					return i;
// 				}
// 			}
// 			return UINT16_MAX;
// 		}

// 		uint16_t findScale(double animationTime, const aiNodeAnim * pNodeAnim)
// 		{
// 			for (uint16_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
// 				if (animationTime < pNodeAnim->mScalingKeys[i + 1].mTime) {
// 					return i;
// 				}
// 			}
// 			return UINT16_MAX;
// 		}

// 		uint16_t findPosition(double animationTime, const aiNodeAnim * pNodeAnim)
// 		{
// 			for (uint16_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
// 				if (animationTime < pNodeAnim->mPositionKeys[i + 1].mTime) {
// 					return i;
// 				}
// 			}
// 			return  UINT16_MAX;
// 		}
// 	}
// }


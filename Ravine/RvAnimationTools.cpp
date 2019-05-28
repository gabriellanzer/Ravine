#include "RvAnimationTools.h"


namespace rvTools
{
	namespace animation
	{
		// Find animation for a given node
		const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const string nodeName)
		{
			for (uint32_t i = 0; i < animation->mNumChannels; i++)
			{
				const aiNodeAnim* nodeAnim = animation->mChannels[i];
				if (string(nodeAnim->mNodeName.data) == nodeName)
				{
					return nodeAnim;
				}
			}
			return nullptr;
		}

		aiMatrix4x4 interpolateTranslation(float interpol, float time, float othertime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim)
		{
			aiVector3D translation;
			aiVector3D othertranslation;

			if (pNodeAnim->mNumPositionKeys == 1)
			{
				translation = pNodeAnim->mPositionKeys[0].mValue;
				aiMatrix4x4 mat;
				aiMatrix4x4::Translation(translation, mat);
				return mat;
			}

			if (otherNodeAnim->mNumPositionKeys == 1)
			{
				othertranslation = otherNodeAnim->mPositionKeys[0].mValue;
				aiMatrix4x4 mat;
				aiMatrix4x4::Translation(othertranslation, mat);
				return mat;
			}

			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
			{
				if (time < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
				{
					frameIndex = i;
					break;
				}
			}

			uint32_t otherframeIndex = 0;
			for (uint32_t i = 0; i < otherNodeAnim->mNumPositionKeys - 1; i++)
			{
				if (othertime < (float)otherNodeAnim->mPositionKeys[i + 1].mTime)
				{
					otherframeIndex = i;
					break;
				}
			}

			aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
			aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];
			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);
			const aiVector3D& start = currentFrame.mValue;
			const aiVector3D& end = nextFrame.mValue;
			translation = (start + delta * (end - start));

			aiVectorKey othercurrentFrame = otherNodeAnim->mPositionKeys[otherframeIndex];
			aiVectorKey othernextFrame = otherNodeAnim->mPositionKeys[(otherframeIndex + 1) % otherNodeAnim->mNumPositionKeys];
			float otherdelta = (othertime - (float)othercurrentFrame.mTime) / (float)(othernextFrame.mTime - othercurrentFrame.mTime);
			const aiVector3D& otherstart = othercurrentFrame.mValue;
			const aiVector3D& otherend = othernextFrame.mValue;
			if (otherstart == otherend) {
				aiMatrix4x4 mat;
				aiMatrix4x4::Translation(translation, mat);
				return mat;
			}
			othertranslation = (otherstart + otherdelta * (otherend - otherstart));
			aiVector3D mixTranslation = (translation + interpol * (othertranslation - translation));

			aiMatrix4x4 mat;
			aiMatrix4x4::Translation(mixTranslation, mat);

			return mat;
		}

		aiMatrix4x4 interpolateRotation(float interpol, float time, float othertime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim)
		{
			aiQuaternion rotation;
			aiQuaternion otherrotation;

			if (pNodeAnim->mNumRotationKeys == 1)
			{
				rotation = pNodeAnim->mRotationKeys[0].mValue;
				aiMatrix4x4 mat(rotation.GetMatrix());
				return mat;
			}
			if (otherNodeAnim->mNumRotationKeys == 1)
			{
				rotation = otherNodeAnim->mRotationKeys[0].mValue;
				aiMatrix4x4 mat(rotation.GetMatrix());
				return mat;
			}

			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
			{
				if (time < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
				{
					frameIndex = i;
					break;
				}
			}
			uint32_t otherframeIndex = 0;
			for (uint32_t i = 0; i < otherNodeAnim->mNumRotationKeys - 1; i++)
			{
				if (othertime < (float)otherNodeAnim->mRotationKeys[i + 1].mTime)
				{
					otherframeIndex = i;
					break;
				}
			}

			aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
			aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];
			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);
			const aiQuaternion& start = currentFrame.mValue;
			const aiQuaternion& end = nextFrame.mValue;
			aiQuaternion::Interpolate(rotation, start, end, delta);
			rotation.Normalize();

			aiQuatKey othercurrentFrame = otherNodeAnim->mRotationKeys[otherframeIndex];
			aiQuatKey othernextFrame = otherNodeAnim->mRotationKeys[(otherframeIndex + 1) % otherNodeAnim->mNumRotationKeys];
			float otherdelta = (othertime - (float)othercurrentFrame.mTime) / (float)(othernextFrame.mTime - othercurrentFrame.mTime);
			const aiQuaternion& otherstart = othercurrentFrame.mValue;
			const aiQuaternion& otherend = othernextFrame.mValue;
			if (otherstart == otherend) {
				aiMatrix4x4 mat(rotation.GetMatrix());
				return mat;
			}

			aiQuaternion::Interpolate(otherrotation, otherstart, otherend, otherdelta);
			otherrotation.Normalize();

			aiQuaternion mixQuaternion;
			aiQuaternion::Interpolate(mixQuaternion, rotation, otherrotation, interpol);

			aiMatrix4x4 mat(mixQuaternion.GetMatrix());
			return mat;
		}
		
		aiMatrix4x4 interpolateScale(float interpol, float time, float othertime, const aiNodeAnim* pNodeAnim, const aiNodeAnim* otherNodeAnim)
		{
			aiVector3D scale;
			aiVector3D otherscale;

			if (pNodeAnim->mNumScalingKeys == 1)
			{
				scale = pNodeAnim->mScalingKeys[0].mValue;
				aiMatrix4x4 mat;
				aiMatrix4x4::Scaling(scale, mat);
				return mat;
			}
			if (otherNodeAnim->mNumScalingKeys == 1)
			{
				scale = otherNodeAnim->mScalingKeys[0].mValue;
				aiMatrix4x4 mat;
				aiMatrix4x4::Scaling(scale, mat);
				return mat;
			}

			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
			{
				if (time < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
				{
					frameIndex = i;
					break;
				}
			}
			uint32_t otherframeIndex = 0;
			for (uint32_t i = 0; i < otherNodeAnim->mNumScalingKeys - 1; i++)
			{
				if (othertime < (float)otherNodeAnim->mScalingKeys[i + 1].mTime)
				{
					otherframeIndex = i;
					break;
				}
			}

			aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
			aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];
			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);
			const aiVector3D& start = currentFrame.mValue;
			const aiVector3D& end = nextFrame.mValue;
			scale = (start + delta * (end - start));

			aiVectorKey othercurrentFrame = otherNodeAnim->mScalingKeys[otherframeIndex];
			aiVectorKey othernextFrame = otherNodeAnim->mScalingKeys[(otherframeIndex + 1) % otherNodeAnim->mNumScalingKeys];
			float otherdelta = (othertime - (float)othercurrentFrame.mTime) / (float)(othernextFrame.mTime - othercurrentFrame.mTime);
			const aiVector3D& otherstart = othercurrentFrame.mValue;
			const aiVector3D& otherend = othernextFrame.mValue;
			if (otherstart == otherend) {
				aiMatrix4x4 mat;
				aiMatrix4x4::Scaling(scale, mat);
				return mat;
			}
			otherscale = (otherstart + otherdelta * (otherend - otherstart));
			aiVector3D mixScale = (scale + interpol * (otherscale - scale));

			aiMatrix4x4 mat;
			aiMatrix4x4::Scaling(mixScale, mat);
			return mat;
		}

		// Returns a 4x4 matrix with interpolated translation between current and next frame
		aiMatrix4x4 interpolateTranslation(float time, const aiNodeAnim* pNodeAnim)
		{
			aiVector3D translation;

			if (pNodeAnim->mNumPositionKeys == 1)
			{
				translation = pNodeAnim->mPositionKeys[0].mValue;
			}
			else
			{
				uint32_t frameIndex = 0;
				for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
				{
					if (time < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
					{
						frameIndex = i;
						break;
					}
				}

				aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
				aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

				float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

				const aiVector3D& start = currentFrame.mValue;
				const aiVector3D& end = nextFrame.mValue;

				translation = (start + delta * (end - start));
			}

			aiMatrix4x4 mat;
			aiMatrix4x4::Translation(translation, mat);
			return mat;
		}

		// Returns a 4x4 matrix with interpolated rotation between current and next frame
		aiMatrix4x4 interpolateRotation(float time, const aiNodeAnim* pNodeAnim)
		{
			aiQuaternion rotation;

			if (pNodeAnim->mNumRotationKeys == 1)
			{
				rotation = pNodeAnim->mRotationKeys[0].mValue;
			}
			else
			{
				uint32_t frameIndex = 0;
				for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
				{
					if (time < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
					{
						frameIndex = i;
						break;
					}
				}

				aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
				aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

				float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

				const aiQuaternion& start = currentFrame.mValue;
				const aiQuaternion& end = nextFrame.mValue;

				aiQuaternion::Interpolate(rotation, start, end, delta);
				rotation.Normalize();
			}

			aiMatrix4x4 mat(rotation.GetMatrix());
			return mat;
		}

		// Returns a 4x4 matrix with interpolated scaling between current and next frame
		aiMatrix4x4 interpolateScale(float time, const aiNodeAnim* pNodeAnim)
		{
			aiVector3D scale;

			if (pNodeAnim->mNumScalingKeys == 1)
			{
				scale = pNodeAnim->mScalingKeys[0].mValue;
			}
			else
			{
				uint32_t frameIndex = 0;
				for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
				{
					if (time < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
					{
						frameIndex = i;
						break;
					}
				}

				aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
				aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

				float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

				const aiVector3D& start = currentFrame.mValue;
				const aiVector3D& end = nextFrame.mValue;

				scale = (start + delta * (end - start));
			}

			aiMatrix4x4 mat;
			aiMatrix4x4::Scaling(scale, mat);
			return mat;
		}

		uint16_t FindRotation(double AnimationTime, const aiNodeAnim * pNodeAnim)
		{
			for (uint16_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
				if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
					return i;
				}
			}
		}

		uint16_t FindScale(double AnimationTime, const aiNodeAnim * pNodeAnim)
		{
			for (uint16_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
				if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
					return i;
				}
			}
		}

		uint16_t FindPosition(double AnimationTime, const aiNodeAnim * pNodeAnim)
		{
			for (uint16_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
				if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
					return i;
				}
			}
		}
	}
}


#version 450

layout(set=0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	vec4 lightColor;
	vec4 camPos;
};

layout(set=2, binding = 0) uniform ModelBufferObject {
	mat4 model;
};

layout(set=2, binding = 1) uniform BonesBufferObject {
	mat4 boneTransforms[128];
};

layout(location = 0) in vec3 inPosition;
layout(location = 4) in uvec4 inBoneID;
layout(location = 5) in vec4 inBoneWeight;

void main() {

	mat4 BoneTransform = boneTransforms[inBoneID[0]] * inBoneWeight[0];
	BoneTransform += boneTransforms[inBoneID[1]] * inBoneWeight[1];
	BoneTransform += boneTransforms[inBoneID[2]] * inBoneWeight[2];
	BoneTransform += boneTransforms[inBoneID[3]] * inBoneWeight[3];

	vec4 PosL = BoneTransform * vec4(inPosition, 1.0);
    gl_Position = proj * view * model * PosL;
}
#version 450

layout(set=0, binding = 0) uniform UniformBufferObject {
	mat4 model;
    mat4 view;
    mat4 proj;
	vec4 objectColor;
	vec4 lightColor;
	vec4 camPos;
} ubo;

layout(set=2, binding = 0) uniform BonesBufferObject {
	mat4 boneTransforms[128];
} bones;

layout(location = 0) in vec3 inPosition;
layout(location = 4) in uvec4 inBoneID;
layout(location = 5) in vec4 inBoneWeight;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {

	mat4 BoneTransform = bones.boneTransforms[inBoneID[0]] * inBoneWeight[0];
	BoneTransform += bones.boneTransforms[inBoneID[1]] * inBoneWeight[1];
	BoneTransform += bones.boneTransforms[inBoneID[2]] * inBoneWeight[2];
	BoneTransform += bones.boneTransforms[inBoneID[3]] * inBoneWeight[3];

	vec4 PosL = BoneTransform * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * ubo.model * PosL;
}
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
    mat4 view;
    mat4 proj;
	vec4 objectColor;
	vec4 lightColor;
	vec4 camPos;
} ubo;

layout(binding = 2) uniform BonesBufferObject {
	mat4 boneTransforms[100];
} bones;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNorm;
layout(location = 4) in uvec4 inBoneID;
layout(location = 5) in vec4 inBoneWeight;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNorm;
layout(location = 3) out vec3 fragPos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {

	mat4 BoneTransform = mat4(1.0);
	BoneTransform += bones.boneTransforms[inBoneID[0]] * inBoneWeight[0];
    BoneTransform += bones.boneTransforms[inBoneID[1]] * inBoneWeight[1];
    BoneTransform += bones.boneTransforms[inBoneID[2]] * inBoneWeight[2];
    BoneTransform += bones.boneTransforms[inBoneID[3]] * inBoneWeight[3];

	vec4 PosL = BoneTransform * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * ubo.model * PosL;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
	fragNorm = mat3(transpose(inverse(ubo.model))) * inNorm;
	fragPos = vec3(ubo.model * PosL);
}
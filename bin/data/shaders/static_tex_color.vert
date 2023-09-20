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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNorm;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 fragColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	vec4 PosL = model * vec4(inPosition, 1.0);
    gl_Position = proj * view * PosL;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
	fragNorm = mat3(transpose(inverse(model))) * inNorm;
	fragPos = PosL.xyz;
}
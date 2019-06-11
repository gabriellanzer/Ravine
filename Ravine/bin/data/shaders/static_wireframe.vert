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

void main()
{
    gl_Position = proj * view * model * vec4(inPosition, 1.0);
	gl_PointSize = 15.0f-(gl_Position.z)*15.0f;
}
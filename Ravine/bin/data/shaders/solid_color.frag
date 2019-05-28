#version 450

layout(binding = 2) uniform MaterialBufferObject {
	vec4 customColor;
} material;

layout(location = 0) out vec4 outColor;

void main() {

    outColor = material.customColor;
}
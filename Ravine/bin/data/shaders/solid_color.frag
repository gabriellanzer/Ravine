#version 450

layout(set=1, binding = 0) uniform MaterialBufferObject {
	vec4 customColor;
} material;

layout(location = 0) out vec4 outColor;

void main() {

    outColor = material.customColor;
}
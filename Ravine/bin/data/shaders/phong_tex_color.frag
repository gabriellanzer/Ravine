#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	vec4 objectColor;
	vec4 lightColor;
	vec4 camPos;
} ubo;

layout(set=1, binding = 0) uniform MaterialBufferObject {
	vec4 customColor;
} material;

layout(set=1, binding = 1) uniform sampler2D texSampler[2];

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

float ambientStrength = 0.8;
float specularStrength = 0.5;
float specularFactor = 64;

vec3 lightPos = vec3(5.0, 5.0, 5.0);

void main() {

	//Ambient
    vec3 ambient = ambientStrength * ubo.lightColor.rgb;

	vec3 norm = normalize(fragNorm);
	vec3 lightDir = normalize(lightPos - fragPos);

	//Diffuse
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * ubo.lightColor.rgb;

	//Specular
	vec3 viewDir = normalize(ubo.camPos.rgb - fragPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularFactor);
	vec3 specular = specularStrength * spec * ubo.lightColor.rgb;  

    vec3 result = (ambient + diffuse + specular) * ubo.objectColor.rgb;

    outColor = vec4(fragColor * texture(texSampler[1], fragTexCoord).rgb * result, 1.0) * material.customColor;
}
#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 innormal;

uniform mat4 modelViewProjectionMatrix;
uniform mat4 model;

out vec3 Normal;
out vec3 FragPos;

void main()
{
	Normal = mat3(transpose(inverse(model))) * innormal;
	FragPos = vec3(model * vec4(position, 1.0));
	gl_Position = modelViewProjectionMatrix * vec4(position, 1.0);
}

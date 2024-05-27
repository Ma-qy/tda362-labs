#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;
const vec3 lightColor = vec3(1.0, 1.0, 1.0);


in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 material_color;

layout(location = 0) out vec4 fragmentColor;

void main()
{
	// Ambient lighting
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * lightColor;

	// Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * material_color;
	fragmentColor = vec4(result, 1.0);
}

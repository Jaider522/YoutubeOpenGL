
#version 330 core

out vec4 FragColor;

in vec3 color;
in vec2 texCoord;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D tex0;
uniform vec4 baseColorFactor;

// Propiedades de la luz
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Intensidades de iluminación
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;
uniform float lightIntensity;

void main()
{
    // Color original de la textura
    vec4 albedo = texture(tex0, texCoord) * baseColorFactor;

    // Normal del fragmento
    vec3 norm = normalize(Normal);

    // Dirección de la luz
    vec3 lightDir = normalize(lightPos - FragPos);

    // Dirección de la cámara
    vec3 viewDir = normalize(viewPos - FragPos);

    // Reflexión de la luz
    vec3 reflectDir = reflect(-lightDir, norm);

    // Componente ambiental
    vec3 ambient = ambientStrength * lightColor;

    // Componente difusa
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Componente especular
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // Iluminación final
    vec3 result = ((ambient + diffuse) * albedo.rgb + specular) * lightIntensity;

    FragColor = vec4(result, albedo.a);
}

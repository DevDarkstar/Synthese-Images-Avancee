#version 450
#extension GL_NV_shadow_samplers_cube : enable

uniform vec3 lightPosition;
uniform samplerCube skyboxTexture;

in VS_OUT{
    vec3 fragPosition; // Coordonnées du sommet dans l'espace global
    vec3 Normal; // Normale du sommet dans l'espace global
} fs_in;

out vec4 finalColor;

void main() {
    // Normalisation de la normale su sommet courant
    vec3 normalized_normal = normalize(fs_in.Normal);

    // Calcul de la direction de la lumière
    vec3 lightDir = normalize(lightPosition - fs_in.fragPosition);

    // Calcul du vecteur réfléchi
    vec3 reflected_vector = reflect(-lightDir, normalized_normal);

    // Calcul de la couleur de texture induite par la skybox (on prend l'opposé de reflected_color pour avoir le reflet de la Skybox dans le bon sens)
    vec3 skyboxColor = textureCube(skyboxTexture, -reflected_vector).rgb;

    finalColor = vec4(skyboxColor, 1.0);
}

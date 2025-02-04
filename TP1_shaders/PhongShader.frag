#version 450
#extension GL_NV_shadow_samplers_cube : enable

uniform vec3 cameraPosition;

struct Material{
    vec3 specular; // Couleur de la lumière spéculaire
    float shininess; // Brillance de l'objet (utilisé pour le calcul de l'intensité de la spéculaire)
    vec3 ambient; // Couleur de la lumière ambiante
    vec3 diffuse; // Couleur de la lumière diffuse
};

uniform vec3 lightPosition;
uniform Material material;
uniform samplerCube texMap;
uniform vec3 objectColor; // Couleur de l'objet
uniform float Ka; // Coefficient de la lumière ambiante
uniform float Kd; // Coefficient de la lumière diffuse
uniform float Ks; // Coefficient de la lumière spéculaire

in VS_OUT{
    vec3 fragPosition; // Coordonnées du sommet dans l'espace global
    vec3 Normal; // Normale au sommet dans l'espace global
} fs_in;

out vec4 finalColor;

void main() {
    // Normalisation de la normale su sommet courant
    vec3 normalized_normal = normalize(fs_in.Normal);
    // Calcul de la couleur de la lumière ambiante résultante en tenant compte du coefficient de
    // la lumière ambiante Ka et de la couleur de la lumière ambiante
    vec3 ambient = Ka * material.ambient;

    // Calcul de la direction de la lumière
    vec3 lightDir = normalize(lightPosition - fs_in.fragPosition);
    // Calcul de l'intensité de la lumière diffuse sur le sommet courant
    vec3 diffuse = Kd * max(dot(lightDir, normalized_normal), 0.0) * material.diffuse;

    // Calcul du vecteur partant du sommet du maillage et allant en direction de la vue de la caméra
    vec3 viewDir = normalize(cameraPosition - fs_in.fragPosition);
    // Calcul du vecteur réfléchi
    vec3 reflected_vector = reflect(-lightDir, normalized_normal);
    // Calcul de l'intensité de la lumière spéculaire sur le sommet courant
    vec3 specular = Ks * pow(max(dot(viewDir, reflected_vector), 0.0), material.shininess) * material.specular;

    // Calcul de la couleur finale du sommet en tenant compte des couleurs ambiante, diffuse et spéculaire
    finalColor = vec4((ambient + diffuse + specular) * objectColor, 1.0);
}

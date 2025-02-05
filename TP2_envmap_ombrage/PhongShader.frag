#version 450
#extension GL_NV_shadow_samplers_cube : enable

struct Material{
    vec3 specular; // Couleur de la lumière spéculaire
    float shininess; // Brillance de l'objet (utilisé pour le calcul de l'intensité de la spéculaire)
    vec3 ambient; // Couleur de la lumière ambiante
    vec3 diffuse; // Couleur de la lumière diffuse
    vec3 objectColor; // Couleur de l'objet
};

uniform vec3 cameraPosition;
uniform vec3 lightPosition;
uniform Material material;
uniform float Ka; // Coefficient de la lumière ambiante
uniform float Kd; // Coefficient de la lumière diffuse
uniform float Ks; // Coefficient de la lumière spéculaire

uniform sampler2D shadowTexture;

in VS_OUT{
    vec3 fragPosition; // Coordonnées du sommet dans l'espace global
    vec3 Normal; // Normale au sommet dans l'espace global
    vec4 fragPosLightSpace; // Coordonnées du sommet dans l'espace de la lumière
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
    // Calcul du produit scalaire entre la normal au sommet et la direction de la lumière
    float dotLight = dot(lightDir, normalized_normal);
    // Calcul de l'intensité de la lumière diffuse sur le sommet courant
    vec3 diffuse = Kd * max(dotLight, 0.0) * material.diffuse;

    // Calcul du vecteur partant du sommet du maillage et allant en direction de la vue de la caméra
    vec3 viewDir = normalize(cameraPosition - fs_in.fragPosition);
    // Calcul du vecteur réfléchi
    vec3 reflected_vector = reflect(-lightDir, normalized_normal);
    // Calcul de l'intensité de la lumière spéculaire sur le sommet courant
    vec3 specular = Ks * pow(max(dot(viewDir, reflected_vector), 0.0), material.shininess) * material.specular;

    // Calcul de l'ombrage
    // Passage des coordonnées homogènes en coordonnées cartésiennes
    vec3 projCoords = fs_in.fragPosLightSpace.xyz / fs_in.fragPosLightSpace.w;
    // Changement de l'intervalle des coordonnées cartésiennes de l'intervalle [-1,1] à [0,1]
    projCoords = projCoords * 0.5 + 0.5;
    // Récupération de la profondeur la plus proche à partir des coordonnées xy du fragment
    float closestDepth = texture(shadowTexture, projCoords.xy).r;
    // Récupération de la profondeur du fragment dans l'espace de la lumière
    float currentDepth = projCoords.z;
    // Création d'un biais afin d'éviter des effets de Moiré sur l'ombrage
    float bias = max(0.05 * (1.0 - dotLight), 0.005);
    // Détermination si le fragment est dans l'ombre
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    // Calcul de la couleur finale du sommet en tenant compte des couleurs ambiante, diffuse et spéculaire et de l'ombre portée de l'objet
    finalColor = vec4((ambient + (1.0 - shadow) * (diffuse + specular)) * material.objectColor, 1.0);
}

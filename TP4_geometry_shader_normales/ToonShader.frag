#version 450

uniform vec3 cameraPosition; // Position de la caméra
uniform vec3 lightPosition; // Position de la lumière
uniform vec3 materialDiffuse; // Couleur de la diffuse
uniform float Kd; // Coefficient de la lumière diffuse
uniform vec3 objectColor; // Couleur de l'objet

in VS_OUT{
    vec3 fragPosition; // Position du sommet dans l'espace global
    vec3 Normal; // Normale du sommet dans l'espace global
} fs_in;

out vec4 finalColor;

void main(){
    // Normalisation de la normale au sommet courant
    vec3 normalized_normal = normalize(fs_in.Normal);

    // Calcul de la direction de la lumière
    vec3 lightDir = normalize(lightPosition - fs_in.fragPosition);

    // Calcul de l'intensité de la diffuse sur le sommet courant
    float diff = max(dot(lightDir, normalized_normal), 0.0);
    // Détermination de la couleur de la diffuse en fonction de son intensité
    vec3 diffuse;
    if (diff > 0.95)
        diffuse = Kd * vec3(1.0,1.0,1.0);
    else if (diff > 0.5)
        diffuse = Kd * vec3(0.78, 0.88, 1.0);
    else if (diff > 0.25)
        diffuse = Kd * vec3(0.62, 0.8, 1.0);
    else
        diffuse = Kd * vec3(0.47, 0.71, 1.0);

    finalColor = vec4(diffuse * objectColor, 1.0);
}

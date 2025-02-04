#version 450

uniform vec3 cameraPosition; // Position de la caméra
uniform vec3 lightPosition; // Position de la lumière
uniform vec3 coolColor; // Couleur froide
uniform vec3 warmColor; // Couleur chaude 
uniform uint displaySilhouette; // Paramètre contrôlant l'affichage de la silhouette de l'objet
uniform vec3 silhouetteColor; // Couleur de la silhouette autour de l'objet
uniform float epsilon; // Paramètre contrôlant l'intensité de l'effet de bord (silhouette) autour de l'objet

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

    // Calcul de l'intensité de la lumière de Gooch en faisant en sorte que la valeur
    // du produit scalaire de la normale au sommet avec la direction de la lumière
    // soit comprise dans l'intervalle [0,1]
    float k = (1.0 + dot(normalized_normal, lightDir)) / 2;

    // Calcul de la couleur du sommet résultante par interpolation linéaire entre la couleur
    // froide et la couleur chaude en fonction de la valeur de l'intensité de la lumière de Gooch
    vec3 goochColor = mix(coolColor, warmColor, k); 

    vec3 silhouetteBorder;
    // Calcul du vecteur partant du sommet du maillage et allant en direction de la vue de la caméra
    vec3 viewDir = normalize(cameraPosition - fs_in.fragPosition);
    // Si l'affichage de la silhouette est actif
    if (displaySilhouette == 1)
        // Si le produit scalaire entre la vue caméra et la normale au sommet est inférieure ou égal à la valeur epsilon
        if (dot(viewDir, normalized_normal) <= epsilon)
            finalColor = vec4(silhouetteColor, 1.0);
        else
            finalColor = vec4(goochColor, 1.0);
    else
        finalColor = vec4(goochColor, 1.0);
}
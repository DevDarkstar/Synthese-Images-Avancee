#version 450

uniform vec3 cameraPosition; // Position de la caméra
uniform vec3 lightPosition; // Position de la lumière
uniform vec3 materialDiffuse; // Couleur de la diffuse
uniform float Kd; // Coefficient de la lumière diffuse
uniform vec3 objectColor; // Couleur de l'objet
uniform uint displaySilhouette; // Paramètre contrôlant l'affichage de la silhouette de l'objet
uniform sampler1D silhouetteTex;

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

    // Si l'affichage de la silhouette est actif
    if (displaySilhouette == 1){
        // Calcul du vecteur partant du sommet du maillage et allant en direction de la vue de la caméra
        vec3 viewDir = normalize(cameraPosition - fs_in.fragPosition);
        // Calcul du produit scalaire entre la vue et la normale au sommet en prenant un résultat compris dans l'intervalle [0,1]
        float value = max(dot(viewDir, normalized_normal), 0.0);
        // Récupération de la couleur associée à cette valeur dans la texture 1D de seuillage
        vec3 color = texture(silhouetteTex, value).rgb;
        finalColor = vec4(diffuse * color * objectColor, 1.0);
    }
    else
        finalColor = vec4(diffuse * objectColor, 1.0);
}

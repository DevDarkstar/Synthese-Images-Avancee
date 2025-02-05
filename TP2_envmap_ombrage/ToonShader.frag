#version 450
#extension GL_NV_shadow_samplers_cube : enable

uniform vec3 cameraPosition; // Position de la caméra
uniform vec3 lightPosition; // Position de la lumière
uniform vec3 materialDiffuse; // Couleur de la diffuse
uniform float Kd; // Coefficient de la lumière diffuse
uniform vec3 objectColor; // Couleur de l'objet
uniform uint displaySilhouette; // Paramètre contrôlant l'affichage de la silhouette de l'objet
uniform float textureMix; // Mix entre une environment map et un mix de texture classique et de shader Toon
uniform float toonMix; // Mix entre une texture classique et le shader Toon
uniform sampler2D classicTexture; // Texture classique
uniform samplerCube skyboxTexture; // Skybox

in VS_OUT{
    vec3 fragPosition; // Coordonnées du sommet dans l'espace global
    vec3 Normal; // Normale du sommet dans l'espace global
    vec2 UVCoords; // Coordonnées de la texture du sommet
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
        diffuse = Kd * vec3(1.0, 0.02, 0.05);
    else if (diff > 0.25)
        diffuse = Kd * vec3(0.8, 0.02, 0.04);
    else
        diffuse = Kd * vec3(0.6, 0.01, 0.03);

    // Calcul du vecteur réfléchi
    vec3 reflected_vector = reflect(-lightDir, normalized_normal);

    // Calcul de la couleur de texture induite par la skybox (on prend l'opposé de reflected_color pour avoir le reflet de la Skybox dans le bon sens)
    vec3 skyboxColor = textureCube(skyboxTexture, -reflected_vector).rgb;

    // Calcul de la couleur du sommet induite par ses coordonnées UV sur la texture classique
    vec3 textureColor = texture(classicTexture, fs_in.UVCoords).rgb;

    // Calcul de la couleur finale par interpolation linéaire entre la skybox et
    // l'interpolation linéaire entre la couleur de la texture classique et le shader Toon
    finalColor = vec4(mix(skyboxColor, mix(textureColor, diffuse * objectColor, toonMix), textureMix), 1.0);
}

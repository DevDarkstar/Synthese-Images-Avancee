#version 450


uniform mat4 MODEL;
uniform mat4 VIEW;
uniform mat4 PERSPECTIVE;

// Récupération des coordonnées du sommet
layout(location = 0) in vec3 position; // le location permet de dire de quel flux/canal on récupère les données (doit être en accord avec le location du code opengl)
// Récupération de sa normale associée
layout(location = 3) in vec3 normal;

out VS_OUT{
    vec3 fragPosition;
    vec3 Normal;
} vs_out;

void main(){
    gl_Position = PERSPECTIVE * VIEW * MODEL * vec4(position, 1.0);
    // Passage des coordonnées du sommet de l'espace local à l'espace global
	vs_out.fragPosition = vec3(MODEL * vec4(position, 1.0));
    // Passage de la normale de l'espace local à l'espace global en utilisant
    // la transposée inverse de la partie linéaire de la matrice modèle
    vs_out.Normal = transpose(inverse(mat3(MODEL))) * normal;
}

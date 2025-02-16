#version 450

uniform mat4 MODEL;
uniform mat4 VIEW;
uniform vec3 color;

// Récupération des coordonnées du sommet
layout(location = 0) in vec3 position; // le location permet de dire de quel flux/canal on récupère les données (doit être en accord avec le location du code opengl)
// Récupération de sa normale associée
layout(location = 3) in vec3 normal;

out VS_OUT{
    vec3 Normal;
    vec3 color;
} vs_out;

void main(){
    gl_Position = VIEW * MODEL * vec4(position, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(VIEW * MODEL)));
    vs_out.Normal = normalize(normalMatrix * normal);
    vs_out.color = color;
}

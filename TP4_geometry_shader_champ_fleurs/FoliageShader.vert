#version 450

// Récupération des coordonnées du sommet
layout(location = 0) in vec3 position; // le location permet de dire de quel flux/canal on récupère les données (doit être en accord avec le location du code opengl)

uniform mat4 MODEL;

void main(){
    gl_Position = vec4(position, 1.0);
}
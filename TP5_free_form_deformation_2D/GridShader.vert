#version 450

uniform mat4 MODEL;
uniform mat4 VIEW;
uniform mat4 PERSPECTIVE;

// Récupération des coordonnées du sommet
layout(location = 0) in vec3 position; // le location permet de dire de quel flux/canal on récupère les données (doit être en accord avec le location du code opengl)

void main(){
    gl_Position = PERSPECTIVE * VIEW * MODEL * vec4(position, 1.0);
}
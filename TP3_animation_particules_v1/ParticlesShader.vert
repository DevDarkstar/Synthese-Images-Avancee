#version 450

layout(location = 0) in vec3 position; // le location permet de dire de quel flux/canal on récupère les données (doit être en accord avec le location du code opengl)

void main(){
    gl_Position = vec4(position,1.);
}

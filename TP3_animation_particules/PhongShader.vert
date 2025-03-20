#version 450

uniform mat4 MODEL;

layout(location = 0) in vec3 position; // le location permet de dire de quel flux/canal on récupère les données (doit être en accord avec le location du code opengl)
layout(location = 3) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 4) in vec3 vitesse;

out VS_OUT{
    vec3 fragPosition;
    vec3 fragNormal;
    vec2 fragTexCoord;
    vec3 fragVitesse;
}vs_out;

void main(){
    gl_Position = vec4(position,1.);	
    gl_PointSize = 8; // définie la taille des points mais il faut activer la fonctionnalité dans opengl par glEnable( GL_PROGRAM_POINT_SIZE );
    vs_out.fragPosition =position;
    vs_out.fragNormal = vec3(transpose(inverse(mat3(MODEL))));
    vs_out.fragTexCoord = texCoord ;
    vs_out.fragVitesse = vitesse ;
}

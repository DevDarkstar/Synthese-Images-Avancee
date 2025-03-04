#version 450


uniform mat4 MVP;
uniform mat4 MODEL;
out vec3 fragPosition ;
out vec3 fragNormal ;
out vec2 fragTexCoord ;
out vec3 fragVitesse ;

layout(location = 0) in vec3 position; // le location permet de dire de quel flux/canal on récupère les données (doit être en accord avec le location du code opengl)
layout(location = 3) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 4) in vec3 vitesse;


void main(){

gl_Position = MVP * vec4(position,1.);	
 gl_PointSize = 8; // définie la taille des points mais il faut activer la fonctionnalité dans opengl par glEnable( GL_PROGRAM_POINT_SIZE );
fragPosition =position;
fragNormal = vec3(normal) ;
fragTexCoord = texCoord ;
fragVitesse = vitesse ;
}



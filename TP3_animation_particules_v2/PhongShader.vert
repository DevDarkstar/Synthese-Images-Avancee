#version 450

struct Particule{
  float position[3];
  float vitesse[3];
  float masse;
  float force[3];
};

layout(std430, binding = 0) buffer particulesBuf {
    Particule particules[];
};

vec3 getPosition(int index) {
    return vec3(
    particules[index].position[0],
    particules[index].position[1],
    particules[index].position[2]
    );
}

void main(){
    gl_Position = vec4(getPosition(gl_VertexID),1.);	
    //gl_PointSize = 8; // définie la taille des points mais il faut activer la fonctionnalité dans opengl par glEnable( GL_PROGRAM_POINT_SIZE );
}

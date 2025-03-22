#version 450

struct Particle{
  float position[3];
  float velocity[3];
  float initialVelocity[3];
  float mass;
  float force[3];
  float age;
  float initialAge;
};

layout(std430, binding = 0) buffer particlesBuf {
    Particle particles[];
};

vec3 getPosition(int index) {
    return vec3(
    particles[index].position[0],
    particles[index].position[1],
    particles[index].position[2]
    );
}

void main(){
    gl_Position = vec4(getPosition(gl_VertexID),1.);
}

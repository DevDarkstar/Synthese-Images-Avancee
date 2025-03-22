#version 450

struct Particle{
  float position[3];
  float velocity[3];
  float mass;
  float force[3];
  float radius;
  float color[3];
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

vec3 getColor(int index){
    return vec3(
        particles[index].color[0],
        particles[index].color[1],
        particles[index].color[2]
    );
}

out VS_OUT{
    float radius;
    vec3 color;
}vs_out;

void main(){
    gl_Position = vec4(getPosition(gl_VertexID),1.);
    vs_out.radius = particles[gl_VertexID].radius;
    vs_out.color = getColor(gl_VertexID);
}

#version 450


uniform mat4 MODEL;
uniform mat4 VIEW;
uniform mat4 PERSPECTIVE;

// Récupération des coordonnées du sommet
//layout(location = 0) in vec3 position; // le location permet de dire de quel flux/canal on récupère les données (doit être en accord avec le location du code opengl)
// Récupération de sa normale associée
//layout(location = 3) in vec3 normal;

struct VertexData {
    float position[3];
    float uv[2];
    float normal[3];
};
// readonly SSBO containing the data
layout(binding = 0, std430) buffer vertexData {VertexData data[];};

out VS_OUT{
    vec3 fragPosition;
    vec3 Normal;
} vs_out;

vec3 getPosition(int index) {
    return vec3(
    data[index].position[0],
    data[index].position[1],
    data[index].position[2]
    );
}

vec3 getNormal(int index) {
    return vec3(
    data[index].normal[0],
    data[index].normal[1],
    data[index].normal[2]
    );
}

vec2 getUV(int index) {
    return vec2(
    data[index].uv[0],
    data[index].uv[1]
    );
}

void main(){
    gl_Position = PERSPECTIVE * VIEW * MODEL * vec4(getPosition(gl_VertexID), 1.0);
    // Passage des coordonnées du sommet de l'espace local à l'espace global
	vs_out.fragPosition = vec3(MODEL * vec4(getPosition(gl_VertexID), 1.0));
    // Passage de la normale de l'espace local à l'espace global en utilisant
    // la transposée inverse de la partie linéaire de la matrice modèle
    vs_out.Normal = transpose(inverse(mat3(MODEL))) * getNormal(gl_VertexID);
}

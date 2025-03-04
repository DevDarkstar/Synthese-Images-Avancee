#version 450


uniform mat4 MODEL;
uniform mat4 VIEW;
uniform mat4 PERSPECTIVE;

// Définition de la structure de données correspondant à celle de openGL côté CPU
struct VertexData
{
    float position[3];
    float uv[2];
    float normal[3];
};

// Récupération de l'ensemble des informations des sommets situées dans un SSBO
layout(std430, binding = 0) buffer VerticesData{
    VertexData data[];
};

out VS_OUT{
    vec3 fragPosition;
    vec3 Normal;
} vs_out;

vec3 getPosition(int index) 
{
    return vec3(
        data[index].position[0],
        data[index].position[1],
        data[index].position[2]
    );
}

void main(){
    vec3 position = getPosition(gl_VertexID);
    gl_Position = PERSPECTIVE * VIEW * MODEL * vec4(position, 1.0);
    // Passage des coordonnées du sommet de l'espace local à l'espace global
	vs_out.fragPosition = vec3(MODEL * vec4(position, 1.0));
    // Passage de la normale de l'espace local à l'espace global en utilisant
    // la transposée inverse de la partie linéaire de la matrice modèle
    vs_out.Normal = transpose(inverse(mat3(MODEL))) * vec3(data[gl_VertexID].normal[0], data[gl_VertexID].normal[1], data[gl_VertexID].normal[2]);
}

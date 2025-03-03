#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 12) out; // Pour chaque sommets du terrain, nous créons un plant de végétation

uniform mat4 MODEL;
uniform mat4 VIEW;
uniform mat4 PERSPECTIVE;
uniform int texturesNumber;

// Récupération de l'ensemble des matrices de transformations situées dans un SSBO
layout(std430, binding = 0) buffer TransformationMatrices{
    mat4 transformations[];
};

out GS_OUT{
    vec3 fragPosition; // Coordonnées du sommet dans l'espace global
    vec3 Normal; // Normale au sommet dans l'espace global
    vec2 UVCoords; // Coordonnées uv du sommet
    float TextureIndex; // Indice du feuillet de la texture 3D
}gs_out;

const float PI = 3.14159265358979323846f;

void main()
{
    // Récupération de l'indice du sommet courant du terrain
    int vertexID = gl_PrimitiveIDIn;
    for(int i = 0; i < 3; i++)
    {
        float angle = i * PI / 3;

        // Création des données du premier point
        float x0 = -1.0 * cos(angle);
        float z0 = -1.0 * sin(angle);
        mat4 proj = PERSPECTIVE * VIEW * MODEL * transformations[vertexID];
        mat3 normalMatrix = transpose(inverse(mat3(MODEL)));
        gl_Position = proj * vec4(x0, 0.0, z0, 1.0);
        gs_out.fragPosition = vec3(MODEL * transformations[vertexID] * vec4(x0, 0.0, z0, 1.0));
        gs_out.Normal = normalMatrix * vec3(-cos(angle), 0.0, -sin(angle));
        gs_out.UVCoords = vec2(0.0, 1.0);
        gs_out.TextureIndex = vertexID % texturesNumber;
        // Création du sommet
        EmitVertex();

        // Création des données du second point
        float x1 = 1.0 * cos(angle);
        float z1 = 1.0 * sin(angle);
        gl_Position = proj * vec4(x1, 0.0, z1, 1.0);
        gs_out.fragPosition = vec3(MODEL * transformations[vertexID] * vec4(x1, 0.0, z1, 1.0));
        gs_out.Normal = normalMatrix * vec3(cos(angle), 0.0, sin(angle));
        gs_out.UVCoords = vec2(1.0, 1.0);
        gs_out.TextureIndex = vertexID % texturesNumber;
        // Création du sommet
        EmitVertex();

        // Création des données du troisième point
        gl_Position = proj * vec4(x0, 2.0, z0, 1.0);
        gs_out.fragPosition = vec3(MODEL * transformations[vertexID] * vec4(x0, 0.0, z0, 1.0));
        gs_out.Normal = normalMatrix * vec3(-cos(angle), 0.0, -sin(angle));
        gs_out.UVCoords = vec2(0.0, 0.0);
        gs_out.TextureIndex = vertexID % texturesNumber;
        // Création du sommet
        EmitVertex();

        // Création des données du quatrième point
        gl_Position = proj * vec4(x1, 2.0, z1, 1.0);
        gs_out.fragPosition = vec3(MODEL * transformations[vertexID] * vec4(x1, 0.0, z1, 1.0));
        gs_out.Normal = normalMatrix * vec3(cos(angle), 0.0, sin(angle));
        gs_out.UVCoords = vec2(1.0, 0.0);
        gs_out.TextureIndex = vertexID % texturesNumber;
        // Création du sommet
        EmitVertex();

        // Création de la primitive
        EndPrimitive();
    }
}
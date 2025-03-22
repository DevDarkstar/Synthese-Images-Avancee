#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 200) out;

in VS_OUT{
    float radius;
    vec3 color;
}gs_in[];

out GS_OUT{
    vec3 fragPosition;
    vec3 color;
}gs_out;

uniform mat4 PERSPECTIVE;
uniform mat4 MODEL;
uniform mat4 VIEW;

const float PI = 3.14159265358979323846f;
const int meridians = 7;
const int parallels = 7;

void main()
{
    // Récupération de la position du sommet courant
    vec3 position = gl_in[0].gl_Position.xyz;
    // Génération d'une sphère en se servant du sommet courant comme centre
    for(int j = 0; j < parallels; j++)
    {
        for(int i = 0; i <= meridians; i++)
        {
            vec4 point_position1 = PERSPECTIVE * VIEW * MODEL * vec4(position + gs_in[0].radius*vec3(cos(i*2*PI/meridians)*sin(j*PI/parallels),sin(i*2*PI/meridians)*sin(j*PI/parallels),cos(i*2*PI/meridians)),1.0) ;
            gl_Position = point_position1;
            gs_out.fragPosition = point_position1.xyz;
            gs_out.color = gs_in[0].color;
            EmitVertex();

            vec4 point_position2 = PERSPECTIVE * VIEW * MODEL * vec4(position + gs_in[0].radius*vec3(cos(i*2*PI/meridians)*sin((j+1)*PI/parallels),sin(i*2*PI/meridians)*sin((j+1)*PI/parallels),cos(i*2*PI/meridians)),1.0);
            gl_Position = point_position2;
            gs_out.fragPosition = point_position2.xyz;
            gs_out.color = gs_in[0].color;
            EmitVertex();
        }
        EndPrimitive();
    }  
}

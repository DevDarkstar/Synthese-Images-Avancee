#version 450

layout(points) in;
layout(line_strip, max_vertices = 64) out;

in VS_OUT{
    vec3 fragPosition;
    vec3 fragNormal;
    vec2 fragTexCoord;
    vec3 fragVitesse;
}gs_in[];

out GS_OUT{
    vec3 fragPosition;
    vec3 fragNormal;
    vec2 fragTexCoord;
    vec3 fragVitesse;
}gs_out;

uniform mat4 PERSPECTIVE;
uniform mat4 MODEL;
uniform mat4 VIEW;
uniform float radius;

const float PI = 3.14159265358979323846f;

void main()
{
    // Récupération de la position du sommet
    vec3 position = gl_in[0].gl_Position.xyz;
    
    for(int j = 0; j < 8; j++)
    {
        for(int i = 0; i < 8; i++)
        {
            vec3 point_position = position + radius*vec3(cos(i*2*PI/8)*cos(j*PI/8),cos(i*2*PI/8)*sin(j*PI/8),sin(i*2*PI/8));
            gl_Position = PERSPECTIVE * VIEW * MODEL * vec4(point_position,1.0);
            gs_out.fragPosition = point_position.xyz;
            gs_out.fragNormal = gs_in[0].fragNormal * vec3(point_position - position);
            gs_out.fragTexCoord = gs_in[0].fragTexCoord;
            gs_out.fragVitesse = gs_in[0].fragVitesse;

            EmitVertex();
        }
    }
    EndPrimitive();
}

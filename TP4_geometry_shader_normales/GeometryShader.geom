#version 450

layout(points) in;
layout(line_strip, max_vertices = 2) out;

in VS_OUT{
    vec3 Normal;
    vec3 color;
}gs_in[];

out GS_OUT{
    vec3 color;
}gs_out;

uniform mat4 PERSPECTIVE;

void main()
{
    // Le premier point de la normale correspond aux coordonnées du sommet lui-même
    gl_Position = PERSPECTIVE * gl_in[0].gl_Position;
    gs_out.color = gs_in[0].color;
    // On crée le sommet
    EmitVertex();
    // Le second point correspond à la position du point initial + la direction de la normale au sommet (normalisée)
    gl_Position = PERSPECTIVE * (gl_in[0].gl_Position + vec4(gs_in[0].Normal, 0.0));
    gs_out.color = gs_in[0].color;
    // On crée le sommet
    EmitVertex();
    // On crée la primitive du geometry shader
    EndPrimitive();
}

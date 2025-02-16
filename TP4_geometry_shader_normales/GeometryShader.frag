#version 450

in GS_OUT{
    vec3 color;
}fs_in;

out vec4 FinalColor;

void main(){
    FinalColor = vec4(fs_in.color, 1.0);
}

#version 450

in GS_OUT{
    vec3 fragPosition;
    vec3 color;
}fs_in;


out vec4 finalColor;

void main() {
  finalColor = vec4(fs_in.color, 1.0);
}

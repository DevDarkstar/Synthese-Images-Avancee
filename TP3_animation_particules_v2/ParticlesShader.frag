#version 450

in GS_OUT{
    vec3 fragPosition;
}fs_in;


out vec4 finalColor;

void main() {
  finalColor = vec4((normalize(fs_in.fragPosition) + 1) / 2, 1.0);
}

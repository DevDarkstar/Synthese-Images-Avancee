#version 450
uniform sampler2D checkerboardTexture;

in VS_OUT{
    vec2 UV; // Coordonnées UV du sommet
} fs_in;

out vec4 finalColor;

void main() {
    // Calcul de la couleur de texture induite par le damier
    vec3 checkerboardColor = texture(checkerboardTexture, fs_in.UV).rgb;

    finalColor = vec4(checkerboardColor, 1.0);
}

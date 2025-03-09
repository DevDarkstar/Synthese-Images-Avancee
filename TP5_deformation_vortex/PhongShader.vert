#version 450

uniform mat4 MODEL;
uniform mat4 VIEW;
uniform mat4 PERSPECTIVE;

struct Vortex
{
    float thetaMax;
    float x0;
    float x1;
};

uniform Vortex vortex;

// Récupération des coordonnées du sommet
layout(location = 0) in vec3 position; // le location permet de dire de quel flux/canal on récupère les données (doit être en accord avec le location du code opengl)
// Récupération de sa normale associée
layout(location = 3) in vec3 normal;

out VS_OUT{
    vec3 fragPosition;
    vec3 Normal;
} vs_out;

mat3 computeRotationMatrix(){
    // Calcul du facteur de pondération dans la rotation de type "vortex"
    float factor = exp(-(position.y * position.y + position.z * position.z));
    // Comparaison avec la valeur de x0 et détermination de l'angle de rotation
    float rx;
    if(position.x < vortex.x0){
        rx = 0.0;
    }
    else if(position.x >= vortex.x0 && position.x <= vortex.x1){
        rx = (position.x - vortex.x0) / (vortex.x1 - vortex.x0) * vortex.thetaMax * factor;
    }
    else{
        rx = vortex.thetaMax * factor;
    }
    float cosinus = cos(rx);
    float sinus = sin(rx);

    return transpose(mat3(1.0f, 0.0f, 0.0f,
                0.0f, cosinus, -sinus,
                0.0f, sinus, cosinus));
}

mat3 computeJacobian(){
    // Calcul du facteur de pondération dans la rotation de type "vortex"
    float factor = exp(-(position.y * position.y + position.z * position.z));
    float rxPrime;
    float rx;
    if(position.x < vortex.x0){
        rx = 0.0;
        rxPrime = 0.0;
    }
    else if(position.x >= vortex.x0 && position.x <= vortex.x1){
        rxPrime = vortex.thetaMax / (vortex.x1 - vortex.x0) * factor;
        rx = (position.x - vortex.x0) / (vortex.x1 -vortex.x0) * vortex.thetaMax * factor;
    }
    else{
        rxPrime = 0.0 * factor;
        rx = vortex.thetaMax * factor;
    }

    return transpose(mat3(1.0, 0.0, 0.0,
            -position.y*rxPrime*sin(rx) - position.z*rxPrime*cos(rx), cos(rx), -sin(rx),
            position.y*rxPrime*cos(rx) - position.z*rxPrime*sin(rx), sin(rx), cos(rx)));
}

void main(){
    mat3 rotationMatrix = computeRotationMatrix();
    vec3 result_position = rotationMatrix * position; 
    gl_Position = PERSPECTIVE * VIEW * MODEL * vec4(result_position, 1.0);
    // Passage des coordonnées du sommet de l'espace local à l'espace global
	vs_out.fragPosition = vec3(MODEL * vec4(result_position, 1.0));
    // Passage de la normale de l'espace local à l'espace global en utilisant
    // la transposée inverse de la partie linéaire de la matrice modèle ainsi que la Jacobienne
    mat3 jacobian_matrix = computeJacobian();
    vs_out.Normal = transpose(inverse(jacobian_matrix)) * normal;
}

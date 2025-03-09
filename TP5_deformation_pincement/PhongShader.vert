#version 450

uniform mat4 MODEL;
uniform mat4 VIEW;
uniform mat4 PERSPECTIVE;

struct Tapper
{
    float force;
    float t0;
    float t1;
};

uniform Tapper tapper;

// Récupération des coordonnées du sommet
layout(location = 0) in vec3 position; // le location permet de dire de quel flux/canal on récupère les données (doit être en accord avec le location du code opengl)
// Récupération de sa normale associée
layout(location = 3) in vec3 normal;

out VS_OUT{
    vec3 fragPosition;
    vec3 Normal;
} vs_out;

mat3 computeTapperMatrix(){
    float sx;
    if(position.x < tapper.t0){
        sx = 1.0f;
    }
    else if(position.x >= tapper.t0 && position.x <= tapper.t1){
        sx = 1.0f - tapper.force * ((position.x - tapper.t0) / (tapper.t1 - tapper.t0));
    }
    else{
        sx = tapper.force;
    }

    return transpose(mat3(1.0f, 0.0f, 0.0f,
                0.0f, sx, 0.0f,
                0.0f, 0.0f, sx));
}

mat3 computeJacobian(){
    float sx;
    if(position.x < tapper.t0){
        sx = 1.0f;
    }
    else if(position.x >= tapper.t0 && position.x <= tapper.t1){
        sx = 1.0f - tapper.force * ((position.x - tapper.t0) / (tapper.t1 - tapper.t0));
    }
    else{
        sx = tapper.force;
    }

    return transpose(mat3(1.0f, 0.0f, 0.0f,
            position.y, sx, 0.0f,
            position.z, 0.0f, sx));
}

void main(){
    mat3 tapperMatrix = computeTapperMatrix();
    vec3 result_position = tapperMatrix * position; 
    gl_Position = PERSPECTIVE * VIEW * MODEL * vec4(result_position, 1.0);
    // Passage des coordonnées du sommet de l'espace local à l'espace global
	vs_out.fragPosition = vec3(MODEL * vec4(result_position, 1.0));
    // Passage de la normale de l'espace local à l'espace global en utilisant
    // la transposée inverse de la partie linéaire de la matrice modèle ainsi que la Jacobienne
    mat3 jacobian_matrix = computeJacobian();
    vs_out.Normal = transpose(inverse(jacobian_matrix)) * normal;
}

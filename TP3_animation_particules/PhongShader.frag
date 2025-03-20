#version 450

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord ;
in vec3 fragVitesse ;
uniform mat4 MODEL;


uniform struct Light {
        vec3 position ;
   vec3 intensities;
   float ambientCoefficient;
 //ajout etape 5
      float attenuation;
  //ajout etape 5
} light;

out vec4 finalColor;

void main() {
int nbIter =1500 ;

vec3 colorRes=vec3(5.,0.,0.);

finalColor = vec4( (normalize(fragVitesse)+1.)/2.,.5);


}

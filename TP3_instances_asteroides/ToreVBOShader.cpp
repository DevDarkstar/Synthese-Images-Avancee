/********************************************************/
/*                     CubeVBOShader.cpp                         */
/********************************************************/
/* Premiers pas avec OpenGL.                            */
/* Objectif : afficher a l'ecran uncube avec ou sans shader    */
/********************************************************/

// Les déclarations des variables à envoyer au GPU se font dans la fonction initOpenGL (vers la ligne 225)
// Les valeurs des variables envoyées au GPU se font vers ligne 404 (dans la fonction traceObjet)
/* inclusion des fichiers d'en-tete Glut */
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <math.h>
#include <random>
#include "shader.hpp"
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Include GLM
#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"

using namespace glm;
using namespace std;

#define P_SIZE 3
#define N_SIZE 3		// c'est forcement 3
#define C_SIZE 3

#define N_VERTS  8
#define N_VERTS_BY_FACE  3
#define N_FACES  12

#define PLANET_R 16 // Nombre de parallèles de la planète
#define PLANET_r 16 // Nombre de méridiens de la planète

#define ASTEROID_R 4 // Nombre de parallèles de l'astéroïde
#define ASTEROID_r 3 // Nombre de méridiens de l'astéroïde

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define PI 3.14159265358979323846
#define ASTEROID_RADIUS 1.0
#define ASTEROID_MIN_RADIUS 0.03 // Rayon minimal d'un astéroïde
#define ASTEROID_MAX_RADIUS 0.1 // Rayon maximal d'un astéroïde
#define PLANET_RADIUS 3.0
#define ASTEROID_INSTANCES 2000 // Nombre d'instances des astéroïdes
#define ASTEROID_MIN_POSITION_XY 7.5 // Position minimale en x et en z où trouver des astéroïdes autour de la planète
#define ASTEROID_MAX_POSITION_XY 17.5 // Position maximale en x et en z où trouver des astéroïdes autour de la planète
#define ASTEROID_MIN_POSITION_Z -2.0 // Position minimale en y où trouver des astéroïdes autour de la planète
#define ASTEROID_MAX_POSITION_Z 2.0 // Position maximale en y où trouver des astéroïdes autour de la planète 

// Création d'un tableau de transformations aléatoires pour les instances d'astéroïdes
glm::mat4 asteroid_transformations[ASTEROID_INSTANCES];

// Structure de données de la planète
struct PlanetElements
{
  // Création des mêmes tableaux cette fois pour stocker les informations de la planète
  // Pour les sommets,nous avons 4 points composés chacun de 3 coordonnées
  // Pour les coordonnées de textures, nous avons 2 coordonnées de textures par sommet donc 8 coordonnées au total
  // Pour les normales, nous avons 3 coordonnées de normales par sommets, soit 12 coordonnées au total
  GLfloat sommets[(PLANET_R+1)*(PLANET_r+1)*3];
  GLfloat normales[(PLANET_R+1)*(PLANET_r+1)*3];
  GLfloat coordTexture[(PLANET_R+1)*(PLANET_r+1)*2];
  // Pour les indices des faces, nous avons donc deux faces triangulaires pour former une face quadrangulaire, soit 6 indices
  GLuint indices[(PLANET_R)*(PLANET_r)*6];
  // VAO et VBOs pour la création de la planète
  GLuint VBO_sommets,VBO_normales, VBO_indices,VBO_UVtext,VAO;
};

// Structure de données d'un astéroïde
struct AsteroidElements
{
  GLfloat sommets[(PLANET_R+1)*(PLANET_r+1)*3];
  GLfloat normales[(PLANET_R+1)*(PLANET_r+1)*3];
  GLfloat coordTexture[(PLANET_R+1)*(PLANET_r+1)*2];
  GLuint indices[(ASTEROID_R)*(ASTEROID_r)*6];
  // VAO et VBOs pour la création de la planète
  GLuint VBO_sommets,VBO_normales, VBO_indices,VBO_UVtext,VAO;
};

// initialisations
void genereVBOPlanete(PlanetElements*);
void genereVBOAsteroide(AsteroidElements*);
void genereSSBOAsteroidTransformations();
void deleteVBOPlanete(PlanetElements*);
void deleteVBOAsteroide(AsteroidElements*);
void deleteSSBOAsteroid(void);
void deleteTextures(void);
void traceObjet(void);

// fonctions de rappel de glut
void affichage();
void clavier(unsigned char,int,int);
void mouse(int, int, int, int);
void mouseMotion(int, int);
void reshape(int,int);
// misc
void drawString(const char *str, int x, int y, float color[4], void *font);
void showInfo();
void *font = GLUT_BITMAP_8_BY_13; // pour afficher des textes 2D sur l'ecran
// variables globales pour OpenGL
bool mouseLeftDown;
bool mouseRightDown;
bool mouseMiddleDown;
float mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;
float cameraDistance=0.;

// variables Handle d'opengl
// SSBO pour stocker les matrices de transformations des instances de la végétation
GLuint SSBO_asteroid_transformations;
//--------------------------

// Shader pour l'affichage de la planète
struct PlanetIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locCameraPosition; // Position de la caméra
  GLuint locMaterialShininess; // Brillance de l'objet
  GLuint locMaterialSpecular; // Couleur de la spéculaire
  GLuint locLightPosition ; // Position de la lumière
  GLuint locMaterialAmbient; // Couleur de la lumière ambiante
  GLuint locMaterialDiffuse; // Couleur de la lumière diffuse
  GLuint locAmbientCoefficient; // Coefficient de la lumière ambiante Ka
  GLuint locDiffuseCoefficient; // Coefficient de la lumière diffuse Kd
  GLuint locSpecularCoefficient; // Coefficient de la lumière spéculaire Ks
  GLuint locPlanetTexture; // Texture de la planète
};

// Shader pour l'affichage des astéroïdes
struct AsteroidIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locCameraPosition; // Position de la caméra
  GLuint locMaterialShininess; // Brillance de l'objet
  GLuint locMaterialSpecular; // Couleur de la spéculaire
  GLuint locLightPosition ; // Position de la lumière
  GLuint locMaterialAmbient; // Couleur de la lumière ambiante
  GLuint locMaterialDiffuse; // Couleur de la lumière diffuse
  GLuint locAmbientCoefficient; // Coefficient de la lumière ambiante Ka
  GLuint locDiffuseCoefficient; // Coefficient de la lumière diffuse Kd
  GLuint locSpecularCoefficient; // Coefficient de la lumière spéculaire Ks
  GLuint locAsteroidTexture; // Texture de l'astéroïde
};

// Création des identifiants pour les éléments de la scène
PlanetIDs planetIds;
AsteroidIDs asteroidIds;
PlanetElements planetData;
AsteroidElements asteroidData;

// location des VBO
//------------------
GLuint indexVertex=0, indexUVTexture=2, indexNormale=3;


//variable pour paramétrage eclairage
//--------------------------------------
vec3 cameraPosition(0.,-15.,15.); // Position de la caméra
// le matériau
//---------------
GLfloat materialShininess=2.; // Brillance de l'objet
vec3 materialSpecularColor(0.47,0.71,1.);  // couleur de la spéculaire (bleu ciel)
vec3 materialAmbientColor(0.7,0.7,0.7); // couleur de la lumière ambiante (gris clair)
vec3 materialDiffuseColor(1.,1.,1.); // couleur de la lumière diffuse (blanc)

// la lumière
//-----------
vec3 lightPosition(1.,0.,.5); // Position de la lumière dans la scène
GLfloat Ka = .8; // Coefficient de la lumière ambiante
GLfloat Kd = .9; // Coefficient de la lumière diffuse
GLfloat Ks = .2; // Coefficient de la lumière spéculaire

glm::mat4 MVP;      // justement la voilà
glm::mat4 Model, View, Projection;    // Matrices constituant MVP

// Dimensions de la fenêtre d'affichage
int screenHeight = 500;
int screenWidth = 500;

// Initialisation de la génération de nombres pseudo-aléatoires
std::random_device rd;
// Création du moteur de génération
std::default_random_engine engine(rd());

// pour les textures
//-------------------
// Gestionnaires de texture
GLuint planetTexture; // Gestionnaire de la texture de la planète
GLuint asteroidTexture; // Gestionnaire de la texture de l'astéroïde
// Liens relatifs vers les textures
const char* planetTexFilepath = "./texture/jupiter_texture.jpg";
const char* asteroidTexFilepath = "./texture/asteroid_texture.jpg";

void createPlanet(float radius, PlanetElements* data, int NB_R, int NB_r)
{
  float theta = radians(360.f) / (float)NB_R;
  float phi = radians(180.f) / (float)NB_r;

  float pasU = 1.0f / NB_R;
  float pasV = 1.0f / NB_r;

  for (int i = 0; i <= NB_R; i++) {
    for (int j = 0; j <= NB_r; j++) {
        // Coordonnées des sommets
        float x = radius * sin(j * phi) * cos(i * theta);
        float y = radius * sin(j * phi) * sin(i * theta);
        float z = radius * cos(j * phi);

        data->sommets[(i * (NB_r+1) * 3) + (j * 3)]     = x;
        data->sommets[(i * (NB_r+1) * 3) + (j * 3) + 1] = y;
        data->sommets[(i * (NB_r+1) * 3) + (j * 3) + 2] = z;

        // Normales (identiques aux sommets normalisés)
        data->normales[(i * (NB_r+1) * 3) + (j * 3)]     = sin(j * phi) * cos(i * theta);
        data->normales[(i * (NB_r+1) * 3) + (j * 3) + 1] = sin(j * phi) * sin(i * theta);
        data->normales[(i * (NB_r+1) * 3) + (j * 3) + 2] = cos(j * phi);

        // Coordonnées UV
        data->coordTexture[(i * (NB_r+1) * 2) + (j * 2)]     = (float)i * pasU;
        data->coordTexture[(i * (NB_r+1) * 2) + (j * 2) + 1] = (float)j * pasV;
    }
  }

  // Indices des triangles
  for (int i = 0; i < NB_R; i++) {
    for (int j = 0; j < NB_r; j++) {
      int i0 = (i * (NB_r+1)) + j;
      int i1 = ((i+1) * (NB_r+1)) + j;
      int i2 = ((i+1) * (NB_r+1)) + (j+1);
      int i3 = (i * (NB_r+1)) + j;
      int i4 = ((i+1) * (NB_r+1)) + (j+1);
      int i5 = (i * (NB_r+1)) + (j+1);

      data->indices[(i * NB_r * 6) + (j * 6)]     = (unsigned int)i0;
      data->indices[(i * NB_r * 6) + (j * 6) + 1] = (unsigned int)i1;
      data->indices[(i * NB_r * 6) + (j * 6) + 2] = (unsigned int)i2;
      data->indices[(i * NB_r * 6) + (j * 6) + 3] = (unsigned int)i3;
      data->indices[(i * NB_r * 6) + (j * 6) + 4] = (unsigned int)i4;
      data->indices[(i * NB_r * 6) + (j * 6) + 5] = (unsigned int)i5;

      // Fermeture de la sphère
      if (j == NB_r - 1) {
          data->indices[(i * NB_r * 6) + (j * 6) + 2] = (i+1) * (NB_r+1);
          data->indices[(i * NB_r * 6) + (j * 6) + 5] = i * (NB_r+1);
      }
    }
  }
}

void createAsteroid(float radius, AsteroidElements* data, int NB_R, int NB_r)
{
	{
    float theta = radians(360.f) / (float)NB_R;
    float phi = radians(180.f) / (float)NB_r;

    float pasU = 1.0f / NB_R;
    float pasV = 1.0f / NB_r;

    for (int i = 0; i <= NB_R; i++) {
      for (int j = 0; j <= NB_r; j++) {
          // Coordonnées des sommets
          float x = radius * sin(j * phi) * cos(i * theta);
          float y = radius * sin(j * phi) * sin(i * theta);
          float z = radius * cos(j * phi);

          data->sommets[(i * (NB_r+1) * 3) + (j * 3)]     = x;
          data->sommets[(i * (NB_r+1) * 3) + (j * 3) + 1] = y;
          data->sommets[(i * (NB_r+1) * 3) + (j * 3) + 2] = z;

          // Normales (identiques aux sommets normalisés)
          data->normales[(i * (NB_r+1) * 3) + (j * 3)]     = sin(j * phi) * cos(i * theta);
          data->normales[(i * (NB_r+1) * 3) + (j * 3) + 1] = sin(j * phi) * sin(i * theta);
          data->normales[(i * (NB_r+1) * 3) + (j * 3) + 2] = cos(j * phi);

          // Coordonnées UV
          data->coordTexture[(i * (NB_r+1) * 2) + (j * 2)]     = (float)i * pasU;
          data->coordTexture[(i * (NB_r+1) * 2) + (j * 2) + 1] = (float)j * pasV;
      }
    }

    // Indices des triangles
    for (int i = 0; i < NB_R; i++) {
      for (int j = 0; j < NB_r; j++) {
        int i0 = (i * (NB_r+1)) + j;
        int i1 = ((i+1) * (NB_r+1)) + j;
        int i2 = ((i+1) * (NB_r+1)) + (j+1);
        int i3 = (i * (NB_r+1)) + j;
        int i4 = ((i+1) * (NB_r+1)) + (j+1);
        int i5 = (i * (NB_r+1)) + (j+1);

        data->indices[(i * NB_r * 6) + (j * 6)]     = (unsigned int)i0;
        data->indices[(i * NB_r * 6) + (j * 6) + 1] = (unsigned int)i1;
        data->indices[(i * NB_r * 6) + (j * 6) + 2] = (unsigned int)i2;
        data->indices[(i * NB_r * 6) + (j * 6) + 3] = (unsigned int)i3;
        data->indices[(i * NB_r * 6) + (j * 6) + 4] = (unsigned int)i4;
        data->indices[(i * NB_r * 6) + (j * 6) + 5] = (unsigned int)i5;

        // Fermeture de la sphère
        if (j == NB_r - 1) {
            data->indices[(i * NB_r * 6) + (j * 6) + 2] = (i+1) * (NB_r+1);
            data->indices[(i * NB_r * 6) + (j * 6) + 5] = i * (NB_r+1);
        }
      }
    }
  }
}

//----------------------------------------
void initTextureRGB(const char* filepath, GLuint* handler)
//-----------------------------------------
{
 int iwidth,iheight,nrChannels;
  //GLubyte *  image = NULL;
  unsigned char* image = NULL;
  image = stbi_load(filepath, &iwidth, &iheight, &nrChannels, 0);
  //Initialisation de la texture
	glGenTextures(1, handler);
  // Utilisation de la texture 2D
	glBindTexture(GL_TEXTURE_2D, *handler);
  // Affectation de ses paramètres
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // Remplissage de la texture à partir du contenu de la variable "image" chargée
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, iwidth,iheight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

  // Désactivation de la texture une fois le paramétrage effectué
  glBindTexture(GL_TEXTURE_2D, 0); 

  stbi_image_free(image);
}

void initAsteroidTranslations(void){
  // Génération aléatoire des translations
  // Création de la méthode de génération des translations en X des astéroïdes (suivant une distribution normale)
  std::uniform_real_distribution<float> positionXY(ASTEROID_MIN_POSITION_XY, ASTEROID_MAX_POSITION_XY);
  // Ainsi que la position en Y
  std::uniform_real_distribution<float> positionZ(ASTEROID_MIN_POSITION_Z, ASTEROID_MAX_POSITION_Z);
  // Création d'une méthode de génération d'un angle compris entre 0 et 2*PI
  std::uniform_real_distribution<float> angle(0.0f, 2*PI);

  // Remplissage du tableau des translations des instances
  for(int i = 0; i < ASTEROID_INSTANCES; i++)
  {
    float position_XY = positionXY(engine);
    float position_Z = positionZ(engine);
    float ang = angle(engine);
    asteroid_transformations[i] = glm::translate(glm::mat4(1.0f), glm::vec3(position_XY*cos(ang), position_XY*sin(ang), position_Z));
  }
}

void initAsteroidScales(void){
  // Création de la méthode de génération des niveaux d'échelles des astéroïdes (suivant une distribution normale)
  std::uniform_real_distribution<float> scale(ASTEROID_MIN_RADIUS, ASTEROID_MAX_RADIUS);
  for(int i = 0; i < ASTEROID_INSTANCES; i++){
    asteroid_transformations[i] = glm::scale(asteroid_transformations[i], glm::vec3(scale(engine), scale(engine), scale(engine)));
  }
}

void initAsteroidRotations(void){
  // Création de la méthode de génération de valeur d'angle aléatoire comprise entre 0 et Pi/3 
  std::uniform_real_distribution<float> rotation(0.0f, 2*PI);
  for(int i = 0; i < ASTEROID_INSTANCES; i++)
  {
    //tirage d'une valeur d'angle aléatoire comprise entre 0 et 2*PI
    float angle = rotation(engine);
    //Création de la matrice de rotation adéquate
    asteroid_transformations[i] = glm::rotate(asteroid_transformations[i], angle, glm::vec3(0.0,1.0,0.0));
  }
}

// Récupération des emplacements des variables uniformes pour l'affichage de la planète
void getUniformLocationPlanet(PlanetIDs& planet){
  //Chargement des vertex et fragment shaders pour l'affichage de la planète
  planet.programID = LoadShaders("PlanetShader.vert", "PlanetShader.frag");
 
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  planet.MatrixIDView = glGetUniformLocation(planet.programID, "VIEW");
  planet.MatrixIDModel = glGetUniformLocation(planet.programID, "MODEL");
  planet.MatrixIDPerspective = glGetUniformLocation(planet.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables uniformes du shader de Phong
  planet.locCameraPosition = glGetUniformLocation(planet.programID, "cameraPosition");
  planet.locAmbientCoefficient = glGetUniformLocation(planet.programID, "Ka");
  planet.locDiffuseCoefficient = glGetUniformLocation(planet.programID, "Kd");
  planet.locSpecularCoefficient = glGetUniformLocation(planet.programID, "Ks");
  planet.locMaterialAmbient = glGetUniformLocation(planet.programID, "material.ambient");
  planet.locLightPosition = glGetUniformLocation(planet.programID, "lightPosition");
  planet.locMaterialDiffuse = glGetUniformLocation(planet.programID, "material.diffuse");
  planet.locMaterialShininess = glGetUniformLocation(planet.programID, "material.shininess");
  planet.locMaterialSpecular = glGetUniformLocation(planet.programID, "material.specular");
  planet.locPlanetTexture = glGetUniformLocation(planet.programID, "planetTexture");
}

// Récupération des emplacements des variables uniformes pour l'affichage des astéroïdes
void getUniformLocationAsteroid(AsteroidIDs& asteroid){
  //Chargement des vertex et fragment shaders pour l'affichage des astéroïdes
  asteroid.programID = LoadShaders("AsteroidShader.vert", "AsteroidShader.frag");
 
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  asteroid.MatrixIDView = glGetUniformLocation(asteroid.programID, "VIEW");
  asteroid.MatrixIDModel = glGetUniformLocation(asteroid.programID, "MODEL");
  asteroid.MatrixIDPerspective = glGetUniformLocation(asteroid.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables uniformes du shader de Phong
  asteroid.locCameraPosition = glGetUniformLocation(asteroid.programID, "cameraPosition");
  asteroid.locAmbientCoefficient = glGetUniformLocation(asteroid.programID, "Ka");
  asteroid.locDiffuseCoefficient = glGetUniformLocation(asteroid.programID, "Kd");
  asteroid.locSpecularCoefficient = glGetUniformLocation(asteroid.programID, "Ks");
  asteroid.locMaterialAmbient = glGetUniformLocation(asteroid.programID, "material.ambient");
  asteroid.locLightPosition = glGetUniformLocation(asteroid.programID, "lightPosition");
  asteroid.locMaterialDiffuse = glGetUniformLocation(asteroid.programID, "material.diffuse");
  asteroid.locMaterialShininess = glGetUniformLocation(asteroid.programID, "material.shininess");
  asteroid.locMaterialSpecular = glGetUniformLocation(asteroid.programID, "material.specular");
  asteroid.locAsteroidTexture = glGetUniformLocation(asteroid.programID, "asteroidTexture");
}

//----------------------------------------
void initOpenGL(void)
//----------------------------------------
{
  glCullFace (GL_BACK); // on spécifie queil faut éliminer les face arriere
  glDisable(GL_CULL_FACE); // on active l'élimination des faces qui par défaut n'est pas active
  glEnable(GL_DEPTH_TEST);

  // Récupération des emplacements des variables uniformes pour le shader de visualisation de la planète
  getUniformLocationPlanet(planetIds);
  // Récupération des emplacements des variables uniformes pour le shader de visualisation des astéroïdes
  getUniformLocationAsteroid(asteroidIds);

  // Projection matrix : 65 Field of View, 1:1 ratio, display range : 1 unit <-> 1000 units
  // ATTENTIOn l'angle est donné en radians si f GLM_FORCE_RADIANS est défini sinon en degré
  Projection = glm::perspective( glm::radians(60.f), 1.0f, 1.0f, 1000.0f);

}
//----------------------------------------
int main(int argc,char **argv)
//----------------------------------------
{

  /* initialisation de glut et creation
     de la fenetre */

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE|GLUT_RGB);
  glutInitWindowPosition(200,200);
  glutInitWindowSize(screenWidth,screenHeight);
  glutCreateWindow("TP INSTANCES ASTEROIDES");


// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

//info version GLSL
  std::cout << "***** Info GPU *****" << std::endl;
  std::cout << "Fabricant : " << glGetString (GL_VENDOR) << std::endl;
  std::cout << "Carte graphique: " << glGetString (GL_RENDERER) << std::endl;
  std::cout << "Version : " << glGetString (GL_VERSION) << std::endl;
  std::cout << "Version GLSL : " << glGetString (GL_SHADING_LANGUAGE_VERSION) << std::endl << std::endl;

	initOpenGL(); 

  // Remplissage des tableaux des données de la planète
  createPlanet(PLANET_RADIUS, &planetData, PLANET_R, PLANET_r);
  // Remplissage des tableaux de données de l'astéroïde
  // Remplissage des tableaux de données du support pour la végétation
  createAsteroid(ASTEROID_RADIUS, &asteroidData, ASTEROID_R, ASTEROID_r);
  // Génération de la position aléatoire des astéroïdes
  initAsteroidTranslations(); 
  // Et la rotation de ces derniers
  initAsteroidRotations();
  // Tout comme la mise à l'échelle des plans
  initAsteroidScales();

  //Initialisation des textures utilisées dans le programme
  initTextureRGB(planetTexFilepath, &planetTexture);
  initTextureRGB(asteroidTexFilepath, &asteroidTexture);

  // construction des VBO à partir des tableaux du plan
  genereVBOPlanete(&planetData);
  // construction des VBO à partir des tableaux de données des astéroïdes
  genereVBOAsteroide(&asteroidData);
  // Construction du SSBO permettant d'envoyer les matrices de transformations à chaque instance des astéroïdes
  genereSSBOAsteroidTransformations();
  
  /* enregistrement des fonctions de rappel */
  glutDisplayFunc(affichage);
  glutKeyboardFunc(clavier);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotion);

  /* Entree dans la boucle principale glut */
  glutMainLoop();
  return 0;
}

void genereVBOPlanete(PlanetElements* data){
  if(glIsBuffer(data->VBO_sommets) == GL_TRUE) glDeleteBuffers(1, &data->VBO_sommets);
  glGenBuffers(1, &data->VBO_sommets);
  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_sommets);
  glBufferData(GL_ARRAY_BUFFER, sizeof(data->sommets),data->sommets , GL_STATIC_DRAW);

  if(glIsBuffer(data->VBO_normales) == GL_TRUE) glDeleteBuffers(1, &data->VBO_normales);
  glGenBuffers(1, &data->VBO_normales);
  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_normales);
  glBufferData(GL_ARRAY_BUFFER, sizeof(data->normales),data->normales , GL_STATIC_DRAW);

  if(glIsBuffer(data->VBO_indices) == GL_TRUE) glDeleteBuffers(1, &data->VBO_indices);
  glGenBuffers(1, &data->VBO_indices); // ATTENTIOn IBO doit etre un GL_ELEMENT_ARRAY_BUFFER
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->VBO_indices);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data->indices),data->indices , GL_STATIC_DRAW);

  if(glIsBuffer(data->VBO_UVtext) == GL_TRUE) glDeleteBuffers(1, &data->VBO_UVtext);
  glGenBuffers(1, &data->VBO_UVtext);
  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_UVtext);
  glBufferData(GL_ARRAY_BUFFER, sizeof(data->coordTexture),data->coordTexture , GL_STATIC_DRAW);

  glGenVertexArrays(1, &data->VAO);
  glBindVertexArray(data->VAO); // ici on bind le VAO , c'est lui qui recupèrera les configurations des VBO glVertexAttribPointer , glEnableVertexAttribArray...
  glEnableVertexAttribArray(indexVertex);
  glEnableVertexAttribArray(indexNormale );
  glEnableVertexAttribArray(indexUVTexture);

  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_sommets);
  glVertexAttribPointer (indexVertex, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_normales);
  glVertexAttribPointer (indexNormale, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_UVtext);
  glVertexAttribPointer (indexUVTexture, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->VBO_indices);

  // une fois la config terminée   
  // on désactive le dernier VBO et le VAO pour qu'ils ne soit pas accidentellement modifié 
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void genereVBOAsteroide(AsteroidElements* data){
  if(glIsBuffer(data->VBO_sommets) == GL_TRUE) glDeleteBuffers(1, &data->VBO_sommets);
  glGenBuffers(1, &data->VBO_sommets);
  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_sommets);
  glBufferData(GL_ARRAY_BUFFER, sizeof(data->sommets),data->sommets , GL_STATIC_DRAW);

  if(glIsBuffer(data->VBO_normales) == GL_TRUE) glDeleteBuffers(1, &data->VBO_normales);
  glGenBuffers(1, &data->VBO_normales);
  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_normales);
  glBufferData(GL_ARRAY_BUFFER, sizeof(data->normales),data->normales , GL_STATIC_DRAW);

  if(glIsBuffer(data->VBO_indices) == GL_TRUE) glDeleteBuffers(1, &data->VBO_indices);
  glGenBuffers(1, &data->VBO_indices); // ATTENTIOn IBO doit etre un GL_ELEMENT_ARRAY_BUFFER
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->VBO_indices);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data->indices),data->indices , GL_STATIC_DRAW);

  if(glIsBuffer(data->VBO_UVtext) == GL_TRUE) glDeleteBuffers(1, &data->VBO_UVtext);
  glGenBuffers(1, &data->VBO_UVtext);
  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_UVtext);
  glBufferData(GL_ARRAY_BUFFER, sizeof(data->coordTexture),data->coordTexture , GL_STATIC_DRAW);

  glGenVertexArrays(1, &data->VAO);
  glBindVertexArray(data->VAO); // ici on bind le VAO , c'est lui qui recupèrera les configurations des VBO glVertexAttribPointer , glEnableVertexAttribArray...
  glEnableVertexAttribArray(indexVertex);
  glEnableVertexAttribArray(indexNormale );
  glEnableVertexAttribArray(indexUVTexture);

  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_sommets);
  glVertexAttribPointer (indexVertex, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_normales);
  glVertexAttribPointer (indexNormale, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, data->VBO_UVtext);
  glVertexAttribPointer (indexUVTexture, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->VBO_indices);

  // une fois la config terminée   
  // on désactive le dernier VBO et le VAO pour qu'ils ne soit pas accidentellement modifié 
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void genereSSBOAsteroidTransformations(void){
  // Afin de pouvoir envoyer des matrices de transformations différentes à chaque instance de la végétation,
  // nous allons utiliser un SSBO (Shader Storage Buffer Object)
  // Initialisation du SSBO
  glGenBuffers(1, &SSBO_asteroid_transformations);
  // Utilisation du SSBO
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_asteroid_transformations);
  // Affectation des données des matrices de transformations au SSBO
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(asteroid_transformations), asteroid_transformations, GL_DYNAMIC_DRAW);
  // On effectue le lien entre le SSBO et le point de binding 0 dans le shader
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO_asteroid_transformations);
}


void deleteVBOPlanete(PlanetElements* data)
{
  glDeleteBuffers(1, &data->VBO_sommets);
  glDeleteBuffers(1, &data->VBO_normales);
  glDeleteBuffers(1, &data->VBO_UVtext);
  glDeleteBuffers(1, &data->VBO_indices);
  glDeleteBuffers(1, &data->VAO);
}

void deleteVBOAsteroide(AsteroidElements* data)
{
  glDeleteBuffers(1, &data->VBO_sommets);
  glDeleteBuffers(1, &data->VBO_normales);
  glDeleteBuffers(1, &data->VBO_UVtext);
  glDeleteBuffers(1, &data->VBO_indices);
  glDeleteBuffers(1, &data->VAO);
}

void deleteSSBOAsteroid(void)
{
  glDeleteBuffers(1, &SSBO_asteroid_transformations);
}

void deleteTextures(void)
{
  glDeleteTextures(1, &planetTexture);
  glDeleteTextures(1, &asteroidTexture);
}

/* fonction d'affichage */
void affichage()
{

  /* effacement de l'image avec la couleur de fond */
 /* Initialisation d'OpenGL */
  glClearColor(0.0,0.0,0.0,1.0);
  glClearDepth(10.0f);                         // 0 is near, >0 is far
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glColor3f(1.0,1.0,1.0);
  glPointSize(2.0);
 
  View       = glm::lookAt(cameraPosition, // Camera is at (0,0,3), in World Space
                          glm::vec3(0,0,0), // and looks at the origin
                          glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                          );
  Model = glm::mat4(1.0f);
  Model = glm::translate(Model,glm::vec3(0,0,cameraDistance));
  Model = glm::rotate(Model,glm::radians(cameraAngleX),glm::vec3(1, 0, 0) );
  Model = glm::rotate(Model,glm::radians(cameraAngleY),glm::vec3(0, 1, 0) );
  Model = glm::scale(Model,glm::vec3(.8, .8, .8));
  MVP = Projection * View * Model;

  traceObjet();// trace VBO avec ou sans shader

 /* on force l'affichage du resultat */
   glutPostRedisplay();
   glutSwapBuffers();
}

// Affectation de valeurs pour les variables uniformes du shader gérant l'affichage de la planète
void setPlanetUniformValues(PlanetIDs& planet){
  //on envoie les données necessaires aux shaders
  glUniformMatrix4fv(planet.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(planet.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(planet.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);

  glUniform3f(planet.locCameraPosition,cameraPosition.x, cameraPosition.y, cameraPosition.z);
  glUniform1f(planet.locAmbientCoefficient, Ka);
  glUniform1f(planet.locDiffuseCoefficient, Kd);
  glUniform3f(planet.locMaterialAmbient, materialAmbientColor.r, materialAmbientColor.g, materialAmbientColor.b);
  glUniform3f(planet.locLightPosition,lightPosition.x,lightPosition.y,lightPosition.z);
  glUniform3f(planet.locMaterialDiffuse, materialDiffuseColor.r, materialDiffuseColor.g, materialDiffuseColor.b);
  glUniform1f(planet.locMaterialShininess, materialShininess);
  glUniform3f(planet.locMaterialSpecular, materialSpecularColor.r,materialSpecularColor.g,materialSpecularColor.b);
  glUniform1f(planet.locSpecularCoefficient, Ks);
}

// Affectation de valeurs pour les variables uniformes du shader gérant l'affichage des astéroïdes
void setAsteroidUniformValues(AsteroidIDs& asteroid){
  //on envoie les données necessaires aux shaders
  glUniformMatrix4fv(asteroid.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(asteroid.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(asteroid.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);

  glUniform3f(asteroid.locCameraPosition,cameraPosition.x, cameraPosition.y, cameraPosition.z);
  glUniform1f(asteroid.locAmbientCoefficient, Ka);
  glUniform1f(asteroid.locDiffuseCoefficient, Kd);
  glUniform3f(asteroid.locMaterialAmbient, materialAmbientColor.r, materialAmbientColor.g, materialAmbientColor.b);
  glUniform3f(asteroid.locLightPosition,lightPosition.x,lightPosition.y,lightPosition.z);
  glUniform3f(asteroid.locMaterialDiffuse, materialDiffuseColor.r, materialDiffuseColor.g, materialDiffuseColor.b);
  glUniform1f(asteroid.locMaterialShininess, materialShininess);
  glUniform3f(asteroid.locMaterialSpecular, materialSpecularColor.r,materialSpecularColor.g,materialSpecularColor.b);
  glUniform1f(asteroid.locSpecularCoefficient, Ks);
}

void tracePlanete()
{
	glBindVertexArray(planetData.VAO); // on active le VAO
  glDrawElements(GL_TRIANGLES,  sizeof(planetData.indices) / sizeof(GLuint), GL_UNSIGNED_INT, 0);// on appelle la fonction dessin 
}

void traceAsteroide()
{
  glBindVertexArray(asteroidData.VAO); // on active le VAO
  glDrawElementsInstanced(GL_TRIANGLES, sizeof(asteroidData.indices) / sizeof(GLuint), GL_UNSIGNED_INT, 0, ASTEROID_INSTANCES);// on appelle la fonction dessin 
}

//-------------------------------------
//Trace le tore 2 via le VAO (pour le shader d'environment map et le mix entre shader de Toon, environment map et texture classique)
void traceObjet()
//-------------------------------------
{
  // Affichage de la planète
  // utilisation du shader program adéquat
  glUseProgram(planetIds.programID);
  // Affectation des valeurs aux variables uniformes dédiées dans le shader
  setPlanetUniformValues(planetIds);
  // Activation d'une unité de texture pour la texture de la planète
  glActiveTexture(GL_TEXTURE0);
  // Utilisation de la texture de terrain
  glBindTexture(GL_TEXTURE_2D, planetTexture);
  // affectation de la texture au shader
  glUniform1i(planetIds.locPlanetTexture, 0);
  // Affichage de la planète
  tracePlanete();

  // Affichage des astéroïdes
  // Utilisation du shader program adéquat
  glUseProgram(asteroidIds.programID);
  // Affectation des valeurs aux variables uniformes dédiées dans le shader
  setAsteroidUniformValues(asteroidIds);
  // Activation d'une autre unité de texture pour la texture de la végétation
  glActiveTexture(GL_TEXTURE1);
  // Utilisation de la texture de l'astéroïde
  glBindTexture(GL_TEXTURE_2D, asteroidTexture);
  // Affectation de la texture à la variable uniforme correspondante
  glUniform1i(asteroidIds.locAsteroidTexture, 1);
  // Affichage de la végétation
  traceAsteroide();
}

void reshape(int w, int h)
{
    // set viewport to be the entire window
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);// ATTENTION GLsizei important - indique qu'il faut convertir en entier non négatif

    // set perspective viewing frustum
    float aspectRatio = (float)w / h;

        Projection = glm::perspective(glm::radians(60.0f),(float)(w)/(float)h, 1.0f, 1000.0f);
}


void clavier(unsigned char touche,int x,int y)
{
  switch (touche)
  {
    case 'f': /* affichage du carre plein */
      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
      glutPostRedisplay();
      break;
    case 'e': /* affichage en mode fil de fer */
      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
      glutPostRedisplay();
      break;
    case 'v' : /* Affichage en mode sommets seuls */
      glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);
      glutPostRedisplay();
      break;
    case 's' : /* Affichage en mode sommets seuls */
      materialShininess-=.5;
      glutPostRedisplay();
      break;
    case 'S' : /* Affichage en mode sommets seuls */
      materialShininess+=.5;
      glutPostRedisplay();
      break;
    case 'x' : /* Affichage en mode sommets seuls */
      lightPosition.x-=.2;
      glutPostRedisplay();
      break;
    case 'X' : /* Affichage en mode sommets seuls */
      lightPosition.x+=.2;
      glutPostRedisplay();
      break;
    case 'y' : /* Affichage en mode sommets seuls */
      lightPosition.y-=.2;
      glutPostRedisplay();
      break;
    case 'Y' : /* Affichage en mode sommets seuls */
      lightPosition.y+=.2;
      glutPostRedisplay();
      break;
    case 'z' : /* Affichage en mode sommets seuls */
      lightPosition.z-=.2;
      glutPostRedisplay();
      break;
    case 'Z' : /* Affichage en mode sommets seuls */
      lightPosition.z+=.2;
      glutPostRedisplay();
      break;
    case 'a' : /* Affichage en mode sommets seuls */
      Ka-=.1;
      glutPostRedisplay();
      break;
    case 'A' : /* Affichage en mode sommets seuls */
      Ka+=.1;
      glutPostRedisplay();
      break;    
    case 'q' : /*la touche 'q' permet de quitter le programme */
      std::cout << "Désactivation du VAO actif ainsi que le SSBO...\n";
      glBindVertexArray(0);
      // Désactivation du SSBO ainsi que le lien dans le shader
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
      std::cout << "Désactivation du shader program actif ainsi que les textures utilisées pour les rendus...\n";
      glUseProgram(0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, 0);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, 0);
      std::cout << "Suppression des éléments du programme...\n";
      // Suppression des shader programs
      glDeleteProgram(planetIds.programID);
      glDeleteProgram(asteroidIds.programID);
      // Ainsi que des VAO, VBOs, SSBO et textures utilisées dans le programme
      deleteVBOPlanete(&planetData);
      deleteVBOAsteroide(&asteroidData);
      deleteSSBOAsteroid();
      deleteTextures();
      std::cout << "Désactivations et suppressions terminées..." << std::endl;

      exit(0);
  }
}



void mouse(int button, int state, int x, int y)
{
    mouseX = x;
    mouseY = y;

    if(button == GLUT_LEFT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseLeftDown = true;
        }
        else if(state == GLUT_UP)
            mouseLeftDown = false;
    }

    else if(button == GLUT_RIGHT_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseRightDown = true;
        }
        else if(state == GLUT_UP)
            mouseRightDown = false;
    }

    else if(button == GLUT_MIDDLE_BUTTON)
    {
        if(state == GLUT_DOWN)
        {
            mouseMiddleDown = true;
        }
        else if(state == GLUT_UP)
            mouseMiddleDown = false;
    }
}


void mouseMotion(int x, int y)
{
    if(mouseLeftDown)
    {
        cameraAngleY += (x - mouseX);
        cameraAngleX += (y - mouseY);
        mouseX = x;
        mouseY = y;
    }
    if(mouseRightDown)
    {
        cameraDistance += (y - mouseY) * 0.2f;
        mouseY = y;
    }

    glutPostRedisplay();
}

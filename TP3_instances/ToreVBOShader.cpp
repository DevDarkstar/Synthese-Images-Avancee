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
#include "../glm/gtc/type_ptr.hpp"

using namespace glm;
using namespace std;

#define P_SIZE 3
#define N_SIZE 3		// c'est forcement 3
#define C_SIZE 3

#define N_VERTS  8
#define N_VERTS_BY_FACE  3
#define N_FACES  12

#define PLAN_R 5
#define PLAN_r 5
#define TERRAIN_WIDTH 16.0 // Longueur du terrain
#define TERRAIN_HEIGHT 16.0 // Largeur du terrain
#define TEXTURE_NUMBER 8
#define ATLAS_WIDTH 512 // Largeur de l'atlas de texture
#define ATLAS_HEIGHT 256 // hauteur de l'atlas de texture
#define ATLAS_ROW 2 // nombre de textures par ligne de l'atlas
#define ATLAS_COLUMN 4 // nombre de textures par colonne de l'atlas

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define PI 3.14159265358979323846
#define FOLIAGE_PLANE_NUMBER 3 // Nombre de plans utilisés pour afficher une plante
#define SUPPORT_RADIUS 1.0
#define SUPPORT_MIN_RADIUS 0.2 // Rayon minimal de chaque plan de la végétation
#define SUPPORT_MAX_RADIUS 0.7 // Rayon minimal de chaque plan de la végétation
#define FOLIAGE_INSTANCES 50 // Nombre d'instances de végétation

// Création des mêmes tableaux cette fois pour stocker les informations d'un plan
// Pour les sommets,nous avons 4 points composés chacun de 3 coordonnées
// Pour les coordonnées de textures, nous avons 2 coordonnées de textures par sommet donc 8 coordonnées au total
// Pour les normales, nous avons 3 coordonnées de normales par sommets, soit 12 coordonnées au total
GLfloat sommets_terrain[(PLAN_R+1)*(PLAN_r+1)*8]; 
// Pour les indices des faces, nous avons donc deux faces triangulaires pour former un plan, soit 6 indices
GLuint indices_terrain[(PLAN_R)*(PLAN_r)*6];

// Définition des tableaux de données pour les supports floraux
// Chaque support est consitué de 3 plans, soient 12 points et chaque point est constitué de 3 coordonnées de position,
// 3 coordonnées de normales, soit un total de 12 * (3 + 3) = 72 valeurs
GLfloat sommets_support[72];
// Pour le tableau d'indices, nous avons 3 plans pour représenter un support. Chaque plan peut se diviser en 2 triangles
// et nous avons besoin de 3 indices pour tracer un triangle. Par conséquent, le nombre total de données est de 3 plans * 2 triangles * 3 indices = 18 valeurs
GLuint indices_support[18];
// Création d'un tableau de transformations aléatoires pour les instances de végétations
glm::mat4 foliage_transformations[FOLIAGE_INSTANCES];
// Création d'un tableau contenant les coordonnées UV permettant d'afficher une des 8 textures présentes dans
// l'atlas sur un plant (il y a 8 textures, chaque plant est composé de 12 sommets, et nous avons de 2 coordonnées UV pour localiser la position de la texture) 
GLfloat foliage_uv_coordinates[12*2*TEXTURE_NUMBER];

// initialisations
void genereVBOTerrain();
void genereVBOFoliageSupport();
void deleteVBOTerrain();
void deleteVBOFoliageSupport();
void traceObjet();

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
// VAO et VBOs pour la création du plan
GLuint VBO_sommets_terrain, VBO_indices_terrain, VAO_terrain;
// VAO et VBO pour la création du support de la végétation
GLuint VBO_sommets_vegetation, VBO_indices_vegetation, VBO_uv_coordinates, VAO_vegetation;
//--------------------------

// Shader pour l'affichage du terrain
struct TerrainIDs{
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
  GLuint locTerrainTexture; // Texture du terrain
};

// Shader pour l'affichage de la végétation
struct FoliageIDs{
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
  GLuint locFoliageTexture; // Texture de végétation
  GLuint locFoliageTransformation; // Tableau contenant l'ensemble des rotations des instances de la végétation
};

TerrainIDs terrainIds;
FoliageIDs foliageIds;

// location des VBO
//------------------
GLuint indexVertex=0, indexUVTexture=2, indexNormale=3;


//variable pour paramétrage eclairage
//--------------------------------------
vec3 cameraPosition(0.,1.,5.); // Position de la caméra
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
glm::mat4 LightSpaceMatrix; // Matrice de l'espace de la lumière

GLfloat textureMix = 0.5f; // Paramètre de mixage entre une texture classique et une texture type skybox
GLfloat toonMix = 0.5f;
GLuint shaderType; // type de shader actif (permet de contrôler le shader à afficher à l'écran)
// 0 = Environment Map, 1 = Mix Environment Map, Texture simple et shader Toon, 2 = Effet d'ombrage

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
GLuint terrainTexture; // Gestionnaire de la texture du terrain
GLuint foliageTexture; // Gestionnaire de la texture de végétation
// Liens relatifs vers les textures
const char* terrainTexFilepath = "./texture/terrain.jpg";
const char* foliageTexFilepath = "./texture/grass_atlas.png";

void createTerrain()
{
  // Remplissage des coordonnées des sommets du plan
  for(int i = 0; i < PLAN_R; i++)
  {
    for (int j = 0; j < PLAN_r; j++)
    {
      int index = (i * PLAN_r + j) * 8;
      // Coordonnées des sommets
      sommets_terrain[index] = (j * TERRAIN_WIDTH / (PLAN_r - 1)) - (TERRAIN_WIDTH / 2);
      sommets_terrain[index+1] = 0.0f;
      sommets_terrain[index+2] = (i * TERRAIN_HEIGHT / (PLAN_R - 1)) - (TERRAIN_HEIGHT / 2);
      // Normales
      sommets_terrain[index+3] = 0.0f;
      sommets_terrain[index+4] = 1.0f;
      sommets_terrain[index+5] = 0.0f;
      // Coordonnées de texture
      sommets_terrain[index+6] = (GLfloat)i / (GLfloat)(PLAN_R - 1);
      sommets_terrain[index+7] = (GLfloat)j / (GLfloat)(PLAN_r - 1);
    }
  } 

  for(int i = 0; i < PLAN_R - 1; i++)
  {
    for(int j = 0; j < PLAN_r - 1; j++)
    {
      int index = (i * PLAN_r + j) * 6;
      indices_terrain[index]= (unsigned int)(i*(PLAN_r) + j); 
      indices_terrain[index+1]=(unsigned int)((i+1)*PLAN_r + j);
      indices_terrain[index+2]=(unsigned int)((i+1)*PLAN_r + (j+1));
      indices_terrain[index+3]=(unsigned int)(i*(PLAN_r) + j);
      indices_terrain[index+4]=(unsigned int)((i+1)*(PLAN_r) + (j+1));
      indices_terrain[index+5]=(unsigned int)(i*(PLAN_r) + (j+1));
    }
  }
}

void createFoliageSupport()
{
  for(int i = 0; i < FOLIAGE_PLANE_NUMBER; i++){
    // Calcul de l'angle du plan dans l'espace
    GLfloat angle = i * PI / 3.0f;
    // Génération des points
    // Coordonnées de positions du premier point
    sommets_support[i*4*6] = SUPPORT_RADIUS * cos(angle);
    sommets_support[i*4*6 + 1] = 2*SUPPORT_RADIUS;
    sommets_support[i*4*6 + 2] = SUPPORT_RADIUS * sin(angle);

    // Coordonnées de normales du premier point
    sommets_support[i*4*6 + 3] = cos(angle);
    sommets_support[i*4*6 + 4] = 0.0f;
    sommets_support[i*4*6 + 5] = sin(angle);

    // Coordonnées de texture du premier point
    //sommets_support[i*4*8 + 6] = 0.0f;
    //sommets_support[i*4*8 + 7] = 0.0f;

    // Nous faisons de même pour les 3 autres points du plan
    // deuxième point
    sommets_support[i*4*6 + 6] = - SUPPORT_RADIUS * cos(angle);
    sommets_support[i*4*6 + 7] = 2*SUPPORT_RADIUS;
    sommets_support[i*4*6 + 8] = - SUPPORT_RADIUS * sin(angle);

    sommets_support[i*4*6 + 9] = -cos(angle);
    sommets_support[i*4*6 + 10] = 0.0f;
    sommets_support[i*4*6 + 11] = -sin(angle);

    //sommets_support[i*4*8 + 14] = 1.0f;
    //sommets_support[i*4*8 + 15] = 0.0f;

    // troisième point
    sommets_support[i*4*6 + 12] = -SUPPORT_RADIUS * cos(angle);
    sommets_support[i*4*6 + 13] = 0.0f;
    sommets_support[i*4*6 + 14] = -SUPPORT_RADIUS * sin(angle);

    sommets_support[i*4*6 + 15] = -cos(angle);
    sommets_support[i*4*6 + 16] = 0.0f;
    sommets_support[i*4*6 + 17] = -sin(angle);

    //sommets_support[i*4*8 + 22] = 1.0f;
    //sommets_support[i*4*8 + 23] = 1.0f;

    // quatrième point
    sommets_support[i*4*6 + 18] = SUPPORT_RADIUS * cos(angle);
    sommets_support[i*4*6 + 19] = 0.0f;
    sommets_support[i*4*6 + 20] = SUPPORT_RADIUS * sin(angle);

    sommets_support[i*4*6 + 21] = cos(angle);
    sommets_support[i*4*6 + 22] = 0.0f;
    sommets_support[i*4*6 + 23] = sin(angle);

    //sommets_support[i*4*8 + 30] = 0.0f;
    //sommets_support[i*4*8 + 31] = 1.0f;

    // Remplissage du tableau des indices pour ce plan
    indices_support[i*6] = i*4;
    indices_support[i*6 + 1] = i*4 + 3;
    indices_support[i*6 + 2] = i*4 + 2;

    indices_support[i*6 + 3] = i*4;
    indices_support[i*6 + 4] = i*4 + 2;
    indices_support[i*6 + 5] = i*4 + 1;
  }
}

//----------------------------------------
void initTexture(const char* filepath, GLuint* handler, bool hasTransparency)
//-----------------------------------------
{
 int iwidth,iheight,nrChannels;
  //GLubyte *  image = NULL;
  unsigned char* image = NULL;
  if (hasTransparency)
    image = stbi_load(filepath, &iwidth, &iheight, &nrChannels, 4);
  else
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
  if (hasTransparency)
	  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iwidth,iheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
  else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, iwidth,iheight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

  // Désactivation de la texture une fois le paramétrage effectué
  glBindTexture(GL_TEXTURE_2D, 0); 

  stbi_image_free(image);
}

void initFoliageUVCoordinates(void){
  // Comme nous utilisons une image stockant un atlas de textures de végétation, nous devons
  // générer les coordonnées uv des instances de végétation en fonction de l'atlas.
  // Ce dernier est composé de 8 textures représentées sur 2 lignes et 4 colonnes (nous supposons qu'elles ont toutes la même dimension)
  for(int i = 0; i < ATLAS_ROW; i++)
  {
    for(int j = 0; j < ATLAS_COLUMN; j++)
    {
      for(int k = 0; k < FOLIAGE_PLANE_NUMBER; k++)
      {
        foliage_uv_coordinates[i*8*ATLAS_COLUMN*FOLIAGE_PLANE_NUMBER + k*8 + j*8*FOLIAGE_PLANE_NUMBER] = (GLfloat)(j*ATLAS_WIDTH) / (GLfloat)(ATLAS_COLUMN*ATLAS_WIDTH); // Coordonnée point haut gauche
        foliage_uv_coordinates[i*8*ATLAS_COLUMN*FOLIAGE_PLANE_NUMBER + k*8 + 1 + j*8*FOLIAGE_PLANE_NUMBER] =  (GLfloat)(i*ATLAS_HEIGHT) / (GLfloat)(ATLAS_ROW*ATLAS_HEIGHT);// Coordonnée point haut gauche
        foliage_uv_coordinates[i*8*ATLAS_COLUMN*FOLIAGE_PLANE_NUMBER + k*8 + 2 + j*8*FOLIAGE_PLANE_NUMBER] = (GLfloat)((j+1)*ATLAS_WIDTH) / (GLfloat)(ATLAS_COLUMN*ATLAS_WIDTH); // Coordonnée point haut droite
        foliage_uv_coordinates[i*8*ATLAS_COLUMN*FOLIAGE_PLANE_NUMBER + k*8 + 3 + j*8*FOLIAGE_PLANE_NUMBER] = (GLfloat)(i*ATLAS_HEIGHT) / (GLfloat)(ATLAS_ROW*ATLAS_HEIGHT); // Coordonnée point haut droite
        foliage_uv_coordinates[i*8*ATLAS_COLUMN*FOLIAGE_PLANE_NUMBER + k*8 + 4 + j*8*FOLIAGE_PLANE_NUMBER] = (GLfloat)((j+1)*ATLAS_WIDTH) / (GLfloat)(ATLAS_COLUMN*ATLAS_WIDTH); // Coordonnée point bas droite
        foliage_uv_coordinates[i*8*ATLAS_COLUMN*FOLIAGE_PLANE_NUMBER + k*8 + 5 + j*8*FOLIAGE_PLANE_NUMBER] = (GLfloat)((i+1)*ATLAS_HEIGHT) / (GLfloat)(ATLAS_ROW*ATLAS_HEIGHT); // Coordonnée point bas droite
        foliage_uv_coordinates[i*8*ATLAS_COLUMN*FOLIAGE_PLANE_NUMBER + k*8 + 6 + j*8*FOLIAGE_PLANE_NUMBER] = (GLfloat)(j*ATLAS_WIDTH) / (GLfloat)(ATLAS_COLUMN*ATLAS_WIDTH); // Coordonnée point bas gauche
        foliage_uv_coordinates[i*8*ATLAS_COLUMN*FOLIAGE_PLANE_NUMBER + k*8 + 7 + j*8*FOLIAGE_PLANE_NUMBER] = (GLfloat)((i+1)*ATLAS_HEIGHT) / (GLfloat)(ATLAS_ROW*ATLAS_HEIGHT); // Coordonnée point bas gauche    
      }
    }
  }
}

void initFoliageTranslations(void){
  // Définition des limites de positions pour la génération des instances
  float minPositionX = -TERRAIN_WIDTH / 2 + SUPPORT_MAX_RADIUS;
  float maxPositionX = TERRAIN_WIDTH / 2 - SUPPORT_MAX_RADIUS;
  float minPositionZ = -TERRAIN_HEIGHT / 2 + SUPPORT_MAX_RADIUS;
  float maxPositionZ = TERRAIN_HEIGHT / 2 - SUPPORT_MAX_RADIUS; 
  // Génération aléatoire des positions
  // Création de la méthode de génération des positions en X de la végétation (suivant une distribution normale)
  std::uniform_real_distribution<float> positionX(minPositionX, maxPositionX);
  // De même pour la position en Z
  std::uniform_real_distribution<float> positionZ(minPositionZ, maxPositionZ);

  // Remplissage du tableau des positions des instances
  for(int i = 0; i < FOLIAGE_INSTANCES; i++)
  {
    foliage_transformations[i] = glm::translate(glm::mat4(1.0f), glm::vec3(positionX(engine), 0.0f, positionZ(engine)));
  }
}

void initFoliageScales(void){
  // Création de la méthode de génération des échelles de la végétation (suivant une distribution normale)
  std::uniform_real_distribution<float> scale(SUPPORT_MIN_RADIUS, SUPPORT_MAX_RADIUS);
  for(int i = 0; i < FOLIAGE_INSTANCES; i++){
    float random_scale = scale(engine);
    foliage_transformations[i] = glm::scale(foliage_transformations[i], glm::vec3(random_scale, random_scale, random_scale));
  }
}

void initFoliageRotations(void){
  // Création de la méthode de génération de valeur d'angle aléatoire comprise entre 0 et Pi/3 
  std::uniform_real_distribution<float> rotation(0.0f, PI/3);
  for(int i = 0; i < FOLIAGE_INSTANCES; i++)
  {
    //tirage d'une valeur d'angle aléatoire comprise entre 0 et PI/3
    float angle = rotation(engine);
    //Création de la matrice de rotation adéquate
    foliage_transformations[i] = glm::rotate(foliage_transformations[i], angle, glm::vec3(0.0,1.0,0.0));
  }
}

// Récupération des emplacements des variables uniformes pour l'affichage du terrain
void getUniformLocationTerrain(TerrainIDs& terrain){
  //Chargement des vertex et fragment shaders pour l'affichage du tarrin
  terrain.programID = LoadShaders("TerrainShader.vert", "TerrainShader.frag");
 
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  terrain.MatrixIDView = glGetUniformLocation(terrain.programID, "VIEW");
  terrain.MatrixIDModel = glGetUniformLocation(terrain.programID, "MODEL");
  terrain.MatrixIDPerspective = glGetUniformLocation(terrain.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables uniformes du shader de Phong
  terrain.locCameraPosition = glGetUniformLocation(terrain.programID, "cameraPosition");
  terrain.locAmbientCoefficient = glGetUniformLocation(terrain.programID, "Ka");
  terrain.locDiffuseCoefficient = glGetUniformLocation(terrain.programID, "Kd");
  terrain.locSpecularCoefficient = glGetUniformLocation(terrain.programID, "Ks");
  terrain.locMaterialAmbient = glGetUniformLocation(terrain.programID, "material.ambient");
  terrain.locLightPosition = glGetUniformLocation(terrain.programID, "lightPosition");
  terrain.locMaterialDiffuse = glGetUniformLocation(terrain.programID, "material.diffuse");
  terrain.locMaterialShininess = glGetUniformLocation(terrain.programID, "material.shininess");
  terrain.locMaterialSpecular = glGetUniformLocation(terrain.programID, "material.specular");
  terrain.locTerrainTexture = glGetUniformLocation(terrain.programID, "terrainTexture");
}

// Récupération des emplacements des variables uniformes pour l'affichage de la végétation
void getUniformLocationFoliage(FoliageIDs& foliage){
  //Chargement des vertex et fragment shaders pour l'affichage de la végétation
  foliage.programID = LoadShaders("FoliageShader.vert", "TerrainShader.frag");
 
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  foliage.MatrixIDView = glGetUniformLocation(foliage.programID, "VIEW");
  foliage.MatrixIDModel = glGetUniformLocation(foliage.programID, "MODEL");
  foliage.MatrixIDPerspective = glGetUniformLocation(foliage.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables uniformes du shader de Phong
  foliage.locCameraPosition = glGetUniformLocation(foliage.programID, "cameraPosition");
  foliage.locAmbientCoefficient = glGetUniformLocation(foliage.programID, "Ka");
  foliage.locDiffuseCoefficient = glGetUniformLocation(foliage.programID, "Kd");
  foliage.locSpecularCoefficient = glGetUniformLocation(foliage.programID, "Ks");
  foliage.locMaterialAmbient = glGetUniformLocation(foliage.programID, "material.ambient");
  foliage.locLightPosition = glGetUniformLocation(foliage.programID, "lightPosition");
  foliage.locMaterialDiffuse = glGetUniformLocation(foliage.programID, "material.diffuse");
  foliage.locMaterialShininess = glGetUniformLocation(foliage.programID, "material.shininess");
  foliage.locMaterialSpecular = glGetUniformLocation(foliage.programID, "material.specular");
  foliage.locFoliageTexture = glGetUniformLocation(foliage.programID, "terrainTexture");
  foliage.locFoliageTransformation = glGetUniformLocation(foliage.programID, "transformations");
}

//----------------------------------------
void initOpenGL(void)
//----------------------------------------
{
  glCullFace (GL_BACK); // on spécifie queil faut éliminer les face arriere
  glDisable(GL_CULL_FACE); // on active l'élimination des faces qui par défaut n'est pas active
  glEnable(GL_DEPTH_TEST);

  // Récupération des emplacements des variables uniformes pour le shader de visualisation du terrain
  getUniformLocationTerrain(terrainIds);
  // Récupération des emplacements des variables uniformes pour le shader de visualisation de la végétation
  getUniformLocationFoliage(foliageIds);

  // Projection matrix : 65 Field of View, 1:1 ratio, display range : 1 unit <-> 1000 units
  // ATTENTIOn l'angle est donné en radians si f GLM_FORCE_RADIANS est défini sinon en degré
  Projection = glm::perspective( glm::radians(60.f), 1.0f, 1.0f, 1000.0f);

  shaderType = 0;
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
  glutCreateWindow("TP INSTANCES");


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

  // Remplissage des tableaux des données du terrain
  createTerrain();
  // Remplissage des tableaux de données du support pour la végétation
  createFoliageSupport();
  // Génération de la position aléatoire de la végétation
  initFoliageTranslations(); 
  // Et la rotation de ces derniers
  initFoliageRotations();
  // Tout comme la mise à l'échelle des plans
  initFoliageScales();

  //Initialisation des textures utilisées dans le programme
  initTexture(terrainTexFilepath, &terrainTexture, false);
  initTexture(foliageTexFilepath, &foliageTexture, true);

  // Initialisation des coordonnées UV de la végétation en fonction de l'atlas de texture
  initFoliageUVCoordinates();

  // construction des VBO à partir des tableaux du plan
  genereVBOTerrain();
  // construction des VBO à partir des tableaux de données du support de végétation
  genereVBOFoliageSupport();
  
  /* enregistrement des fonctions de rappel */
  glutDisplayFunc(affichage);
  glutKeyboardFunc(clavier);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotion);

  /* Entree dans la boucle principale glut */
  glutMainLoop();

  // Suppression des shader programs
  glDeleteProgram(terrainIds.programID);
  deleteVBOTerrain();
  deleteVBOFoliageSupport();
  return 0;
}

void genereVBOTerrain(){
  if(glIsBuffer(VBO_sommets_terrain) == GL_TRUE) glDeleteBuffers(1, &VBO_sommets_terrain);
  glGenBuffers(1, &VBO_sommets_terrain);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_terrain);
  glBufferData(GL_ARRAY_BUFFER, sizeof(sommets_terrain),sommets_terrain , GL_STATIC_DRAW);

  if(glIsBuffer(VBO_indices_terrain) == GL_TRUE) glDeleteBuffers(1, &VBO_indices_terrain);
  glGenBuffers(1, &VBO_indices_terrain); // ATTENTIOn IBO doit etre un GL_ELEMENT_ARRAY_BUFFER
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices_terrain);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices_terrain),indices_terrain , GL_STATIC_DRAW);

  glGenVertexArrays(1, &VAO_terrain);
  glBindVertexArray(VAO_terrain); // ici on bind le VAO , c'est lui qui recupèrera les configurations des VBO glVertexAttribPointer , glEnableVertexAttribArray...
  glEnableVertexAttribArray(indexVertex);
  glEnableVertexAttribArray(indexNormale);
  glEnableVertexAttribArray(indexUVTexture);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_terrain);
  glVertexAttribPointer (indexVertex, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
  glVertexAttribPointer (indexNormale, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
  glVertexAttribPointer (indexUVTexture, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),  (void*)(6 * sizeof(GLfloat)));

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices_terrain);
 
   // une fois la config terminée   
   // on désactive le dernier VBO et le VAO pour qu'ils ne soit pas accidentellement modifié 
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void genereVBOFoliageSupport(){
  if(glIsBuffer(VBO_sommets_vegetation) == GL_TRUE) glDeleteBuffers(1, &VBO_sommets_vegetation);
  glGenBuffers(1, &VBO_sommets_vegetation);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_vegetation);
  glBufferData(GL_ARRAY_BUFFER, sizeof(sommets_support),sommets_support , GL_STATIC_DRAW);

  if(glIsBuffer(VBO_indices_vegetation) == GL_TRUE) glDeleteBuffers(1, &VBO_indices_vegetation);
  glGenBuffers(1, &VBO_indices_vegetation); // ATTENTIOn IBO doit etre un GL_ELEMENT_ARRAY_BUFFER
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices_vegetation);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices_support),indices_support , GL_STATIC_DRAW);

  if(glIsBuffer(VBO_uv_coordinates) == GL_TRUE) glDeleteBuffers(1, &VBO_uv_coordinates);
  glGenBuffers(1, &VBO_uv_coordinates);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_uv_coordinates);
  glBufferData(GL_ARRAY_BUFFER, sizeof(foliage_uv_coordinates),foliage_uv_coordinates , GL_STATIC_DRAW);

  glGenVertexArrays(1, &VAO_vegetation);
  glBindVertexArray(VAO_vegetation); // ici on bind le VAO , c'est lui qui recupèrera les configurations des VBO glVertexAttribPointer , glEnableVertexAttribArray...
  glEnableVertexAttribArray(indexVertex); // location = 0
  glEnableVertexAttribArray(indexNormale); // location = 2
  glEnableVertexAttribArray(indexUVTexture); // location = 3


  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_vegetation);
  glVertexAttribPointer (indexVertex, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
  glVertexAttribPointer (indexNormale, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

  glBindBuffer(GL_ARRAY_BUFFER, VBO_uv_coordinates);
  glVertexAttribPointer(indexUVTexture, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
  // Demande de changer la valeur de translation toutes les nouvelles instances
  glVertexAttribDivisor(indexUVTexture, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices_vegetation);
 
   // une fois la config terminée   
   // on désactive le dernier VBO et le VAO pour qu'ils ne soit pas accidentellement modifié 
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}


void deleteVBOTerrain()
{
  glDeleteBuffers(1, &VBO_sommets_terrain);
  glDeleteBuffers(1, &VBO_indices_terrain);
  glDeleteBuffers(1, &VAO_terrain);
}

void deleteVBOFoliageSupport()
{
  glDeleteBuffers(1, &VBO_sommets_vegetation);
  glDeleteBuffers(1, &VBO_indices_vegetation);
  glDeleteBuffers(1, &VBO_uv_coordinates);
  glDeleteBuffers(1, &VAO_vegetation);
}

/* fonction d'affichage */
void affichage()
{

  /* effacement de l'image avec la couleur de fond */
 /* Initialisation d'OpenGL */
  glClearColor(0.7,0.7,0.7,1.0);
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

// Affectation de valeurs pour les variables uniformes du shader gérant l'affichage du terrain
void setTerrainUniformValues(TerrainIDs& terrain){
  //on envoie les données necessaires aux shaders
  glUniformMatrix4fv(terrain.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(terrain.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(terrain.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);

  glUniform3f(terrain.locCameraPosition,cameraPosition.x, cameraPosition.y, cameraPosition.z);
  glUniform1f(terrain.locAmbientCoefficient, Ka);
  glUniform1f(terrain.locDiffuseCoefficient, Kd);
  glUniform3f(terrain.locMaterialAmbient, materialAmbientColor.r, materialAmbientColor.g, materialAmbientColor.b);
  glUniform3f(terrain.locLightPosition,lightPosition.x,lightPosition.y,lightPosition.z);
  glUniform3f(terrain.locMaterialDiffuse, materialDiffuseColor.r, materialDiffuseColor.g, materialDiffuseColor.b);
  glUniform1f(terrain.locMaterialShininess, materialShininess);
  glUniform3f(terrain.locMaterialSpecular, materialSpecularColor.r,materialSpecularColor.g,materialSpecularColor.b);
  glUniform1f(terrain.locSpecularCoefficient, Ks);
}

// Affectation de valeurs pour les variables uniformes du shader gérant l'affichage du terrain
void setFoliageUniformValues(FoliageIDs& foliage){
  //on envoie les données necessaires aux shaders
  glUniformMatrix4fv(foliage.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(foliage.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(foliage.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);

  glUniform3f(foliage.locCameraPosition,cameraPosition.x, cameraPosition.y, cameraPosition.z);
  glUniform1f(foliage.locAmbientCoefficient, Ka);
  glUniform1f(foliage.locDiffuseCoefficient, Kd);
  glUniform3f(foliage.locMaterialAmbient, materialAmbientColor.r, materialAmbientColor.g, materialAmbientColor.b);
  glUniform3f(foliage.locLightPosition,lightPosition.x,lightPosition.y,lightPosition.z);
  glUniform3f(foliage.locMaterialDiffuse, materialDiffuseColor.r, materialDiffuseColor.g, materialDiffuseColor.b);
  glUniform1f(foliage.locMaterialShininess, materialShininess);
  glUniform3f(foliage.locMaterialSpecular, materialSpecularColor.r,materialSpecularColor.g,materialSpecularColor.b);
  glUniform1f(foliage.locSpecularCoefficient, Ks);
  glUniformMatrix4fv(foliage.locFoliageTransformation, FOLIAGE_INSTANCES, GL_FALSE, glm::value_ptr(foliage_transformations[0]));
}

void traceTerrain()
{
	glBindVertexArray(VAO_terrain); // on active le VAO
  glDrawElements(GL_TRIANGLES,  sizeof(indices_terrain) / sizeof(GLuint), GL_UNSIGNED_INT, 0);// on appelle la fonction dessin 
	glBindVertexArray(0);    // on desactive les VAO
}

void traceFoliageSupport()
{
  glBindVertexArray(VAO_vegetation); // on active le VAO
  glDrawElementsInstanced(GL_TRIANGLES, sizeof(indices_support) / sizeof(GLuint), GL_UNSIGNED_INT, 0, FOLIAGE_INSTANCES);// on appelle la fonction dessin 
  //glDrawArraysInstanced(GL_TRIANGLES, 0, sizeof(sommets_support) / (6 * sizeof(GLfloat)), FOLIAGE_INSTANCES);
  glBindVertexArray(0);    // on desactive les VAO
}

//-------------------------------------
//Trace le tore 2 via le VAO (pour le shader d'environment map et le mix entre shader de Toon, environment map et texture classique)
void traceObjet()
//-------------------------------------
{
  // Affichage du terrain
  // utilisation du shader program adéquat
  glUseProgram(terrainIds.programID);
  // Affectation des valeurs aux variables uniformes dédiées dans le shader
  setTerrainUniformValues(terrainIds);
  // Activation d'une unité de texture pour la texture du terrain
  glActiveTexture(GL_TEXTURE0);
  // Utilisation de la texture de terrain
  glBindTexture(GL_TEXTURE_2D, terrainTexture);
  // affectation de la texture au shader
  glUniform1i(terrainIds.locTerrainTexture, 0);
  // Affichage du terrain
  traceTerrain();

  // Affchage de la végétation
  // Utilisation du shader program adéquat
  glUseProgram(foliageIds.programID);
  // Affectation des valeurs aux variables uniformes dédiées dans le shader
  setFoliageUniformValues(foliageIds);
  // Activation d'une autre unité de texture pour la texture de la végétation
  glActiveTexture(GL_TEXTURE1);
  // Utilisation de la texture de végétation
  glBindTexture(GL_TEXTURE_2D, foliageTexture);
  // Affectation de la texture à la variable uniforme correspondante
  glUniform1i(foliageIds.locFoliageTexture, 1);
  // Affichage de la végétation
  traceFoliageSupport();
  // Désactivation du shader program 
  glUseProgram(0);         // et le pg

  // Désactivation des textures
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
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

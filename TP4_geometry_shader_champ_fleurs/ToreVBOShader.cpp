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

#define PLAN_R 50 // Nombre de subdivisions en x du terrain
#define PLAN_r 40 // Nombre de subdivisions en z du terrain
#define TERRAIN_WIDTH 16.0 // Longueur du terrain
#define TERRAIN_HEIGHT 16.0 // Largeur du terrain
#define ATLAS_WIDTH 512 // Largeur de l'atlas de texture (en pixels)
#define ATLAS_HEIGHT 256 // hauteur de l'atlas de texture (en pixels)
#define ATLAS_ROW 2 // nombre de lignes de l'atlas
#define ATLAS_COLUMN 4 // nombre de colonnes de l'atlas

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define PI 3.14159265358979323846
#define FOLIAGE_PLANE_NUMBER 3 // Nombre de plans utilisés pour afficher une plante
#define SUPPORT_RADIUS 1.0
#define SUPPORT_MIN_RADIUS 0.2 // Rayon minimal de chaque plan de la végétation
#define SUPPORT_MAX_RADIUS 0.7 // Rayon minimal de chaque plan de la végétation

// Création des mêmes tableaux cette fois pour stocker les informations d'un plan
// Pour les sommets,nous avons 4 points composés chacun de 3 coordonnées
// Pour les coordonnées de textures, nous avons 2 coordonnées de textures par sommet donc 8 coordonnées au total
// Pour les normales, nous avons 3 coordonnées de normales par sommets, soit 12 coordonnées au total
GLfloat sommets_terrain[(PLAN_R+1)*(PLAN_r+1)*8]; 
// Pour les indices des faces, nous avons donc deux faces triangulaires pour former un plan, soit 6 indices
GLuint indices_terrain[(PLAN_R)*(PLAN_r)*6];

// Définition des tableaux de données pour les supports floraux
// Création d'un tableau de transformations aléatoires pour les instances de végétations
glm::mat4 foliage_transformations[PLAN_R * PLAN_r];
// Création d'un tableau permettant de stocker un indice aléatoire de texture de l'atlas de végétation pour chaque instance de plants de la scène
GLfloat foliage_texture_index[PLAN_R * PLAN_r];

// initialisations
void genereVBOTerrain();
void genereSSBOFoliageTransformations();
void deleteVBOTerrain();
void deleteSSBOFoliageSupport();
void traceObjet();
void deleteTextures();

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
// SSBO pour stocker les matrices de transformations des instances de la végétation
GLuint SSBO_foliage_transformations;
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
  GLuint locTexturesNumber; // Nombre de textures dans l'atlas
};

TerrainIDs terrainIds;
FoliageIDs foliageIds;

// location des VBO
//------------------
GLuint indexVertex=0, indexUVTexture=2, indexNormale=3;


//variable pour paramétrage eclairage
//--------------------------------------
vec3 cameraPosition(0.,3.,7.); // Position de la caméra
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

//----------------------------------------
void initTextureRGBA(const char* filepath, GLuint* handler)
//-----------------------------------------
{
 int iwidth,iheight,nrChannels;
  //GLubyte *  image = NULL;
  unsigned char* image = NULL;
  image = stbi_load(filepath, &iwidth, &iheight, &nrChannels, 4);
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iwidth,iheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

  // Désactivation de la texture une fois le paramétrage effectué
  glBindTexture(GL_TEXTURE_2D, 0); 

  stbi_image_free(image);
}

void initAtlasTextureRGB(const char* filepath, GLuint* handler)
{
  // Création d'un tableau permettant de contenir séparément l'ensemble des données de chaque texture de l'atlas
  unsigned char atlas_data_rgb[ATLAS_ROW * ATLAS_COLUMN][(ATLAS_WIDTH / ATLAS_COLUMN) * (ATLAS_HEIGHT / ATLAS_ROW)* 3];
  int iwidth,iheight,nrChannels;
  //GLubyte *  image = NULL;
  unsigned char* image = NULL;
  image = stbi_load(filepath, &iwidth, &iheight, &nrChannels, 0);
  //Initialisation de la texture
	glGenTextures(1, handler);
  // Utilisation de la texture 3D
  // En effet, comme nous extrayons des textures d'un atlas, nous allons créer une texture 3D
  // où chacun de ses feuillets correspondra à une texture 2D de l'atlas
	glBindTexture(GL_TEXTURE_2D_ARRAY, *handler);
  // Affectation de ses paramètres
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Récupération des dimensions d'une texture de l'atlas (nous supposons que les textures ont la même dimension)
  GLsizei texture_width = ATLAS_WIDTH / ATLAS_COLUMN;
  GLsizei texture_height = ATLAS_HEIGHT / ATLAS_ROW;

  // Initialisation des données contenues dans la texture 3D
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, texture_width, texture_height, ATLAS_ROW*ATLAS_COLUMN, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  // Remplissage de la texture 3D à partir du contenu de la variable "image" chargée
  for(int i = 0; i < ATLAS_ROW; i++)
  {
    for(int k = 0; k < texture_height; k++)
    {
      for(int j = 0; j < ATLAS_COLUMN; j++)
      {
        // Récupération de la position de la texture courante dans l'atlas
        GLsizei offsetRow = i*texture_height;
        GLsizei offsetColumn = j*texture_width;

        // Extraction des données de la texture de l'atlas
        unsigned char* current_texture = NULL;
        // Récupération de la position du début de la k-ème ligne de la (i*ATLAS_COLUMN + j)-ème texture
        current_texture = image + (offsetRow*ATLAS_WIDTH + offsetColumn + k*ATLAS_WIDTH) * 3;
        // Ajout de la ligne dans le tableau de la (i*ATLAS_COLUMN + j)-ème texture
        memcpy(&atlas_data_rgb[i*ATLAS_COLUMN + j][k*texture_height*3], current_texture, texture_width * 3 * sizeof(unsigned char));
      }
    }
  }

  // Remplissage des feuillets de la texture 3D
  for(int i = 0; i < ATLAS_ROW * ATLAS_COLUMN; i++){
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, texture_width, texture_height, 1, GL_RGB, GL_UNSIGNED_BYTE, atlas_data_rgb[i]);
  }

  // Désactivation de la texture une fois le paramétrage effectué
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0); 

  stbi_image_free(image);
}

void initAtlasTextureRGBA(const char* filepath, GLuint* handler)
{
  // Création d'un tableau permettant de contenir séparément l'ensemble des données de chaque texture de l'atlas
  unsigned char atlas_data_rgba[ATLAS_ROW * ATLAS_COLUMN][(ATLAS_WIDTH / ATLAS_COLUMN) * (ATLAS_HEIGHT / ATLAS_ROW)* 4];
  int iwidth,iheight,nrChannels;
  //GLubyte *  image = NULL;
  unsigned char* image = NULL;
  image = stbi_load(filepath, &iwidth, &iheight, &nrChannels, 4);
  //Initialisation de la texture
	glGenTextures(1, handler);
  // Utilisation de la texture 3D
  // En effet, comme nous extrayons des textures d'un atlas, nous allons créer une texture 3D
  // où chacun de ses feuillets correspondra à une texture 2D de l'atlas
	glBindTexture(GL_TEXTURE_2D_ARRAY, *handler);
  // Affectation de ses paramètres
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Récupération des dimensions d'une texture de l'atlas (nous supposons que les textures ont la même dimension)
  GLsizei texture_width = ATLAS_WIDTH / ATLAS_COLUMN;
  GLsizei texture_height = ATLAS_HEIGHT / ATLAS_ROW;

  // Initialisation des données contenues dans la texture 3D
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, texture_width, texture_height, ATLAS_ROW*ATLAS_COLUMN, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  // Remplissage de la texture 3D à partir du contenu de la variable "image" chargée
  for(int i = 0; i < ATLAS_ROW; i++)
  {
    for(int k = 0; k < texture_height; k++)
    {
      for(int j = 0; j < ATLAS_COLUMN; j++)
      {
        // Récupération de la position de la texture courante dans l'atlas
        GLsizei offsetRow = i*texture_height;
        GLsizei offsetColumn = j*texture_width;

        // Extraction des données de la texture de l'atlas
        unsigned char* current_texture = NULL;
        //std::cout << "texture " << i*ATLAS_COLUMN + j << ", ligne " << k << std::endl;
        // Récupération de la position du début de la k-ème ligne de la (i*COLUMN_ROW + j)-ème texture
        current_texture = image + (offsetRow*ATLAS_WIDTH + offsetColumn + k*ATLAS_WIDTH) * 4;
        // Ajout de la ligne dans le tableau de la (i*COLUMN_ROW + j)-ème texture
        memcpy(&atlas_data_rgba[i*ATLAS_COLUMN + j][k*texture_height*4], current_texture, texture_width * 4 * sizeof(unsigned char));
      }
    }
  }

  // Remplissage des feuillets de la texture 3D
  for(int i = 0; i < ATLAS_ROW * ATLAS_COLUMN; i++){
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, texture_width, texture_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data_rgba[i]);
  }

  // Désactivation de la texture une fois le paramétrage effectué
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0); 

  stbi_image_free(image);
}

void initFoliageTranslations(void){
  // Définition des limites de translations pour la génération des instances
  float minPositionX = -TERRAIN_WIDTH / 2 + SUPPORT_MAX_RADIUS;
  float maxPositionX = TERRAIN_WIDTH / 2 - SUPPORT_MAX_RADIUS;
  float minPositionZ = -TERRAIN_HEIGHT / 2 + SUPPORT_MAX_RADIUS;
  float maxPositionZ = TERRAIN_HEIGHT / 2 - SUPPORT_MAX_RADIUS; 
  // Génération aléatoire des translations
  // Création de la méthode de génération des translations en X de la végétation (suivant une distribution normale)
  std::uniform_real_distribution<float> positionX(minPositionX, maxPositionX);
  // De même pour la position en Z
  std::uniform_real_distribution<float> positionZ(minPositionZ, maxPositionZ);

  // Remplissage du tableau des translations des instances
  for(int i = 0; i < PLAN_R * PLAN_r; i++)
  {
    foliage_transformations[i] = glm::translate(glm::mat4(1.0f), glm::vec3(positionX(engine), 0.0f, positionZ(engine)));
  }
}

void initFoliageScales(void){
  // Création de la méthode de génération des échelles de la végétation (suivant une distribution normale)
  std::uniform_real_distribution<float> scale(SUPPORT_MIN_RADIUS, SUPPORT_MAX_RADIUS);
  for(int i = 0; i < PLAN_R * PLAN_r; i++){
    float random_scale = scale(engine);
    foliage_transformations[i] = glm::scale(foliage_transformations[i], glm::vec3(random_scale, random_scale, random_scale));
  }
}

void initFoliageRotations(void){
  // Création de la méthode de génération de valeur d'angle aléatoire comprise entre 0 et Pi/3 
  std::uniform_real_distribution<float> rotation(0.0f, PI/FOLIAGE_PLANE_NUMBER);
  for(int i = 0; i < PLAN_R * PLAN_r; i++)
  {
    //tirage d'une valeur d'angle aléatoire comprise entre 0 et PI/3
    float angle = rotation(engine);
    //Création de la matrice de rotation adéquate
    foliage_transformations[i] = glm::rotate(foliage_transformations[i], angle, glm::vec3(0.0,1.0,0.0));
  }
}

void initFoliageTextureIndex(void){
  // Création de la méthode de génération d'index de la texture 2D dans la texture 3D de l'atlas 
  std::uniform_int_distribution<int> index(0, ATLAS_ROW*ATLAS_COLUMN - 1);
  for(int i = 0; i < PLAN_R * PLAN_r; i++)
  {
    foliage_texture_index[i] = index(engine);
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
  foliage.programID = LoadShadersWithGeom("FoliageShader.vert", "FoliageShader.geom", "FoliageShader.frag");
 
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
  foliage.locFoliageTexture = glGetUniformLocation(foliage.programID, "foliageTexture");
  foliage.locTexturesNumber = glGetUniformLocation(foliage.programID, "texturesNumber");
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
  glutCreateWindow("TP GEOMETRY SHADER - CHAMP FLEURS");


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
  // Génération de la position aléatoire de la végétation
  initFoliageTranslations(); 
  // Et la rotation de ces derniers
  initFoliageRotations();
  // Tout comme la mise à l'échelle des plans
  initFoliageScales();

  //Initialisation des textures utilisées dans le programme
  initTextureRGB(terrainTexFilepath, &terrainTexture);
  initAtlasTextureRGBA(foliageTexFilepath, &foliageTexture);

  // Initialisation des indices de textures pour la végétation
  initFoliageTextureIndex();

  // construction des VBO à partir des tableaux du plan
  genereVBOTerrain();
  // Construction du SSBO permettant d'envoyer les matrice de transformations à chaque instance de la végétation
  genereSSBOFoliageTransformations();
  
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
  glDeleteProgram(foliageIds.programID);
  // Ainsi que des VBO et SSBO utilisés dans le programme
  deleteVBOTerrain();
  deleteSSBOFoliageSupport();
  deleteTextures();
  return 0;
}

void genereVBOTerrain(void){
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

void genereSSBOFoliageTransformations(void){
  // Afin de pouvoir envoyer des matrices de transformations différentes à chaque instance de la végétation,
  // nous allons utiliser un SSBO (Shader Storage Buffer Object)
  // Initialisation du SSBO
  glGenBuffers(1, &SSBO_foliage_transformations);
  // Utilisation du SSBO
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_foliage_transformations);
  // Affectation des données des matrices de transformations au SSBO
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(foliage_transformations), foliage_transformations, GL_DYNAMIC_DRAW);
  // Désactivation du SSBO une fois la paramétrisation terminée
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 
}


void deleteVBOTerrain()
{
  glDeleteBuffers(1, &VBO_sommets_terrain);
  glDeleteBuffers(1, &VBO_indices_terrain);
  glDeleteBuffers(1, &VAO_terrain);
}

void deleteSSBOFoliageSupport(void)
{
  glDeleteBuffers(1, &SSBO_foliage_transformations);
}

void deleteTextures()
{
  glDeleteTextures(1, &terrainTexture);
  glDeleteTextures(1, &foliageTexture);
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
  glUniform1i(foliage.locTexturesNumber, ATLAS_ROW * ATLAS_COLUMN);
}

void traceTerrain()
{
	glBindVertexArray(VAO_terrain); // on active le VAO
  glDrawElements(GL_TRIANGLES,  sizeof(indices_terrain) / sizeof(GLuint), GL_UNSIGNED_INT, 0);// on appelle la fonction dessin 
	glBindVertexArray(0);    // on desactive les VAO
}

void traceFoliageSupport()
{
  glBindVertexArray(VAO_terrain); // on active le VAO du terrain
  // On utilise le SSBO des transformations
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_foliage_transformations);
  // On effectue le lien entre le SSBO et le point de binding 0 dans le shader program
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO_foliage_transformations);
  // Pour dessiner les plants de végétation, nous allons dessiner le terrain sous la forme de points
  // Ces points seront remplacés dans le geometry shader par nos plants
  // Par conséquent, le nombre de plants affichés sera égal au nombre de sommets du terrain
  glDrawElements(GL_POINTS, sizeof(indices_terrain) / sizeof(GLuint), GL_UNSIGNED_INT, 0);// on appelle la fonction dessin 
  glBindVertexArray(0);    // on desactive les VAO
  // Ainsi que le SSBO tout en désactivant le lien dans le shader
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
  glBindTexture(GL_TEXTURE_2D_ARRAY, foliageTexture);
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
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
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

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

#define PLAN_R 5
#define PLAN_r 5

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define PI 3.14159265358979323846
#define PLANE_SUPPORT 3 // Nombre de plans utilisés pour afficher une plante

// Création des mêmes tableaux cette fois pour stocker les informations d'un plan
// Pour les sommets,nous avons 4 points composés chacun de 3 coordonnées
// Pour les coordonnées de textures, nous avons 2 coordonnées de textures par sommet donc 8 coordonnées au total
// Pour les normales, nous avons 3 coordonnées de normales par sommets, soit 12 coordonnées au total
GLfloat sommets_terrain[(PLAN_R+1)*(PLAN_r+1)*8]; 
// Pour les indices des faces, nous avons donc deux faces triangulaires pour former un plan, soit 6 indices
GLuint indices_terrain[(PLAN_R)*(PLAN_r)*6];

// Définition des tableaux de données pour les supports floraux
// Chaque support est consitué de 3 plans, soient 12 points et chaque point est constitué de 3 coordonnées de position,
// 3 coordonnées de normales et 2 coordonnées de texture, soit un total de 12 * (3 + 3 + 2) = 96 valeurs
GLfloat sommets_support[96];
// Pour le tableau d'indices, nous avons 3 plans pour représenter un support. Chaque plan peut se diviser en 2 triangles
// et nous avons besoin de 3 indices pour tracer un triangle. Par conséquent, le nombre total de données est de 3 plans * 2 triangles * 3 indices = 18 valeurs
GLuint indices_support[18];

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
// VAO et VBO pour la création du sopport de la végétation
GLuint VBO_sommets_vegetation, VBO_indices_vegetation, VAO_vegetation;
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
};

TerrainIDs terrainIds;
FoliageIDs foliageIds;

// location des VBO
//------------------
GLuint indexVertex=0, indexUVTexture=2, indexNormale=3;


//variable pour paramétrage eclairage
//--------------------------------------
vec3 cameraPosition(0.,1.,3.); // Position de la caméra
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

// pour les textures
//-------------------
// Gestionnaires de texture
GLuint terrainTexture; // Gestionnaire de la texture du terrain
GLuint grassTexture; // Gestionnaire de la texture d'herbe
// Liens relatifs vers les textures
const char* terrainTexFilepath = "./texture/terrain.jpg";
const char* grassTexFilepath = "./texture/grass.png";

void createTerrain()
{
  // Définition de la longueur et de la hauteur du plan
  const GLfloat width = 8.0f;
  const GLfloat height = 8.0f;
  // Remplissage des coordonnées des sommets du plan
  for(int i = 0; i < PLAN_R; i++)
  {
    for (int j = 0; j < PLAN_r; j++)
    {
      int index = (i * PLAN_r + j) * 8;
      // Coordonnées des sommets
      sommets_terrain[index] = (j * width / (PLAN_r - 1)) - (width / 2);
      sommets_terrain[index+1] = 0.0f;
      sommets_terrain[index+2] = (i * height / (PLAN_R - 1)) - (height / 2);
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
  // Définition du rayon de chaque plan
  float radius = 0.4f;
  for(int i = 0; i < PLANE_SUPPORT; i++){
    // Calcul de l'angle du plan dans l'espace
    GLfloat angle = i * PI / 3.0f;
    // Génération des points
    // Coordonnées de positions du premier point
    sommets_support[i*4*8] = radius * cos(angle);
    sommets_support[i*4*8 + 1] = 2*radius;
    sommets_support[i*4*8 + 2] = radius * sin(angle);

    // Coordonnées de normales du premier point
    sommets_support[i*4*8 + 3] = cos(angle);
    sommets_support[i*4*8 + 4] = 0.0f;
    sommets_support[i*4*8 + 5] = sin(angle);

    // Coordonnées de texture du premier point
    sommets_support[i*4*8 + 6] = 0.0f;
    sommets_support[i*4*8 + 7] = 0.0f;

    // Nous faisons de même pour les 3 autres points du plan
    // deuxième point
    sommets_support[i*4*8 + 8] = - radius * cos(angle);
    sommets_support[i*4*8 + 9] = 2*radius;
    sommets_support[i*4*8 + 10] = - radius * sin(angle);

    sommets_support[i*4*8 + 11] = -cos(angle);
    sommets_support[i*4*8 + 12] = 0.0f;
    sommets_support[i*4*8 + 13] = -sin(angle);

    sommets_support[i*4*8 + 14] = 1.0f;
    sommets_support[i*4*8 + 15] = 0.0f;

    // troisième point
    sommets_support[i*4*8 + 16] = -radius * cos(angle);
    sommets_support[i*4*8 + 17] = 0.0f;
    sommets_support[i*4*8 + 18] = -radius * sin(angle);

    sommets_support[i*4*8 + 19] = -cos(angle);
    sommets_support[i*4*8 + 20] = 0.0f;
    sommets_support[i*4*8 + 21] = -sin(angle);

    sommets_support[i*4*8 + 22] = 1.0f;
    sommets_support[i*4*8 + 23] = 1.0f;

    // quatrième point
    sommets_support[i*4*8 +24] = radius * cos(angle);
    sommets_support[i*4*8 + 25] = 0.0f;
    sommets_support[i*4*8 + 26] = radius * sin(angle);

    sommets_support[i*4*8 + 27] = cos(angle);
    sommets_support[i*4*8 + 28] = 0.0f;
    sommets_support[i*4*8 + 29] = sin(angle);

    sommets_support[i*4*8 + 30] = 0.0f;
    sommets_support[i*4*8 + 31] = 1.0f;

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
  foliage.programID = LoadShaders("TerrainShader.vert", "TerrainShader.frag");
 
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

  //Initialisation des textures utilisées dans le programme
  initTexture(terrainTexFilepath, &terrainTexture, false);
  initTexture(grassTexFilepath, &grassTexture, true);

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

  glGenVertexArrays(1, &VAO_vegetation);
  glBindVertexArray(VAO_vegetation); // ici on bind le VAO , c'est lui qui recupèrera les configurations des VBO glVertexAttribPointer , glEnableVertexAttribArray...
  glEnableVertexAttribArray(indexVertex);
  glEnableVertexAttribArray(indexNormale);
  glEnableVertexAttribArray(indexUVTexture);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_vegetation);
  glVertexAttribPointer (indexVertex, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
  glVertexAttribPointer (indexNormale, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
  glVertexAttribPointer (indexUVTexture, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),  (void*)(6 * sizeof(GLfloat)));

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
  glDrawElements(GL_TRIANGLES,  sizeof(indices_support), GL_UNSIGNED_INT, 0);// on appelle la fonction dessin 
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
  glBindTexture(GL_TEXTURE_2D, grassTexture);
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

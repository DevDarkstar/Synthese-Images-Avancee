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

#define NB_R 40
#define NB_r 20
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define SILHOUETTE_BUFFER_SIZE 128

GLfloat sommets[(NB_R+1)*(NB_r+1)*3] ; // x 3 coordonnées (+1 acr on double les dernierspoints pour avoir des coord de textures <> pour les points de jonctions)
GLuint indices[NB_R*NB_r*6]; // x6 car pour chaque face quadrangulaire on a 6 indices (2 triangles=2x 3 indices)
GLfloat coordTexture[(NB_R+1)*(NB_r+1)*2] ; // x 2 car U+V par sommets
GLfloat normales[(NB_R+1)*(NB_r+1)*3];

// initialisations
void genereVBO();
void deleteVBO();
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

// Gestionnaires des VAO et VBO du programme
GLuint VBO_sommets,VBO_normales, VBO_indices,VBO_UVtext,VAO;
//--------------------------
// Identifiants pour les variables uniformes dans les shaders
// Shader de Phong
struct PhongIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locObjectColor; // Couleur de l'objet
  GLuint locCameraPosition; // Position de la caméra
  GLuint locMaterialShininess; // Brillance de l'objet
  GLuint locMaterialSpecular; // Couleur de la spéculaire
  GLuint locLightPosition ; // Position de la lumière
  GLuint locMaterialAmbient; // Couleur de la lumière ambiante
  GLuint locMaterialDiffuse; // Couleur de la lumière diffuse
  GLuint locAmbientCoefficient; // Coefficient de la lumière ambiante Ka
  GLuint locDiffuseCoefficient; // Coefficient de la lumière diffuse Kd
  GLuint locSpecularCoefficient; // Coefficient de la lumière spéculaire Ks
};

struct ToonIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locObjectColor; // Couleur de l'objet
  GLuint locCameraPosition; // Position de la caméra
  GLuint locLightPosition; // Position de la lumière
  GLuint locMaterialDiffuse; // Couleur de la lumière diffuse
  GLuint locDiffuseCoefficient; // Coefficient de la lumière diffuse Kd
  GLuint locDisplaySilhouette; // Contrôle l'affichage de la silhouette autour du tore
  GLuint locSilhouetteTex; // Texture de la silhouette autour de l'objet
};

struct GoochIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locCameraPosition; // Position de la caméra
  GLuint locLightPosition; // Position de la lumière
  GLuint locCoolColor; // Intensité de la couleur froide du shader de Gooch (bleu)
  GLuint locWarmColor; // Intensité de la couleur chaude su shader de Gooch (jaune)
  GLuint locDisplaySilhouette; // Contrôle l'affichage de la silhouette autour du tore
  GLuint locEps; // Contrôle l'intensité de l'effet de bord (silhouette)
  GLuint locSilhouetteColor; // Couleur de l'effet de bord du tore (silhouette)
};

PhongIDs phongIds;
ToonIDs toonIds;
GoochIDs goochIds;

// location des VBO
//------------------
GLuint indexVertex=0, indexUVTexture=2, indexNormale=3;

//Gestionnaires de la texture de la silhouette pour le shader Toon
GLuint silhouetteTex;

//variable pour paramétrage eclairage
//--------------------------------------
vec3 cameraPosition(0.,0.,3.); // Position de la caméra
// le matériau
//---------------
vec3 objectColor(1.0,0.5,0.0); // Couleur de l'objet (orange)
GLfloat materialShininess=16.; // Brillance de l'objet
vec3 materialSpecularColor(0.47,0.71,1.);  // couleur de la spéculaire (bleu ciel)
vec3 materialAmbientColor(1.,1.,1.); // couleur de la lumière ambiante (blanc)
vec3 materialDiffuseColor(0.,1.,1.); // couleur de la lumière diffuse (cyan)

// la lumière
//-----------
vec3 lightPosition(1.,0.,.5); // Position de la lumière dans la scène
GLfloat Ka = .8; // Coefficient de la lumière ambiante
GLfloat Kd = .9; // Coefficient de la lumière diffuse
GLfloat Ks = .7; // Coefficient de la lumière spéculaire

// silhouette
GLfloat eps = 0.3; // Variable epsilon indiquant l'intensité de la silhouette autour de l'objet (effet de bord)
vec3 silhouetteColor(0.,0.,0.); // Couleur de la silhouette
GLint displaySilhouette = 0; // Booléen indiquant si la silhouette est visible ou non
vec3 silhouetteData[SILHOUETTE_BUFFER_SIZE]; // Données de la texture permettant de représenter la silhouette dans le shader Toon

glm::mat4 MVP;      // justement la voilà
glm::mat4 Model, View, Projection;    // Matrices constituant MVP

// shader de Gooch
vec3 coolColor(0.0,0.0,1.0); // Couleur froide (bleu)
vec3 warmColor(1.0,1.0,0.0); // Couleur chaude (jaune)

GLuint shaderType; // type de shader actif (permet de contrôler le shader à afficher à l'écran)
// 0 = Phong, 1 = Toon, 2 = Gooch

// Dimensions de la fenêtre d'affichage
int screenHeight = 500;
int screenWidth = 500;

//-------------------------
void createTorus(float R, float r )
{
	float theta, phi;
	theta = ((float)radians(360.f))/((float)NB_R);
	phi = ((float)(radians(360.f)))/((float)NB_r);

	float pasU, pasV;
  pasU= 1./NB_R;
  pasV= 1./NB_r;
  for (int i =0;i<=NB_R;i++ )
    for (int j =0;j<=NB_r;j++ )
    {
      float a,b,c;
      sommets[(i*(NB_r+1)*3)+ (j*3)] =   (R+r*cos((float)j*phi)) * cos((float)i*theta)    ;//x
      sommets[(i*(NB_r+1)*3)+ (j*3)+1] =  (R+r*cos((float)j*phi)) * sin((float)i*theta)  ;//y
      sommets[(i*(NB_r+1)*3)+ (j*3)+2] =  r*sin((float)j*phi)  ;
      
      normales[(i*(NB_r+1)*3)+ (j*3)] =   cos((float)j*phi)*cos((float)i*theta)    ;//x
      normales[(i*(NB_r+1)*3)+ (j*3)+1] = cos((float)j*phi)* sin((float)i*theta)  ;//y
      normales[(i*(NB_r+1)*3)+ (j*3)+2] =  sin((float)j*phi)  ;
      
      coordTexture[(i*(NB_r+1)*2)+ (j*2)]= ((float)i)*pasV;
      coordTexture[(i*(NB_r+1)*2)+ (j*2)+1]= ((float)j)*pasV;
    }

int indiceMaxI =((NB_R+1)*(NB_r))-1;
int indiceMaxJ= (NB_r+1);

for (int i =0;i<NB_R;i++ )
for (int j =0;j<NB_r;j++ )
{ 	
int i0,i1,i2,i3,i4,i5;
 	indices[(i*NB_r*6)+ (j*6)]= (unsigned int)((i*(NB_r+1))+ j); 
   indices[(i*NB_r*6)+ (j*6)+1]=(unsigned int)((i+1)*(NB_r+1)+ (j));
   indices[(i*NB_r*6)+ (j*6)+2]=(unsigned int)(((i+1)*(NB_r+1))+ (j+1));
   indices[(i*NB_r*6)+ (j*6)+3]=(unsigned int)((i*(NB_r+1))+ j);
   indices[(i*NB_r*6)+ (j*6)+4]=(unsigned int)(((i+1)*(NB_r+1))+ (j+1));
   indices[(i*NB_r*6)+ (j*6)+5]=(unsigned int)(((i)*(NB_r+1))+ (j+1));
}
}

void initSilhouetteTexture(){
  // Création des données à stocker dans la texture de seuillage
  for(int i = 0; i < SILHOUETTE_BUFFER_SIZE; i++){
    // Calcul de la valeur à l'indice i du tableau comprise entre 0 et 1
    GLfloat value = (GLfloat)i / (GLfloat)SILHOUETTE_BUFFER_SIZE;
    // Puis stockage de la couleur adéquate en fonction d'une valeur epsilon
    if (value <= eps)
      silhouetteData[i] = silhouetteColor;
    else
      silhouetteData[i] = vec3(1.,1.,1.);
  }
  // Création d'une texture 1D qui contiendra les valeurs de texture de la silhouette
  glGenTextures(1, &silhouetteTex);
  // Utilisation de la texture 1D
  glBindTexture(GL_TEXTURE_1D, silhouetteTex);

  // Initialisation de la texture 1D
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F, SILHOUETTE_BUFFER_SIZE, 0, GL_RGB, GL_FLOAT, silhouetteData);

  //Paramètres de la texture
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  //Désactivation de la texture 1D après paramétrage
  glBindTexture(GL_TEXTURE_1D, 0);
}

// Récupération des emplacements des variables uniformes pour le shader de Phong
void getUniformLocationPhong(PhongIDs& phong){
  //Chargement des vertex et fragment shaders pour Phong
  phong.programID = LoadShaders("PhongShader.vert", "PhongShader.frag");
 
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  phong.MatrixIDView = glGetUniformLocation(phong.programID, "VIEW");
  phong.MatrixIDModel = glGetUniformLocation(phong.programID, "MODEL");
  phong.MatrixIDPerspective = glGetUniformLocation(phong.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables unfiformes du shader de Phong
  phong.locObjectColor = glGetUniformLocation(phong.programID, "objectColor");
  phong.locCameraPosition = glGetUniformLocation(phong.programID, "cameraPosition");
  phong.locAmbientCoefficient = glGetUniformLocation(phong.programID, "Ka");
  phong.locDiffuseCoefficient = glGetUniformLocation(phong.programID, "Kd");
  phong.locSpecularCoefficient = glGetUniformLocation(phong.programID, "Ks");
  phong.locMaterialAmbient = glGetUniformLocation(phong.programID, "material.ambient");
  phong.locLightPosition = glGetUniformLocation(phong.programID, "lightPosition");
  phong.locMaterialDiffuse = glGetUniformLocation(phong.programID, "material.diffuse");
  phong.locMaterialShininess = glGetUniformLocation(phong.programID, "material.shininess");
  phong.locMaterialSpecular = glGetUniformLocation(phong.programID, "material.specular");
}

// Récupération des emplacements des variables uniformes pour le shader Toon
void getUniformLocationToon(ToonIDs& toon){
  //Chargement des vertex et fragment shaders pour Toon
  toon.programID = LoadShaders("ToonShader.vert", "ToonShader.frag");
 
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  toon.MatrixIDView = glGetUniformLocation(toon.programID, "VIEW");
  toon.MatrixIDModel = glGetUniformLocation(toon.programID, "MODEL");
  toon.MatrixIDPerspective = glGetUniformLocation(toon.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables unfiformes du shader de Toon
  toon.locObjectColor = glGetUniformLocation(toon.programID, "objectColor");
  toon.locCameraPosition = glGetUniformLocation(toon.programID, "cameraPosition");
  toon.locDiffuseCoefficient = glGetUniformLocation(toon.programID, "Kd");
  toon.locLightPosition = glGetUniformLocation(toon.programID, "lightPosition");
  toon.locMaterialDiffuse = glGetUniformLocation(toon.programID, "materialDiffuse");
  toon.locDisplaySilhouette = glGetUniformLocation(toon.programID, "displaySilhouette");
  toon.locSilhouetteTex = glGetUniformLocation(toon.programID, "silhouetteTex");
}

// Récupération des emplacements des variables uniformes pour le shader de Gooch
void getUniformLocationGooch(GoochIDs& gooch){
  //Chargement des vertex et fragment shaders pour Gooch
  gooch.programID = LoadShaders("GoochShader.vert", "GoochShader.frag");
 
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  gooch.MatrixIDView = glGetUniformLocation(gooch.programID, "VIEW");
  gooch.MatrixIDModel = glGetUniformLocation(gooch.programID, "MODEL");
  gooch.MatrixIDPerspective = glGetUniformLocation(gooch.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables unfiformes du shader de Gooch
  gooch.locCoolColor = glGetUniformLocation(gooch.programID, "coolColor");
  gooch.locWarmColor = glGetUniformLocation(gooch.programID, "warmColor");
  gooch.locCameraPosition = glGetUniformLocation(gooch.programID, "cameraPosition");
  gooch.locLightPosition = glGetUniformLocation(gooch.programID, "lightPosition");
  gooch.locEps = glGetUniformLocation(gooch.programID, "epsilon");
  gooch.locDisplaySilhouette = glGetUniformLocation(gooch.programID, "displaySilhouette");
  gooch.locSilhouetteColor = glGetUniformLocation(gooch.programID, "silhouetteColor");
}

//----------------------------------------
void initOpenGL(void)
//----------------------------------------
{
  glCullFace (GL_BACK); // on spécifie queil faut éliminer les face arriere
  glEnable(GL_CULL_FACE); // on active l'élimination des faces qui par défaut n'est pas active
  glEnable(GL_DEPTH_TEST);

  //création et remplissage de la texture de seuillage de la silhouette pour le shader Toon
  initSilhouetteTexture();
  // Récupération des emplacements des variables uniformes pour le shader de Phong
  getUniformLocationPhong(phongIds);
  // Récupération des emplacements des variables uniformes pour le shader Toon
  getUniformLocationToon(toonIds);
  // Récupération des emplacements des variables uniformes pour le shader de Gooch
  getUniformLocationGooch(goochIds);

  // Projection matrix : 65 Field of View, 1:1 ratio, display range : 1 unit <-> 1000 units
  // ATTENTIOn l'angle est donné en radians si f GLM_FORCE_RADIANS est défini sinon en degré
  Projection = glm::perspective( glm::radians(60.f), 1.0f, 1.0f, 1000.0f);
  // Initialisation du shader à afficher (par défaut le shader de Phong)
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
  glutCreateWindow("CUBE VBO SHADER ");


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

  // Création d'un tore
  createTorus(1.,.3);

  // construction des VBO à partir des tableaux des informations du tore
  genereVBO();
  
  /* enregistrement des fonctions de rappel */
  glutDisplayFunc(affichage);
  glutKeyboardFunc(clavier);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotion);

  /* Entree dans la boucle principale glut */
  glutMainLoop();

  // Suppression des "shader programs"
  glDeleteProgram(phongIds.programID);
  glDeleteProgram(toonIds.programID);
  glDeleteProgram(goochIds.programID);
  deleteVBO();
  return 0;
}

void genereVBO ()
{
  if(glIsBuffer(VBO_sommets) == GL_TRUE) glDeleteBuffers(1, &VBO_sommets);
  glGenBuffers(1, &VBO_sommets);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);
  glBufferData(GL_ARRAY_BUFFER, sizeof(sommets),sommets , GL_STATIC_DRAW);

  if(glIsBuffer(VBO_normales) == GL_TRUE) glDeleteBuffers(1, &VBO_normales);
  glGenBuffers(1, &VBO_normales);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_normales);
  glBufferData(GL_ARRAY_BUFFER, sizeof(normales),normales , GL_STATIC_DRAW);

  if(glIsBuffer(VBO_indices) == GL_TRUE) glDeleteBuffers(1, &VBO_indices);
  glGenBuffers(1, &VBO_indices); // ATTENTIOn IBO doit etre un GL_ELEMENT_ARRAY_BUFFER
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),indices , GL_STATIC_DRAW);

  if(glIsBuffer(VBO_UVtext) == GL_TRUE) glDeleteBuffers(1, &VBO_UVtext);
  glGenBuffers(1, &VBO_UVtext);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_UVtext);
  glBufferData(GL_ARRAY_BUFFER, sizeof(coordTexture),coordTexture , GL_STATIC_DRAW);

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO); // ici on bind le VAO , c'est lui qui recupèrera les configurations des VBO glVertexAttribPointer , glEnableVertexAttribArray...
  glEnableVertexAttribArray(indexVertex);
  glEnableVertexAttribArray(indexNormale );
  glEnableVertexAttribArray(indexUVTexture);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);
  glVertexAttribPointer ( indexVertex, 3, GL_FLOAT, GL_FALSE, 0, (void*)0 );

  glBindBuffer(GL_ARRAY_BUFFER, VBO_normales);
  glVertexAttribPointer ( indexNormale, 3, GL_FLOAT, GL_FALSE, 0, (void*)0  );

  glBindBuffer(GL_ARRAY_BUFFER, VBO_UVtext);
  glVertexAttribPointer (indexUVTexture, 2, GL_FLOAT, GL_FALSE, 0,  (void*)0  );

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices);

  // une fois la config terminée   
  // on désactive le dernier VBO et le VAO pour qu'ils ne soit pas accidentellement modifié 
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
 
}
//-----------------
void deleteVBO ()
//-----------------
{
  glDeleteBuffers(1, &VBO_sommets);
  glDeleteBuffers(1, &VBO_normales);
  glDeleteBuffers(1, &VBO_indices);
  glDeleteBuffers(1, &VBO_UVtext);
  glDeleteBuffers(1, &VAO);
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
 
     View       = glm::lookAt(   cameraPosition, // Camera is at (0,0,3), in World Space
                                            glm::vec3(0,0,0), // and looks at the origin
                                            glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                                             );
     Model = glm::mat4(1.0f);
     Model = glm::translate(Model,glm::vec3(0,0,cameraDistance));
     Model = glm::rotate(Model,glm::radians(cameraAngleX),glm::vec3(1, 0, 0) );
     Model = glm::rotate(Model,glm::radians(cameraAngleY),glm::vec3(0, 1, 0) );
     Model = glm::scale(Model,glm::vec3(.8, .8, .8));
     MVP = Projection * View * Model;
     traceObjet(); // affiche le tore à l'écran

 /* on force l'affichage du resultat */
   glutPostRedisplay();
   glutSwapBuffers();
}

// Affectation de valeurs pour les variables uniformes du shader de Phong
void setPhongUniformValues(PhongIDs& phong){
  //on envoie les données necessaires aux shaders */
  glUniformMatrix4fv(phong.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(phong.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(phong.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);

  glUniform3f(phong.locObjectColor, objectColor.r, objectColor.g, objectColor.b);
  glUniform3f(phong.locCameraPosition,cameraPosition.x, cameraPosition.y, cameraPosition.z);
  glUniform1f(phong.locAmbientCoefficient, Ka);
  glUniform1f(phong.locDiffuseCoefficient, Kd);
  glUniform3f(phong.locMaterialAmbient, materialAmbientColor.r, materialAmbientColor.g, materialAmbientColor.b);
  glUniform3f(phong.locLightPosition,lightPosition.x,lightPosition.y,lightPosition.z);
  glUniform3f(phong.locMaterialDiffuse, materialDiffuseColor.r, materialDiffuseColor.g, materialDiffuseColor.b);
  glUniform1f(phong.locMaterialShininess, materialShininess);
  glUniform3f(phong.locMaterialSpecular, materialSpecularColor.r,materialSpecularColor.g,materialSpecularColor.b);
  glUniform1f(phong.locSpecularCoefficient, Ks);
}

// Affectation de valeurs pour les variables uniformes du shader Toon
void setToonUniformValues(ToonIDs& toon){
  //on envoie les données necessaires aux shaders */
  glUniformMatrix4fv(toon.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(toon.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(toon.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);

  glUniform3f(toon.locObjectColor, 0.94f, 0.97f, 1.0f);
  glUniform3f(toon.locCameraPosition,cameraPosition.x, cameraPosition.y, cameraPosition.z);
  glUniform1f(toon.locDiffuseCoefficient, Kd);
  glUniform3f(toon.locLightPosition,lightPosition.x,lightPosition.y,lightPosition.z);
  glUniform3f(toon.locMaterialDiffuse, materialDiffuseColor.r, materialDiffuseColor.g, materialDiffuseColor.b);
  glUniform1ui(toon.locDisplaySilhouette, displaySilhouette);
  glUniform1i(toon.locSilhouetteTex, 0);
}

// Affectation de valeurs pour les variables uniformes du shader de Gooch
void setGoochUniformValues(GoochIDs& gooch){
  //on envoie les données necessaires aux shaders */
  glUniformMatrix4fv(gooch.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(gooch.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(gooch.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);

  glUniform3f(gooch.locCoolColor, coolColor.r, coolColor.g, coolColor.b);
  glUniform3f(gooch.locWarmColor, warmColor.r, warmColor.g, warmColor.b);
  glUniform3f(gooch.locCameraPosition,cameraPosition.x, cameraPosition.y, cameraPosition.z);
  glUniform3f(gooch.locLightPosition,lightPosition.x,lightPosition.y,lightPosition.z);
  glUniform1ui(gooch.locDisplaySilhouette, displaySilhouette);
  glUniform1f(gooch.locEps,eps);
  glUniform3f(gooch.locSilhouetteColor, silhouetteColor.r, silhouetteColor.g, silhouetteColor.b);
}

//-------------------------------------
//Trace le tore 2 via le VAO
void traceObjet()
//-------------------------------------
{
 if (shaderType == 0){
  // Activation du shader program contrôlant l'affichage du shader de Phong
  glUseProgram(phongIds.programID);
  // Affectation de valeurs pour les variables uniformes du shader de Phong
  setPhongUniformValues(phongIds);
 }
 else if (shaderType == 1){
  // Activation du shader program contrôlant l'affichage du shader Toon
  glUseProgram(toonIds.programID);
  // Utilisation de la texture de seuillage pour la silhouette
  glBindTexture(GL_TEXTURE_1D, silhouetteTex);
  // Affectation de valeurs pour les variables uniformes du shader Toon
  setToonUniformValues(toonIds);
 }
 else{
  // Activation du shader program contrôlant l'affichage du shader de Gooch
  glUseProgram(goochIds.programID);
  // Affectation de valeurs pour les variables uniformes du shader de Gooch
  setGoochUniformValues(goochIds);
 }
 
  //pour l'affichage
	glBindVertexArray(VAO); // on active le VAO
  glDrawElements(GL_TRIANGLES,  sizeof(indices), GL_UNSIGNED_INT, 0);// on appelle la fonction dessin 
	glBindVertexArray(0);    // on desactive les VAO
  glUseProgram(0);         // et le pg
  // Ainsi que d'éventuelles textures utilisées (cas du shader Toon)
  glBindTexture(GL_TEXTURE_1D, 0);

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
    case 'p' : /*Passage en mode d'affichage de Phong*/
      shaderType = 0;
      glutPostRedisplay();
      break;
    case 't' : /*Passage en mode d'affichage Toon*/
      shaderType = 1;
      glutPostRedisplay();
      break;
    case 'g': /*Passage en mode d'affichage Gooch*/
      shaderType = 2;
      glutPostRedisplay();
      break;
    case 'd' : /*Affichage ou non du mode silhouette*/
      displaySilhouette = (displaySilhouette + 1) % 2;
      glutPostRedisplay();
      break;
    case '+' : /*Augmente l'effet de bord de la silhouette*/
       eps += 0.1;
      //if ( eps > 1.)  eps = 1.;
      glutPostRedisplay();
      break;
    case '-' : /* Diminue l'effet de bord de la silhouette*/
       eps -= 0.1;
      //if ( eps < 0.)  eps = 0.;
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

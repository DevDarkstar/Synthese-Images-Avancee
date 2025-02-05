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

// variables Handle d'opengl
GLuint VBO_sommets,VBO_normales, VBO_indices,VBO_UVtext,VAO;
//--------------------------
// Identifiants pour l'affichage du tore sur lequel s'applique les reflets d'une skybox
struct SkyboxIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locLightPosition ; // Position de la lumière 
  GLuint locSkyboxTexture; // Texture de la Skybox
};

// shader mixant une environment map, une texture classique et le shader Toon
struct ToonIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locObjectColor; // Couleur de l'objet
  GLuint locCameraPosition; // Position de la caméra
  GLuint locLightPosition; // Position de la lumière
  GLuint locMaterialDiffuse; // Couleur de la lumière diffuse
  GLuint locDiffuseCoefficient; // Coefficient de la lumière diffuse Kd
  GLuint locTextureMix; // Paramètre de mixage entre un mix de texture classique et un shader Toon et celle de la skybox
  GLuint locToonMix; // Paramètre de mixage entre une texture classique et un shader Toon
  GLuint locSkyboxTexture; // Texture de la Skybox
  GLuint locTexture; // Texture classique
};

SkyboxIDs skyboxIds;
ToonIDs toonIds;

// location des VBO
//------------------
GLuint indexVertex=0, indexUVTexture=2, indexNormale=3;
GLuint skyboxTexture; // Gestionnaire de la texture skybox
GLuint texture; // Gestionaire de la texture classique

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

glm::mat4 MVP;      // justement la voilà
glm::mat4 Model, View, Projection;    // Matrices constituant MVP

GLfloat textureMix = 0.5f; // Paramètre de mixage entre une texture classique et une texture type skybox
GLfloat toonMix = 0.5f;
GLuint shaderType; // type de shader actif (permet de contrôler le shader à afficher à l'écran)
// 0 = Phong, 1 = Toon, 2 = Gooch

// Dimensions de la fenêtre d'affichage
int screenHeight = 500;
int screenWidth = 500;

// pour la texcture
//-------------------
GLuint image ;
GLuint bufTexture,bufNormalMap;
GLuint locationTexture,locationNormalMap;
const char* texture_link = "./texture/Metalcolor.ppm";
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

//----------------------------------------
GLubyte* glmReadPPM(const char* filename, int* width, int* height)
//----------------------------------------
{
    FILE* fp;
    int i, w, h, d;
    unsigned char* image;
    char head[70];          /* max line <= 70 in PPM (per spec). */
    
    fp = fopen(filename, "rb");
    if (!fp) {
        perror(filename);
        return NULL;
    }
    
    /* grab first two chars of the file and make sure that it has the
       correct magic cookie for a raw PPM file. */
    fgets(head, 70, fp);
    if (strncmp(head, "P6", 2)) {
        fprintf(stderr, "%s: Not a raw PPM file\n", filename);
        return NULL;
    }
    
    /* grab the three elements in the header (width, height, maxval). */
    i = 0;
    while(i < 3) {
        fgets(head, 70, fp);
        if (head[0] == '#')     /* skip comments. */
            continue;
        if (i == 0)
            i += sscanf(head, "%d %d %d", &w, &h, &d);
        else if (i == 1)
            i += sscanf(head, "%d %d", &h, &d);
        else if (i == 2)
            i += sscanf(head, "%d", &d);
    }
    
    /* grab all the image data in one fell swoop. */
    image = new unsigned char[w*h*3];
    fread(image, sizeof(unsigned char), w*h*3, fp);
    fclose(fp);
    
    *width = w;
    *height = h;
    return image;
}

//----------------------------------------
void initTexture(void)
//-----------------------------------------
{
 int iwidth  , iheight;
  GLubyte *  image = NULL;
 
  image = glmReadPPM(texture_link, &iwidth, &iheight);
  //Initialisation de la texture
	glGenTextures(1, &texture);
  // Utilisation de l'unité de texture 1 pour y stocker la texture classique
  glActiveTexture(GL_TEXTURE0);
  // Utilisation de la texture 2D
	glBindTexture(GL_TEXTURE_2D, texture);
  // Affectation de ses paramètres
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Remplissage de la texture à partir du contenu de la variable "image" chargée
	glTexImage2D(GL_TEXTURE_2D, 0, 3, iwidth,iheight, 0, GL_RGB,GL_UNSIGNED_BYTE, image);

  // Désactivation de la texture une fois le paramétrage effectué
  glBindTexture(GL_TEXTURE_2D, 0); 
}

void initSkyboxTexture(void){
  //Initialisation de la texture
  glGenTextures(1, &skyboxTexture);
  // Utilisation de l'unité de texture 1 pour y stocker la texture de Sky Box
  glActiveTexture(GL_TEXTURE1);
  // Utilisation de la texture de type Cube Map
  glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

  // Chargement des faces du cube et remplissage des textures des faces avec le contenu des images chargées
  int iwidth, iheight;
  GLubyte* front = glmReadPPM("./skybox/front.ppm", &iwidth, &iheight);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z ,0,3,iwidth,iheight,0,GL_RGB,GL_UNSIGNED_BYTE, front);

  GLubyte* left = glmReadPPM("./skybox/left.ppm", &iwidth, &iheight);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X ,0,3,iwidth,iheight,0,GL_RGB,GL_UNSIGNED_BYTE, left);

  GLubyte* right = glmReadPPM("./skybox/right.ppm", &iwidth, &iheight);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X ,0,3,iwidth,iheight,0,GL_RGB,GL_UNSIGNED_BYTE, right);

  GLubyte* top = glmReadPPM("./skybox/top.ppm", &iwidth, &iheight);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y ,0,3,iwidth,iheight,0,GL_RGB,GL_UNSIGNED_BYTE, top);

  GLubyte* bottom = glmReadPPM("./skybox/bottom.ppm", &iwidth, &iheight);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y ,0,3,iwidth,iheight,0,GL_RGB,GL_UNSIGNED_BYTE, bottom);

  GLubyte* back = glmReadPPM("./skybox/back.ppm", &iwidth, &iheight);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ,0,3,iwidth,iheight,0,GL_RGB,GL_UNSIGNED_BYTE, back);

  // Paramétrage de la Sky Box
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  
  // Désactivation de la texture une fois les paramétrages effectués
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void getUniformLocationSkybox(SkyboxIDs& skybox){
  // Chargement des vertex et fragment shaders pour le shader de la Sky Box
  skybox.programID = LoadShaders("Skybox.vert", "Skybox.frag");
 
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  skybox.MatrixIDView = glGetUniformLocation(skybox.programID, "VIEW");
  skybox.MatrixIDModel = glGetUniformLocation(skybox.programID, "MODEL");
  skybox.MatrixIDPerspective = glGetUniformLocation(skybox.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables unfiformes du shader de la Skybox
  skybox.locLightPosition = glGetUniformLocation(skybox.programID, "lightPosition");
  skybox.locSkyboxTexture = glGetUniformLocation(skybox.programID, "skyboxTexture");
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
  toon.locTextureMix = glGetUniformLocation(toon.programID, "textureMix");
  toon.locToonMix = glGetUniformLocation(toon.programID, "toonMix");
  toon.locTexture = glGetUniformLocation(toon.programID, "classicTexture");
  toon.locSkyboxTexture = glGetUniformLocation(toon.programID, "skyboxTexture");
}

//----------------------------------------
void initOpenGL(void)
//----------------------------------------
{
  glCullFace (GL_BACK); // on spécifie queil faut éliminer les face arriere
  glEnable(GL_CULL_FACE); // on active l'élimination des faces qui par défaut n'est pas active
  glEnable(GL_DEPTH_TEST);

  // Récupération des emplacements des variables uniformes pour le shader de la Sky Box
  getUniformLocationSkybox(skyboxIds);
  // Récupération des emplacements des variables uniformes pour le shader Toon
  getUniformLocationToon(toonIds);

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

   
  createTorus(1.,.3);

  //Initialisation des textures utilisées dans le programme
  initTexture();
  initSkyboxTexture();

  // construction des VBO a partir des tableaux du cube deja construit
  genereVBO();
  
  /* enregistrement des fonctions de rappel */
  glutDisplayFunc(affichage);
  glutKeyboardFunc(clavier);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotion);

  /* Entree dans la boucle principale glut */
  glutMainLoop();

  // Suppression des shader programs
  glDeleteProgram(skyboxIds.programID);
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
     traceObjet();        // trace VBO avec ou sans shader

 /* on force l'affichage du resultat */
   glutPostRedisplay();
   glutSwapBuffers();
}


// Affectation de valeurs pour les variables uniformes du shader de la skybox
void setSkyBoxUniformValues(SkyboxIDs& skybox){
  //on envoie les données necessaires aux shaders
  glUniformMatrix4fv(skybox.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(skybox.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(skybox.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);
  glUniform3f(skybox.locLightPosition,lightPosition.x,lightPosition.y,lightPosition.z);
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
  glUniform1f(toon.locTextureMix, textureMix);
  glUniform1f(toon.locToonMix, toonMix);
}

//-------------------------------------
//Trace le tore 2 via le VAO
void traceObjet()
//-------------------------------------
{
 // Affichage du shader représentant les reflets d'une environment map sur un tore
 if (shaderType == 0){
  // Utilisation du shader program du shader de la SkyBox
  glUseProgram(skyboxIds.programID);
  setSkyBoxUniformValues(skyboxIds);
  // Activation de l'unité contenant la texture de l'environment map
  glActiveTexture(GL_TEXTURE1);
  // Utilisation de la texture
  glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
  // Affectation de la texture à la variable uniforme adéquate
  glUniform1i(skyboxIds.locSkyboxTexture, 1);
 }
 // Sinon affichage du shader Toon mixant les textures d'environment et d'une texture classique
 else{
  // Utilisation du shader program du shader de la SkyBox
  glUseProgram(toonIds.programID);
  setToonUniformValues(toonIds);
  // Activation de l'unité contenant la texture classique
  glActiveTexture(GL_TEXTURE0);
  // Utilisation de la texture
  glBindTexture(GL_TEXTURE_2D, texture);
  // Affectation de la texture à la variable uniforme adéquate
  glUniform1i(toonIds.locTexture, 0);
  // Activation de l'unité contenant la texture de l'environment map
  glActiveTexture(GL_TEXTURE1);
  // Utilisation de la texture
  glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
  // Affectation de la texture à la variable uniforme adéquate
  glUniform1i(toonIds.locSkyboxTexture, 1);
 }
 
  //pour l'affichage
	glBindVertexArray(VAO); // on active le VAO
  glDrawElements(GL_TRIANGLES,  sizeof(indices), GL_UNSIGNED_INT, 0);// on appelle la fonction dessin 
	glBindVertexArray(0);    // on desactive les VAO
  glUseProgram(0);         // et le pg

  // Désactivation des textures
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
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
    case 'k' : /*Passage en mode d'affichage du tore avec la Sky Box*/
      shaderType = 0;
      glutPostRedisplay();
      break;
    case 't' : /*Passage en mode d'affichage du tore en mixant l'environment map, une texture classique ainsi qu'un shader Toon*/
      shaderType = 1;
      glutPostRedisplay();
      break;
    case 'p' : /*Agit sur le mix entre le shader Toon et la texture classique*/
      toonMix += 0.1;
      if (toonMix > 1.) toonMix = 1.;
      glutPostRedisplay();
      break;
    case 'm' : //*Agit sur le mix entre le shader Toon et la texture classique*/
      toonMix -= 0.1;
      if (toonMix < 0.) toonMix = 0.;
      glutPostRedisplay();
      break;    
    case '+' : /*Agit sur le mix entre la texture de la SkyBox et la texture classique*/
      textureMix += 0.1;
      if (textureMix > 1.) textureMix = 1.;
      glutPostRedisplay();
      break;
    case '-' : //*Agit sur le mix entre la texture de la SkyBox et la texture classique*/
      textureMix -= 0.1;
      if (textureMix < 0.) textureMix = 0.;
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

/* inclusion des fichiers d'en-tete Glut */
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

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

struct VertexData {
  GLfloat position[3];
  GLfloat uv[2];
  GLfloat normal[3];
};

std::vector<VertexData> vertices;
GLuint indices[NB_R*NB_r*6]; // x6 car pour chaque face quadrangulaire on a 6 indices (2 triangles=2x 3 indices)

// initialisations
void genereVAO();
void deleteVAO();
void genereSSBOToreVertex();
void deleteSSBOToreVertex();
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

// Gestionnaires des VAO et IBO du programme
GLuint VAO, IBO_indices;
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

PhongIDs phongIds;
// Gestionnaire du SSBO contenant les indices des sommets du tore
GLuint ssbo_tore_vertex;

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

// Dimensions de la fenêtre d'affichage
int screenHeight = 500;
int screenWidth = 500;

//-------------------------
void createTorus(float R, float r )
{
  // Réservation de l'espace mémoire pour le stockage des structures de données de chaque sommet du tore
  //vertices.reserve((NB_R + 1) * (NB_r + 1));
	float theta, phi;
	theta = ((float)radians(360.f))/((float)NB_R);
	phi = ((float)(radians(360.f)))/((float)NB_r);

	float pasU, pasV;
  pasU= 1./NB_R;
  pasV= 1./NB_r;
  for (int i =0;i<NB_R;i++ )
    for (int j =0;j<NB_r;j++ )
    {
      // Création d'une nouvelle structure VertexData
      VertexData data;
      // Remplissage de la structure
      data.position[0] =   (R+r*cos((float)j*phi)) * cos((float)i*theta)    ;//x
      data.position[1] =  (R+r*cos((float)j*phi)) * sin((float)i*theta)  ;//y
      data.position[2] =  r*sin((float)j*phi)  ;
      
      data.normal[0] =   cos((float)j*phi)*cos((float)i*theta)    ;//x
      data.normal[1] = cos((float)j*phi)* sin((float)i*theta)  ;//y
      data.normal[2] =  sin((float)j*phi)  ;
      
      data.uv[0]= ((float)i)*pasV;
      data.uv[1]= ((float)j)*pasV;
      // Ajout de la structure de données dans le tableau des données des sommets du tore
      vertices.push_back(data);
    }

int indiceMaxI =((NB_R+1)*(NB_r))-1;
int indiceMaxJ= (NB_r+1);

for (int i =0;i<NB_R;i++ )
for (int j =0;j<NB_r;j++ )
{ 	
int i0,i1,i2,i3,i4,i5;
 	indices[(i*NB_r*6)+ (j*6)]= (unsigned int)((i*(NB_r+1))+ j); 
   indices[(i*NB_r*6)+ (j*6)+1]=(unsigned int)(((i+1)%NB_R)*(NB_r+1)+ (j));
   indices[(i*NB_r*6)+ (j*6)+2]=(unsigned int)((((i+1)%NB_R)*(NB_r+1))+ (j+1)%NB_r);
   indices[(i*NB_r*6)+ (j*6)+3]=(unsigned int)((i*(NB_r+1))+ j);
   indices[(i*NB_r*6)+ (j*6)+4]=(unsigned int)((((i+1)%NB_R)*(NB_r+1))+ (j+1)%NB_r);
   indices[(i*NB_r*6)+ (j*6)+5]=(unsigned int)(((i)*(NB_r+1))+ (j+1)%NB_r);
}
for(int i = 0; i < NB_R * NB_r * 6; i++)
{
  std::cout << indices[i] << " ";
}
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
  phong.locObjectColor = glGetUniformLocation(phong.programID, "material.objectColor");
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

//----------------------------------------
void initOpenGL(void)
//----------------------------------------
{
  glCullFace (GL_BACK); // on spécifie queil faut éliminer les face arriere
  glEnable(GL_CULL_FACE); // on active l'élimination des faces qui par défaut n'est pas active
  glEnable(GL_DEPTH_TEST);

  // Récupération des emplacements des variables uniformes pour le shader de Phong
  getUniformLocationPhong(phongIds);

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
  // Création d'un VAO qui contiendra uniquement un IBO
  genereVAO();
  // création du SSBO qui contiendra les informations des sommets du tore
  genereSSBOToreVertex();
  /* enregistrement des fonctions de rappel */
  glutDisplayFunc(affichage);
  glutKeyboardFunc(clavier);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotion);

  /* Entree dans la boucle principale glut */
  glutMainLoop();
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
  // Suppression des "shader programs"
  glDeleteProgram(phongIds.programID);
  // Du VAO et de l'IBO
  deleteVAO();
  // ainsi que le SSBO
  deleteSSBOToreVertex();
  return 0;
}

void genereSSBOToreVertex(void){
  glGenBuffers(1, &ssbo_tore_vertex);
  // Utilisation du SSBO
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_tore_vertex);
  // Affectation des coordonnées des sommets du tore au SSBO
  glNamedBufferStorage(ssbo_tore_vertex, sizeof(VertexData) * vertices.size(), (const void*)vertices.data(), GL_DYNAMIC_STORAGE_BIT);
  // On effectue le lien entre le SSBO et le point de binding 0 dans le shader program
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_tore_vertex);
  // Désactivation du SSBO une fois la paramétrisation terminée
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 
}

void genereVAO()
{
  // Création d'un VAO
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO); // Utilisation du VAO

  // Création de l'IBO
  glGenBuffers(1, &IBO_indices);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_indices);
  // Affectation des données à l'IBO
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),indices , GL_STATIC_DRAW);

  // une fois la config terminée   
  // on désactive l'IBO et le VAO une fois leur utilisation terminée
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
//-----------------
void deleteVAO ()
//-----------------
{
  glDeleteBuffers(1, &IBO_indices);
  glDeleteBuffers(1, &VAO);
}

void deleteSSBOToreVertex(void)
{
  glDeleteBuffers(1, &ssbo_tore_vertex);
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

//-------------------------------------
//Trace le tore 2 via le VAO
void traceObjet()
//-------------------------------------
{
  // Activation du shader program contrôlant l'affichage du shader de Phong
  glUseProgram(phongIds.programID);
  // Affectation de valeurs pour les variables uniformes du shader de Phong
  setPhongUniformValues(phongIds);
 
  //pour l'affichage
	glBindVertexArray(VAO); // on utilise le VAO
  // On utilise le SSBO pour transmettre les informations des sommets aux shaders
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_tore_vertex);
  
  // On effectue le rendu du tore
  std::cout << "hello\n";
  glDrawElements(GL_TRIANGLES, sizeof(indices), GL_UNSIGNED_INT, 0);// on appelle la fonction dessin 
	std::cout << "hello2\n";
  glBindVertexArray(0);    // on desactive le VAO
  // Ainsi que le SSBO
  glUseProgram(0);         // et le shader program
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

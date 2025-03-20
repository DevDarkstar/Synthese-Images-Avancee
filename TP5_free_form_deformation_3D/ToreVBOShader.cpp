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
#include <vector>
#include <array>
#include <algorithm>

// Include GLM
#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
using namespace glm;
using namespace std;

#define PI 3.141592653589793

std::vector<std::array<GLfloat,3>> sommets_cercle; // Coordonnées des sommets du cercle
std::vector<GLuint> indices_cercle; // Indices des sommets des faces du cercle
std::vector<std::array<GLfloat, 3>> normales_cercle; // Coordonnées des normales du cercle
std::vector<std::array<GLfloat,2>> uvs_cercle; // Cordonnées UVs du cercle

std::vector<std::array<GLfloat,3>> sommets_grille; // Coordonnées des sommets de la grille de déformation
std::vector<GLuint> indices_grille; // Indices des sommets des faces de la grille de déformation

std::vector<std::vector<GLfloat>> bernstein_pols; // Conteneur des polynômes de Bernstein pour chaque sommet du cercle unitaire 2D
int control_point_x_coord = 0; // indice du point de contrôle en x dans la grille de déformation
int control_point_y_coord = 0; // indice du point de contrôle en y dans la grille de déformation

// initialisations

void genereVBOCercle();
void deleteVBOCercle();
void genereVBOGrille();
void deleteVBOGrille();
void traceObjet();

// fonctions de rappel de glut
void affichage();
void clavier(unsigned char,int,int);
void mouse(int, int, int, int);
void mouseMotion(int, int);
void reshape(int,int);
// variables globales pour OpenGL
bool mouseLeftDown;
bool mouseRightDown;
bool mouseMiddleDown;
float mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;
float cameraDistance=0.;

// variables Handle d'opengl
// VAO et VBOs pour la création du cercle unitaire 2D
GLuint VBO_sommets,VBO_normales, VBO_indices,VBO_UVtext,VAO;
// VAO et VBOs pour la création de la grille de déformation
GLuint VBO_sommets_grille, VBO_indices_grille, VAO_grille;
//--------------------------
// Identifiants pour l'affichage du tore sur lequel s'applique les reflets d'une skybox
struct CircleIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locCheckerboardTexture; // Texture du damier
};

struct GridIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locColor; // Couleur de la grille
};

CircleIDs circleIds;
GridIDs gridIds;

// location des VBO
//------------------
GLuint indexVertex=0, indexUVTexture=2, indexNormale=3;
GLuint checkerboardTexture; // Gestionaire de la texture en damier

//variable pour paramétrage eclairage
//--------------------------------------
vec3 cameraPosition(0.,0.,3.); // Position de la caméra
// le matériau
//---------------
vec3 objectColor(1.0,0.5,0.0); // Couleur de la grille de déformation

glm::mat4 MVP;      // justement la voilà
glm::mat4 Model, View, Projection;    // Matrices constituant MVP

// Dimensions de la fenêtre d'affichage
int screenHeight = 500;
int screenWidth = 500;

// pour la texture
//-------------------
GLuint image ;
const char* texture_link = "./texture/damier.ppm";
//-------------------------

void createCircle(int resolution)
{
  // Ajout du centre du cercle
  sommets_cercle.emplace_back(std::array<GLfloat,3>{0.0f,0.0f,0.0f});
  normales_cercle.emplace_back(std::array<GLfloat,3>{0.0f,0.0f,1.0f});
  uvs_cercle.emplace_back(std::array<GLfloat,2>{0.5f,0.5f});
  for(int i = 0; i < resolution; i++)
  {
    // Création d'un nouveau sommet
    std::array<GLfloat, 3> vertex;
    vertex[0] = cos(i/GLfloat(resolution)*2*PI);
    vertex[1] = sin(i/GLfloat(resolution)*2*PI);
    vertex[2] = 0.0f;
    // Ajout du sommet dans le tableau correspondant
    sommets_cercle.push_back(vertex);
    // Ajout de sa normale
    normales_cercle.emplace_back(std::array<GLfloat,3>{0.0f, 0.0f, 1.0f});
    // Création des coordonnées uvs du sommet
    std::array<GLfloat, 2> uvs;
    uvs[0] = (cos(i/GLfloat(resolution)*2*PI) + 1) / 2;
    uvs[1] = (sin(i/GLfloat(resolution)*2*PI) + 1) / 2;
    // Ajout des coordonnées uvs du sommet dans le tableau correspondant
    uvs_cercle.push_back(uvs);
  }

  // Création des indices du cercle
  for(int i = 1; i < sommets_cercle.size() - 1; i++)
  {
    std::array<GLuint,3> indices = {0,(GLuint)i,(GLuint)i+1};
    std::copy(indices.begin(), indices.end(), std::back_inserter(indices_cercle));
  }
  // Ajout de la dernière portion du cercle unitaire
  std::array<GLuint,3> indices = {0, (GLuint)(resolution), 1};
  std::copy(indices.begin(), indices.end(), std::back_inserter(indices_cercle));
}

void createDeformationGrid()
{
  for(int y = 0; y < 3; y++)
  {
    for(int x = 0; x < 3; x++)
    {
      sommets_grille.emplace_back(std::array<GLfloat,3>{-1.0f + (GLfloat)x, -1.0f + (GLfloat)y, 0.0f});
    }
  }

  indices_grille = {0,1,0,3,1,2,1,4,2,5,3,4,3,6,4,5,4,7,5,8,6,7,7,8};
}

void computeBernsteinPols()
{
  // création d'une expression lambda permettant de retourner le polynôme de Bernstein de la courbe
  // de Bézier de degré 2 à un instant t
  auto bernstein_pol = [](GLfloat t) -> std::array<GLfloat,3> {
    return std::array<GLfloat,3>{(GLfloat)(pow(1-t, 2)), 2*t*(1-t), (GLfloat)(pow(t, 2))};
  };

  for(int i = 0; i < sommets_cercle.size(); i++)
  {
    // Calcul des polynômes de Bernstein pour les axes x, y et z du point du cercle unitaire courant
    // en faisant en sorte de transposer les coordonnées des sommets du cercle unitaire dans l'intervalle [0,1]
    // (les sommets sont initialement compris entre -1 et 1)
    std::array<GLfloat,3> s = bernstein_pol((sommets_cercle[i][0] + 1) / 2);
    std::array<GLfloat,3> t = bernstein_pol((sommets_cercle[i][1] + 1) / 2);
    std::array<GLfloat,3> u = bernstein_pol((sommets_cercle[i][2] + 1) / 2);
    // Création d'un conteneur pour stocker les polynômes résultants
    std::vector<GLfloat> pols;
    std::copy(s.begin(), s.end(), std::back_inserter(pols));
    std::copy(t.begin(), t.end(), std::back_inserter(pols));
    std::copy(u.begin(), u.end(), std::back_inserter(pols));
    bernstein_pols.push_back(pols);
  }
}

void updateCircleVerticesPosition()
{
  // Mise à jour de la position des sommets du cercle unitaire après application de la déformation
  // en se servant des nouvelles positions des points de contrôles (sommets de la grille de déformation)
  // Parcours des sommets du cercle unitaire
  for(int i = 0; i < sommets_cercle.size(); i++)
  {
    // Parcours des sommets de la grille de déformation en s et en t
    // Création d'un sommet contenant la nouvelle position du sommet du cercle
    std::array<GLfloat,3> pos = {0.0f, 0.0f, 0.0f};
    for(int u = 0; u < 3; u++)
    {
      for(int t = 0; t < 3; t++)
      {
        for(int s = 0; s < 3; s++)
        {
          // Récupérations des polynômes de bernstein aux instants s, t et u
          GLfloat s_coord = bernstein_pols[i][s];
          GLfloat t_coord = bernstein_pols[i][3 + t];
          GLfloat u_coord = bernstein_pols[i][6 + u];
          // Récupération du point de contrôle courant de la grille de déformation
          std::array<GLfloat,3> control_point = sommets_grille[3*t + s];
          // Ajout du produit des polynômes de Bernstein avec les coordonnées du point de contrôle à la position résultante du sommet du cercle unitaire
          pos[0] += (s_coord * t_coord * u_coord * control_point[0]);
          pos[1] += (s_coord * t_coord * u_coord * control_point[1]); 
          pos[2] += (s_coord * t_coord * u_coord * control_point[2]);
        }
      }
    }
    // Mise à jour de la nouvelle position du sommet du cercle unitaire
    sommets_cercle[i][0] = pos[0];
    sommets_cercle[i][1] = pos[1];
    sommets_cercle[i][2] = pos[2];
  }
  // Mise à jour du VBO contenant les coordonnées des sommets du cercle unitaire
  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(std::array<GLfloat,3>) * sommets_cercle.size(), (const void*)sommets_cercle.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
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
	glGenTextures(1, &checkerboardTexture);
  // Utilisation de la texture 2D
	glBindTexture(GL_TEXTURE_2D, checkerboardTexture);
  // Affectation de ses paramètres
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Remplissage de la texture à partir du contenu de la variable "image" chargée
	glTexImage2D(GL_TEXTURE_2D, 0, 3, iwidth,iheight, 0, GL_RGB,GL_UNSIGNED_BYTE, image);
  // Libération de la mémoire allouée au stockage des informations de la texture
  delete[] image;
}

void getUniformLocationCircle(CircleIDs& circle){
  // Chargement des vertex et fragment shaders pour le shader du cercle
  circle.programID = LoadShaders("CircleShader.vert", "CircleShader.frag");
 
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  circle.MatrixIDView = glGetUniformLocation(circle.programID, "VIEW");
  circle.MatrixIDModel = glGetUniformLocation(circle.programID, "MODEL");
  circle.MatrixIDPerspective = glGetUniformLocation(circle.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables uniformes du shader du cercle
  circle.locCheckerboardTexture = glGetUniformLocation(circle.programID, "checkerboardTexture");
}

void getUniformLocationGrid(GridIDs& grid){
  // Chargement des vertex et fragment shaders pour le shader de la grille de déformation
  grid.programID = LoadShaders("GridShader.vert", "GridShader.frag");
 
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  grid.MatrixIDView = glGetUniformLocation(grid.programID, "VIEW");
  grid.MatrixIDModel = glGetUniformLocation(grid.programID, "MODEL");
  grid.MatrixIDPerspective = glGetUniformLocation(grid.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables uniformes du shader de la grille de déformation
  grid.locColor = glGetUniformLocation(grid.programID, "color");
}

//----------------------------------------
void initOpenGL(void)
//----------------------------------------
{
  glDisable(GL_CULL_FACE); // on désactive l'élimination des faces qui par défaut n'est pas active
  glEnable(GL_DEPTH_TEST);

  // Récupération des emplacements des variables uniformes pour le shader du cercle
  getUniformLocationCircle(circleIds);
  // Récupération des emplacements des variables uniformes pour le shader de la grille de déformation
  getUniformLocationGrid(gridIds);

  // Projection matrix : 65 Field of View, 1:1 ratio, display range : 1 unit <-> 1000 units
  // ATTENTIOn l'angle est donné en radians si f GLM_FORCE_RADIANS est défini sinon en degré
  Projection = glm::perspective(glm::radians(60.f), 1.0f, 1.0f, 1000.0f);
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
  glutCreateWindow("TP5 - FREE FORM DEFORMATION 3D");


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

  // Remplissage des tableaux de données du cercle unitaire
  createCircle(32);
  // Remplissage des tableaux de données de la grille de déformation
  createDeformationGrid();
  // Calcul des polynômes de Bernstein sur les sommets du cercle unitaire
  computeBernsteinPols();

  initTexture();

  // construction des VBO à partir des données du cercle unitaire
  genereVBOCercle();
  // construction des VBO à partir des données de la grille de déformation
  genereVBOGrille();

  std::cout << "Point de controle actif : (" << control_point_x_coord << "," << control_point_y_coord << ")" << std::endl;
  
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

void genereVBOCercle()
{
  if(glIsBuffer(VBO_sommets) == GL_TRUE) glDeleteBuffers(1, &VBO_sommets);
  glGenBuffers(1, &VBO_sommets);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);
  glBufferData(GL_ARRAY_BUFFER, sizeof(std::array<GLfloat,3>) * sommets_cercle.size(), (const void*)sommets_cercle.data() , GL_STATIC_DRAW);

  if(glIsBuffer(VBO_normales) == GL_TRUE) glDeleteBuffers(1, &VBO_normales);
  glGenBuffers(1, &VBO_normales);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_normales);
  glBufferData(GL_ARRAY_BUFFER, sizeof(std::array<GLfloat,3>) * normales_cercle.size(), (const void*)normales_cercle.data() , GL_STATIC_DRAW);

  if(glIsBuffer(VBO_indices) == GL_TRUE) glDeleteBuffers(1, &VBO_indices);
  glGenBuffers(1, &VBO_indices); // ATTENTIOn IBO doit etre un GL_ELEMENT_ARRAY_BUFFER
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*indices_cercle.size(), (const void*)indices_cercle.data(), GL_STATIC_DRAW);

  if(glIsBuffer(VBO_UVtext) == GL_TRUE) glDeleteBuffers(1, &VBO_UVtext);
  glGenBuffers(1, &VBO_UVtext);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_UVtext);
  glBufferData(GL_ARRAY_BUFFER, sizeof(std::array<GLfloat,2>)*uvs_cercle.size(), (const void*)uvs_cercle.data() , GL_STATIC_DRAW);

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO); // ici on bind le VAO , c'est lui qui recupèrera les configurations des VBO glVertexAttribPointer , glEnableVertexAttribArray...
  glEnableVertexAttribArray(indexVertex);
  glEnableVertexAttribArray(indexNormale );
  glEnableVertexAttribArray(indexUVTexture);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);
  glVertexAttribPointer (indexVertex, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_normales);
  glVertexAttribPointer (indexNormale, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_UVtext);
  glVertexAttribPointer (indexUVTexture, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices);

  // une fois la config terminée   
  // on désactive le dernier VBO et le VAO pour qu'ils ne soit pas accidentellement modifié 
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void genereVBOGrille()
{
  if(glIsBuffer(VBO_sommets_grille) == GL_TRUE) glDeleteBuffers(1, &VBO_sommets_grille);
  glGenBuffers(1, &VBO_sommets_grille);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_grille);
  glBufferData(GL_ARRAY_BUFFER, sizeof(std::array<GLfloat,3>) * sommets_grille.size(), (const void*)sommets_grille.data() , GL_STATIC_DRAW);

  if(glIsBuffer(VBO_indices_grille) == GL_TRUE) glDeleteBuffers(1, &VBO_indices_grille);
  glGenBuffers(1, &VBO_indices_grille); // ATTENTIOn IBO doit etre un GL_ELEMENT_ARRAY_BUFFER
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices_grille);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*indices_grille.size(), (const void*)indices_grille.data(), GL_STATIC_DRAW);

  glGenVertexArrays(1, &VAO_grille);
  glBindVertexArray(VAO_grille); // ici on bind le VAO , c'est lui qui recupèrera les configurations des VBO glVertexAttribPointer , glEnableVertexAttribArray...
  glEnableVertexAttribArray(indexVertex);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_grille);
  glVertexAttribPointer (indexVertex, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices_grille);

  // une fois la config terminée   
  // on désactive le dernier VBO et le VAO pour qu'ils ne soit pas accidentellement modifié 
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

//-----------------
void deleteVBOCercle()
//-----------------
{
  glDeleteBuffers(1, &VBO_sommets);
  glDeleteBuffers(1, &VBO_normales);
  glDeleteBuffers(1, &VBO_indices);
  glDeleteBuffers(1, &VBO_UVtext);
  glDeleteBuffers(1, &VAO);
}

//-----------------
void deleteVBOGrille()
//-----------------
{
  glDeleteBuffers(1, &VBO_sommets_grille);
  glDeleteBuffers(1, &VBO_indices_grille);
  glDeleteBuffers(1, &VAO_grille);
}

void deleteTextures(void)
{
  glDeleteTextures(1, &checkerboardTexture);
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
  glLineWidth(2.0);
 
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


// Affectation de valeurs pour les variables uniformes du shader du cercle
void setCircleUniformValues(CircleIDs& circle){
  //on envoie les données necessaires aux shaders
  glUniformMatrix4fv(circle.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(circle.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(circle.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);
}

// Affectation de valeurs pour les variables uniformes du shader de la grille de déformation
void setGridUniformValues(GridIDs& grid){
  //on envoie les données necessaires aux shaders
  glUniformMatrix4fv(grid.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(grid.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(grid.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);
  glUniform3f(grid.locColor, objectColor.r, objectColor.g, objectColor.b);
}

void traceCercle()
{
  glBindVertexArray(VAO); // on active le VAO
  glDrawElements(GL_TRIANGLES, indices_cercle.size(), GL_UNSIGNED_INT, 0);// on appelle la fonction dessin 
}

void traceGrilleDeformation()
{
  glBindVertexArray(VAO_grille); // Activation du VAO de la grille de déformation
  glDrawElements(GL_LINES, indices_grille.size(), GL_UNSIGNED_INT, 0);
}

//-------------------------------------
//Trace le tore 2 via le VAO (pour le shader d'environment map et le mix entre shader de Toon, environment map et texture classique)
void traceObjet()
//-------------------------------------
{
  // Utilisation du shader program du shader du cercle unitaire
  glUseProgram(circleIds.programID);
  setCircleUniformValues(circleIds);
  // Affectation de la texture à la variable uniforme adéquate
  glUniform1i(circleIds.locCheckerboardTexture, 0);
 
  //affichage du cercle unitaire
	traceCercle();

  // Utilisation du shader program du shader de la grille de déformation
  glUseProgram(gridIds.programID);
  setGridUniformValues(gridIds);
 
  //affichage de la grille de déformation
	traceGrilleDeformation();
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
    case 'x': /*Déplacement du point de contrôle actif en -x*/
      sommets_grille[control_point_y_coord * 3 + control_point_x_coord][0] -= 0.1f;
      // Mise à jour des coordonnées des sommets de la grille de déformation dans le VBO correspondant
      glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_grille);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(std::array<GLfloat,3>) * sommets_grille.size(), (const void*)sommets_grille.data());
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      // Mise à jour des nouvelles coordonnées des sommets du cercle unitaire après déformation
      updateCircleVerticesPosition();
      glutPostRedisplay();
      break;  
    case 'X': /*Déplacement du point de contrôle actif en +x*/
      sommets_grille[control_point_y_coord * 3 + control_point_x_coord][0] += 0.1f;
      // Mise à jour des coordonnées des sommets de la grille de déformation dans le VBO correspondant
      glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_grille);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(std::array<GLfloat,3>) * sommets_grille.size(), (const void*)sommets_grille.data());
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      // Mise à jour des nouvelles coordonnées des sommets du cercle unitaire après déformation
      updateCircleVerticesPosition();
      glutPostRedisplay();
      break;
    case 'y': /*Déplacement du point de contrôle actif en -y*/
      sommets_grille[control_point_y_coord * 3 + control_point_x_coord][1] -= 0.1f;
      // Mise à jour des coordonnées des sommets de la grille de déformation dans le VBO correspondant
      glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_grille);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(std::array<GLfloat,3>) * sommets_grille.size(), (const void*)sommets_grille.data());
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      // Mise à jour des nouvelles coordonnées des sommets du cercle unitaire après déformation
      updateCircleVerticesPosition();
      glutPostRedisplay();
      break;  
    case 'Y': /*Déplacement du point de contrôle actif en +y*/
      sommets_grille[control_point_y_coord * 3 + control_point_x_coord][1] += 0.1f;
      // Mise à jour des coordonnées des sommets de la grille de déformation dans le VBO correspondant
      glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_grille);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(std::array<GLfloat,3>) * sommets_grille.size(), (const void*)sommets_grille.data());
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      // Mise à jour des nouvelles coordonnées des sommets du cercle unitaire après déformation
      updateCircleVerticesPosition();
      glutPostRedisplay();
      break;
    case 'z': /*Déplacement du point de contrôle actif en -z*/
      sommets_grille[control_point_y_coord * 3 + control_point_x_coord][2] -= 0.1f;
      // Mise à jour des coordonnées des sommets de la grille de déformation dans le VBO correspondant
      glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_grille);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(std::array<GLfloat,3>) * sommets_grille.size(), (const void*)sommets_grille.data());
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      // Mise à jour des nouvelles coordonnées des sommets du cercle unitaire après déformation
      updateCircleVerticesPosition();
      glutPostRedisplay();
      break;  
    case 'Z': /*Déplacement du point de contrôle actif en +z*/
      sommets_grille[control_point_y_coord * 3 + control_point_x_coord][2] += 0.1f;
      // Mise à jour des coordonnées des sommets de la grille de déformation dans le VBO correspondant
      glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets_grille);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(std::array<GLfloat,3>) * sommets_grille.size(), (const void*)sommets_grille.data());
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      // Mise à jour des nouvelles coordonnées des sommets du cercle unitaire après déformation
      updateCircleVerticesPosition();
      glutPostRedisplay();
      break;
    case 's': /*Sélection d'un nouveau point de contrôle selon les x*/
      control_point_x_coord = (control_point_x_coord + 1) % 3;
      std::cout << "Point de controle actif : (" << control_point_x_coord << "," << control_point_y_coord << ")" << std::endl;
      break;
    case 't': /*Sélection d'un nouveau point de contrôle selon les y*/
      control_point_y_coord = (control_point_y_coord + 1) % 3;
      std::cout << "Point de controle actif : (" << control_point_x_coord << "," << control_point_y_coord << ")" << std::endl;
      break;
    case 'q' : /*la touche 'q' permet de quitter le programme */
      std::cout << "Désactivation du VAO actif...\n";
      glBindVertexArray(0);
      std::cout << "Désactivation du shader program actif ainsi que les textures utilisées pour les rendus...\n";
      glUseProgram(0);
      glBindTexture(GL_TEXTURE_1D, 0);
      std::cout << "Suppression des éléments du programme...\n";
      // Suppression des shader programs
      glDeleteProgram(circleIds.programID);
      glDeleteProgram(gridIds.programID);
      // Ainsi que des VAO, VBOs, framebuffers et textures utilisées dans le programme
      deleteVBOCercle();
      deleteVBOGrille();
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

/********************************************************/
/*                     CubeVBOShader.cpp                         */
/********************************************************/
/* Premiers pas avec OpenGL.                            */
/* Objectif : afficher a l'ecran uncube avec ou sans shader    */
/********************************************************/

/* inclusion des fichiers d'en-tete Glut */
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <random>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <math.h>
#include "shader.hpp"
#include <string.h>

#include <chrono>

// Include GLM
#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"

#include <vector>

#define PI 3.14159265358979323846


using namespace glm;
using namespace std;

typedef struct{
  GLuint tauxCreation = 25;
  GLfloat dureeVie = 5.0f;
} SystemeParticule;

typedef struct {
  GLfloat position[3] ;
  GLfloat vitesse[3] ;
  GLfloat masse;
  GLfloat force[3];
  GLfloat age;
} Particule ;

vector<Particule> listeParticules;
SystemeParticule systemeParticules;


void anim( int NumTimer) ;

// initialisations

void genereVBO();
void createParticules(int nbParticules ) ;
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
float cameraDistance=1.;
float t = 0.001f;
float coneRadius = 10.0f;
static float fountainHeight = 60.0f;
// variables Handle d'opengl 
//--------------------------
GLuint programID;   // handle pour le shader
GLuint MatrixIDMVP,MatrixIDView,MatrixIDModel,MatrixIDPerspective;    // handle pour la matrice MVP
GLuint VBO_sommets,VBO_normales, VBO_indices,VBO_UVtext,VAO;

// Initialisation de la génération de nombres pseudo-aléatoires
std::random_device rd;
// Création du moteur de génération
std::default_random_engine engine(rd());
std::uniform_real_distribution<float> angle(0.0f, 2.0f / 3.0f * PI); // Angle du cône à l'intérieur duquel sont projectées les particules
static std::uniform_real_distribution<float> zDir(fountainHeight, (fountainHeight + 0.2*fountainHeight)); // Poussée verticale appliquée initialement sur les particules

// location des VBO
//------------------
GLuint indexVertex=0, indexUVTexture=2, indexNormale=3, indexVitesse=4 ;

//variable pour paramétrage eclairage
//--------------------------------------
vec3 cameraPosition(2.,0.,0.);
// le matériau
//---------------
GLfloat materialShininess=3.;

// la lumière
//-----------
vec3 LightPosition(1.,0.,.5);
vec3 LightIntensities(1.,1.,1.);// couleur la lumiere
GLfloat LightAttenuation =1.;
GLfloat LightAmbientCoefficient=.1;

glm::mat4 MVP;      // justement la voilà
glm::mat4 Model, View, Projection;    // Matrices constituant MVP



int screenHeight = 1500;
int screenWidth = 1500;

// pour la texture
//-------------------
GLuint image ;
GLuint bufTexture,bufNormalMap;
GLuint locationTexture,locationNormalMap;
//-------------------------
void creationParticules(int nbParticules)
{
  for(int i = 0; i < nbParticules; i++)
  {
    // Création d'une nouvelle particule
    Particule p;
    // On lui définit des propriétés
    p.masse = 0.01f; // Une masse
    GLfloat force[3] = {0.0f, 0.0f, -9.91f}; // Une force à laquelle la particule est soumise
    memcpy(p.force, force, 3 * sizeof(GLfloat));
    GLfloat position[3] = {0.0f, 0.0f, 0.0f}; // Une position initiale
    memcpy(p.position, position, 3 * sizeof(GLfloat));
    GLfloat vitesse[3] = {coneRadius * sin(angle(engine)), coneRadius * cos(angle(engine)), zDir(engine)}; // ainsi qu'une vitesse initiale
    memcpy(p.vitesse, vitesse, 3 * sizeof(GLfloat));
    p.age = 0.0f;
    // Ajout de la nouvelle particule dans la liste des particules
    listeParticules.push_back(p);
  }
}

void miseAJourParticules(int delatTemps)
{
  // Conversion du temps en seconde
  GLfloat temps = delatTemps / 1000.0f;
  for(auto it = listeParticules.begin(); it != listeParticules.end();)
  {
    // Ajout du temps à l'âge de la particule
    (*it).age += temps;
    if ((*it).age > systemeParticules.dureeVie)
      listeParticules.erase(it);
    else
    { 
      // Calcul de l'accélération de la particule
      GLfloat acceleration[3] = {(*it).force[0]/(*it).masse, (*it).force[1]/(*it).masse, (*it).force[2]/(*it).masse};

      // Mise à jour de la vitesse
      (*it).vitesse[0] += acceleration[0]*t;
      (*it).vitesse[1] += acceleration[1]*t;
      (*it).vitesse[2] += acceleration[2]*t;

      // Mise à jour de la nouvelle position
      //GLfloat posX = (*it).position[0] + (*it).vitesse[0]*t;
      //(*it).position[0] = (posX < -0.5f) ? -0.5f : (posX > 0.5f) ? 0.5f : posX;
      (*it).position[0] += (*it).vitesse[0]*t;
      //GLfloat posY = (*it).position[1] + (*it).vitesse[1]*t;
      //(*it).position[1] = (posY < -0.5f) ? -0.5f : (posY > 0.5f) ? 0.5f : posY;
      (*it).position[1] += (*it).vitesse[1]*t;
      //GLfloat posZ = (*it).position[2] + (*it).vitesse[2]*t;
      //(*it).position[2] = (posZ > 1.0f) ? 1.0f : posZ;
      (*it).position[2] += (*it).vitesse[2]*t;
      
      if((*it).position[2] <= 0.0f)
      {
        (*it).force[0] = 0.0f;
        (*it).force[1] = 0.0f;
        (*it).force[2] = 0.0f;

        (*it).vitesse[0] = 0.0f;
        (*it).vitesse[1] = 0.0f;
        (*it).vitesse[2] = 0.0f;
      }
      ++it;
    }
  }
}


//----------------------------------------
void initOpenGL(void)
//----------------------------------------
{
  glCullFace (GL_BACK); // on spécifie queil faut éliminer les face arriere
  glEnable(GL_CULL_FACE); // on active l'élimination des faces qui par défaut n'est pas active
  glEnable(GL_DEPTH_TEST); 
// le shader
   programID = LoadShaders( "PhongShader.vert", "PhongShader.frag" );
 glEnable( GL_PROGRAM_POINT_SIZE );
 //  glPointSize(30.);
 glEnable(GL_BLEND);
 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   // Get  handles for our matrix transformations "MVP" VIEW  MODELuniform
  MatrixIDMVP = glGetUniformLocation(programID, "MVP");
//  MatrixIDView = glGetUniformLocation(programID, "VIEW");
  MatrixIDModel = glGetUniformLocation(programID, "MODEL");
 // MatrixIDPerspective = glGetUniformLocation(programID, "PERSPECTIVE");

  // Projection matrix : 65 Field of View, 1:1 ratio, display range : 1 unit <-> 1000 units
  // ATTENTIOn l'angle est donné en radians si f GLM_FORCE_RADIANS est défini sinon en degré
  Projection = glm::perspective( glm::radians(60.f), 1.0f, 1.0f, 1000.0f);


}

void anim( int NumTimer)
{
  using namespace std::chrono;
  static int i=0;
  static time_point<system_clock> refTime = system_clock::now()  ;

    time_point<system_clock> currentTime = system_clock::now(); // This and "end"'s type is std::chrono::time_point

    duration<double> deltaTime = currentTime - refTime;

  int delatTemps = duration_cast<milliseconds>( deltaTime).count() ; // temps  écoulé en millisecondes depuis le dernier appel de anim
  refTime =currentTime ;
  i++; // nb de passages : à utiliser pour initialiser la tirage aleatoire

  // AFAIRE renouvelez ici les particules et réaliser le calcul de simulation
  // Création de nouvelles particules
  creationParticules(systemeParticules.tauxCreation);
  // Mise à jour des positions et vitesses de toutes les particules
  miseAJourParticules(delatTemps);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);

  glBufferData(GL_ARRAY_BUFFER, listeParticules.size()*sizeof(Particule),listeParticules.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glutPostRedisplay();
  glutTimerFunc(25,anim,1 );
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
  glutCreateWindow("FONTAINE A PARTICULES");


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


   //creationParticules(systemeParticules.tauxCreation);

   genereVBO();
  

  /* enregistrement des fonctions de rappel */
  glutDisplayFunc(affichage);
  glutKeyboardFunc(clavier);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotion);
  glutTimerFunc(200, anim, 1);
  /* Entree dans la boucle principale glut */
  glutMainLoop();

  glDeleteProgram(programID);
  deleteVBO();
  return 0;
}

void genereVBO ()
{

    if(glIsBuffer(VBO_sommets) == GL_TRUE) glDeleteBuffers(1, &VBO_sommets);
    glGenBuffers(1, &VBO_sommets);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);

    glBufferData(GL_ARRAY_BUFFER, listeParticules.size()*sizeof(Particule),listeParticules.data() , GL_DYNAMIC_DRAW);
    glGenBuffers(1, &VAO);
    glEnableVertexAttribArray(indexVertex);
    glEnableVertexAttribArray(indexVitesse);

    glBindVertexArray(VAO); // ici on bind le VAO , c'est lui qui recupèrera les configurations des VBO glVertexAttribPointer , glEnableVertexAttribArray...
    glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);

    glVertexAttribPointer(indexVertex, 3, GL_FLOAT, GL_FALSE,sizeof(Particule),reinterpret_cast<void*>( offsetof(Particule, position)));
    glVertexAttribPointer(indexVitesse, 3, GL_FLOAT, GL_FALSE,sizeof(Particule),reinterpret_cast<void*>( offsetof(Particule, vitesse)));

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

  glClearColor(1.,1.,1.,0.0);
  glClearDepth(10.0f);                         // 0 is near, >0 is far
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glColor3f(1.0,1.0,1.0);
  glPointSize(2.0);
 
     View       = glm::lookAt(   cameraPosition, // Camera is at (0,0,3), in World Space
                                            glm::vec3(0,0,1), // and looks at the origin
                                            glm::vec3(0,0,1)  // Head is up (set to 0,-1,0 to look upside-down)
                                             );
     Model = glm::mat4(1.0f);
     Model = glm::rotate(Model,glm::radians(cameraAngleX),glm::vec3(1, 0, 0) );
     Model = glm::rotate(Model,glm::radians(cameraAngleY),glm::vec3(0, 1, 0) );
     Model = glm::scale(Model,glm::vec3(.8, .8, .8)*cameraDistance);
     MVP = Projection * View * Model;
     traceObjet();        // trace VBO avec ou sans shader

   glutPostRedisplay();
   glutSwapBuffers();
}




//-------------------------------------
void traceObjet()
//-------------------------------------
{
 // Use  shader & MVP matrix   MVP = Projection * View * Model;
 glUseProgram(programID);

//on envoie les données necessaires aux shaders */
 glUniformMatrix4fv(MatrixIDMVP, 1, GL_FALSE, &MVP[0][0]);
 //glUniformMatrix4fv(MatrixIDView, 1, GL_FALSE,&View[0][0]);
 glUniformMatrix4fv(MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
 //glUniformMatrix4fv(MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);

 
//pour l'affichage

        glBindVertexArray(VAO); // on active le VAO
  glDrawArrays(GL_POINTS,0,listeParticules.size());
   glBindVertexArray(0);    // on desactive les VAO

  glUseProgram(0);         // et le pg

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
      LightPosition.x-=.2;
      glutPostRedisplay();
      break;
    case 'X' : /* Affichage en mode sommets seuls */
      LightPosition.x+=.2;
      glutPostRedisplay();
      break;
    case 'y' : /* Affichage en mode sommets seuls */
      LightPosition.y-=.2;
      glutPostRedisplay();
      break;
    case 'Y' : /* Affichage en mode sommets seuls */
      LightPosition.y+=.2;
      glutPostRedisplay();
      break;
    case 'z' : /* Affichage en mode sommets seuls */
      LightPosition.z-=.2;
      glutPostRedisplay();
      break;
    case 'Z' : /* Affichage en mode sommets seuls */
      LightPosition.z+=.2;
      glutPostRedisplay();
      break;
    case 'a' : /* Affichage en mode sommets seuls */
      LightAmbientCoefficient-=.1;
      glutPostRedisplay();
      break;
    case 'A' : /* Affichage en mode sommets seuls */
      LightAmbientCoefficient+=.1;
      glutPostRedisplay();
      break;
    case '+' : /* Augmente le taux de création de particules*/
      systemeParticules.tauxCreation += 1;
      if(systemeParticules.tauxCreation > 200) systemeParticules.tauxCreation = 200;
      std::cout << "Taux de creation des particules actuel : " << systemeParticules.tauxCreation << std::endl;
      break;
    case '-' : /*Diminue le taux de création des particules*/
      systemeParticules.tauxCreation -= 1;
      if(systemeParticules.tauxCreation < 0) systemeParticules.tauxCreation = 0;
      std::cout << "Taux de creation des particules actuel : " << systemeParticules.tauxCreation << std::endl;
      break;
    case 'l' : /*Diminue la durée de vie des particules*/
      systemeParticules.dureeVie -= 0.5f;
      if(systemeParticules.dureeVie < 0.1f) systemeParticules.dureeVie = 0.1f;
      std::cout << "Duree de vie actuelle des particules : " << systemeParticules.dureeVie << " secondes" << std::endl;
      break;
    case 'L' : /*Augmente la durée de vie des particules*/
      systemeParticules.dureeVie += 0.5f;
      if(systemeParticules.dureeVie > 10.0f) systemeParticules.dureeVie = 10.0f;
      std::cout << "Duree de vie actuelle des particules : " << systemeParticules.dureeVie << " secondes" << std::endl;
      break;
    case 'h' : /*Diminue la hauteur du jet de la fontaine*/
      fountainHeight -= 2.5f;
      if(fountainHeight < 2.5f) fountainHeight = 2.5f;
      zDir = std::uniform_real_distribution<float>(fountainHeight, (fountainHeight + 0.2*fountainHeight));
      std::cout << "hauteur de la fontaine : " << fountainHeight << " unites" << std::endl;
      break;
    case 'H' : /*Diminue la hauteur du jet de la fontaine*/
      fountainHeight += 2.5f;
      if(fountainHeight > 80.0f) fountainHeight = 80.0f;
      zDir = std::uniform_real_distribution<float>(fountainHeight, (fountainHeight + 0.2*fountainHeight));
      std::cout << "hauteur de la fontaine : " << fountainHeight << " unites" << std::endl;
      break;
    case 'r' : /*Diminue le rayon du cône du jet de la fontaine*/
      coneRadius -= 0.5f;
      if(coneRadius < 0.5f) coneRadius = 0.5f;
      std::cout << "diametre du jet de la fontaine : " << coneRadius << " unites" << std::endl;
      break;
    case 'R' : /*Augmente le rayon du cône du jet de la fontaine*/
      coneRadius += 0.5f;
      if(coneRadius > 15.0f) coneRadius = 15.0f;
      std::cout << "diametre du jet de la fontaine : " << coneRadius << " unites" << std::endl;
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

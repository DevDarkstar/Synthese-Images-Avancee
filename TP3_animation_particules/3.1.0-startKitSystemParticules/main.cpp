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


using namespace glm;
using namespace std;

typedef struct {
GLfloat position[3] ;
GLfloat vitesse[3] ;
GLfloat masse;
GLfloat force[3];
} Particule ;

vector<Particule> listeParticules ;


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
int t = 0.01;
// variables Handle d'opengl 
//--------------------------
GLuint programID;   // handle pour le shader
GLuint MatrixIDMVP,MatrixIDView,MatrixIDModel,MatrixIDPerspective;    // handle pour la matrice MVP
GLuint VBO_sommets,VBO_normales, VBO_indices,VBO_UVtext,VAO;


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
void createParticules(int nbParticules )
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
    GLfloat vitesse[3] = {0.0f, 1.0f, 1.0f}; // ainsi qu'une vitesse initiale
    memcpy(p.vitesse, vitesse, 3 * sizeof(GLfloat));
    // Ajout de la nouvelle particule dans la liste des particules
    listeParticules.push_back(p);
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
    // Mise à jour des positions et vitesses de toutes les particules
    /*for(int i = 0; i < listeParticules.size(); i++)
    {
      // Récupération de la position de la particule courante
      GLfloat position[3];
      memcpy(position, listeParticules[i].position, 3 * sizeof(GLfloat)); 
      // Ainsi que sa vitesse
      GLfloat vitesse[3];
      memcpy(vitesse, listeParticules[i].vitesse, 3 * sizeof(GLfloat));
      // et sa force
      GLfloat force[3];
      memcpy(force, listeParticules[i].force, 3 * sizeof(GLfloat));
      // Mise à jour de la nouvelle position
      listeParticules[i].position[0] = position[0] + vitesse[0]*t;
      listeParticules[i].position[1] = position[1] + vitesse[1]*t;
      listeParticules[i].position[2] = position[2] + vitesse[2]*t;

      //Ainsi que sa vitesse
      listeParticules[i].vitesse[0] = vitesse[0] + force[0];
      listeParticules[i].vitesse[1] = vitesse[1] + force[1];
      listeParticules[i].vitesse[2] = vitesse[2] + force[2];

      // Et la force exercée sur la particule
      listeParticules[i].force[0] = listeParticules[i].masse * force[0];
      listeParticules[i].force[1] = listeParticules[i].masse * force[1];
      listeParticules[i].force[2] = listeParticules[i].masse * force[2];
    } */

    glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);

    glBufferData(GL_ARRAY_BUFFER, listeParticules.size()*sizeof(Particule),listeParticules.data() , GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

glutPostRedisplay();
glutTimerFunc(1000,anim,1 );
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


   createParticules(2500);

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

         glVertexAttribPointer ( indexVertex, 3, GL_FLOAT, GL_FALSE,sizeof(Particule),reinterpret_cast<void*>( offsetof(Particule, position)) );
         glVertexAttribPointer ( indexVitesse, 3, GL_FLOAT, GL_FALSE,sizeof(Particule),reinterpret_cast<void*>( offsetof(Particule, vitesse)) );

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








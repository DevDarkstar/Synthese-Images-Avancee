/********************************************************/
/*                     cube.cpp                                                 */
/********************************************************/
/*                Affiche a l'ecran un cube en 3D                      */
/********************************************************/

/* inclusion des fichiers d'en-tete freeglut */
//Faire interpolation par matrice
//Faire interpolation linéaire quaternion (LERP)
//Faire interpolation sphérique quaternion (SLERP)

#ifdef __APPLE__
#include <GLUT/glut.h> /* Pour Mac OS X */
#else
#include <GL/glut.h>   /* Pour les autres systemes */
#endif 
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <armadillo>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/io.hpp>
#include <chrono>
#include <random>
#include "Bone.hpp"

#define SCALE_MIN 0.5
#define SCALE_MAX 2.0
#define ROTATION_MIN -45.0
#define ROTATION_MAX 45.0

using namespace glm;
using namespace std;

//****************************************
#define NB_BRAS 4

// Initialisation de la génération de nombres pseudo-aléatoires
std::random_device rd;
// Création du moteur de génération
std::default_random_engine engine(rd());
std::vector<vec3> color;
std::vector<Bone*> bones;

//****************************************

char presse;
int anglex,angley,x,y,xold,yold;

/* Prototype des fonctions */
void affichage();
void clavier(unsigned char touche,int x,int y);
void reshape(int x,int y);
void idle();
void mouse(int bouton,int etat,int x,int y);
void mousemotion(int x,int y);


void initColors()
{
  std::uniform_real_distribution<float> rgb(0,1);
  color.reserve(NB_BRAS);
  for(int i = 0; i < NB_BRAS; i++)
  {
    color[i] = vec3(rgb(engine), rgb(engine), rgb(engine));
  }
}

void initBones(void)
{
  std::uniform_real_distribution<float> scaleX(SCALE_MIN, SCALE_MAX);
  std::uniform_real_distribution<float> rotation(ROTATION_MIN, ROTATION_MAX);
  float position = 0.0f;
  bones.reserve(NB_BRAS);
  for(int i = 0; i < NB_BRAS; i++)
  {
    float scale = scaleX(engine);
    float angle = rotation(engine);
    Bone* b = new Bone(vec3(0.f+position, 0.f,0.f), vec3(scale, .2f,.2f), angle, 'Z', color[i]);
    position = scale;
    if (i > 0)
      bones[i-1]->add_child(b);
    bones[i] = b;
  }
}


int main(int argc,char **argv)
{
  /* initialisation de glut et creation
     de la fenetre */
  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowPosition(200,200);
  glutInitWindowSize(1500,1500);
  glutCreateWindow("Bras Articule");
  initColors();
  initBones();

  /* Initialisation d'OpenGL */
  glClearColor(0.0,0.0,0.0,0.0);
  glColor3f(1.0,1.0,1.0);
  glPointSize(10.0);
  glEnable(GL_DEPTH_TEST);

  /* enregistrement des fonctions de rappel */
  glutDisplayFunc(affichage);
  glutKeyboardFunc(clavier);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(mousemotion);

  glMatrixMode( GL_PROJECTION );
     glLoadIdentity();
   gluPerspective(60 ,1,.1,30.);

  /* Entree dans la boucle principale glut */
  glutMainLoop();
  return 0;
}

void bras()
{
  bones[0]->draw_bone();
}

void affichage()
{
  int i,j;
  /* effacement de l'image avec la couleur de fond */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glShadeModel(GL_SMOOTH);
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glTranslatef(0.,0.,-5.);
  glRotatef(angley,1.0,0.0,0.0);
  glRotatef(anglex,0.0,1.0,0.0);

        bras();

    //Repère
    //axe x en rouge
    glBegin(GL_LINES);
        glColor3f(1.0,0.0,0.0);
    	glVertex3f(0, 0,0.0);
    	glVertex3f(1, 0,0.0);
    glEnd();
    //axe des y en vert
    glBegin(GL_LINES);
    	glColor3f(0.0,1.0,0.0);
    	glVertex3f(0, 0,0.0);
    	glVertex3f(0, 1,0.0);
    glEnd();
    //axe des z en bleu
    glBegin(GL_LINES);
    	glColor3f(0.0,0.0,1.0);
    	glVertex3f(0, 0,0.0);
    	glVertex3f(0, 0,1.0);
    glEnd();

  glFlush();
  
  //On echange les buffers 
  glutSwapBuffers();
}

void clavier(unsigned char touche,int x,int y)
{
  switch (touche)
  {
    case 'p': /* affichage du carre plein */
      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
      glutPostRedisplay();
      break;
    case 'f': /* affichage en mode fil de fer */
      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
      glutPostRedisplay();
      break;
    case 's' : /* Affichage en mode sommets seuls */
      glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);
      glutPostRedisplay();
      break;
    case 'd':
      glEnable(GL_DEPTH_TEST);
      glutPostRedisplay();
      break;
    case 'D':
      glDisable(GL_DEPTH_TEST);
      glutPostRedisplay();
      break;
    case 'y':
      bones[0]->add_rotation(0.5f);
      glutPostRedisplay();
      break;
    case 'h':
      bones[0]->add_rotation(-0.5f);
      glutPostRedisplay();
      break;
    case 'u':
      bones[1]->add_rotation(0.5f);
      glutPostRedisplay();
      break;
    case 'j':
      bones[1]->add_rotation(-0.5f);
      glutPostRedisplay();
      break;
    case 'i':
      bones[2]->add_rotation(0.5f);
      glutPostRedisplay();
      break;
    case 'k':
      bones[2]->add_rotation(-0.5f);
      glutPostRedisplay();
      break;
    case 'o':
      bones[3]->add_rotation(0.5f);
      glutPostRedisplay();
      break;
    case 'l':
      bones[3]->add_rotation(-0.5f);
      glutPostRedisplay();
      break;
    case 'q' : /*la touche 'q' permet de quitter le programme */
      std::cout << "Suppression des os...\n";
      for(Bone* bone: bones)
      {
        delete bone;
      }
      bones.clear();
      std::cout << "Os supprimes." << std::endl;
      exit(0);
    }
}

void reshape(int x,int y)
{
  if (x<y)
    glViewport(0,(y-x)/2,x,x);
  else 
    glViewport((x-y)/2,0,y,y);
}

void mouse(int button, int state,int x,int y)
{
  /* si on appuie sur le bouton gauche */
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) 
  {
    presse = 1; /* le booleen presse passe a 1 (vrai) */
    xold = x; /* on sauvegarde la position de la souris */
    yold=y;
  }
  /* si on relache le bouton gauche */
  if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) 
    presse=0; /* le booleen presse passe a 0 (faux) */
}

void mousemotion(int x,int y)
  {
    if (presse) /* si le bouton gauche est presse */
    {
      /* on modifie les angles de rotation de l'objet
	 en fonction de la position actuelle de la souris et de la derniere
	 position sauvegardee */
      anglex=anglex+(x-xold); 
      angley=angley+(y-yold);
      glutPostRedisplay(); /* on demande un rafraichissement de l'affichage */
    }
//    else
//    {
//        pointCible.x= ;
//        pointCible.y= ;
//    }

    xold=x; /* sauvegarde des valeurs courante de le position de la souris */
    yold=y;
  }

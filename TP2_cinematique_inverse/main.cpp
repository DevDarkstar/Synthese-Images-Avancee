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

using namespace glm;
using namespace std;

//****************************************
#define NB_BRAS 4
#define K_MAX 10000 // Nombre d'itérations maximales dans le calcul de la cinématique inverse
#define EPS 0.001 // Marge d'erreur dans le calcul de la cinématique inverse

// Initialisation de la génération de nombres pseudo-aléatoires
std::random_device rd;
// Création du moteur de génération
std::default_random_engine engine(rd());
std::vector<arma::fvec> color;
std::vector<Bone*> bones;
arma::fvec target{0.0f, 2.0f, 0.0f};
float bone_scale[NB_BRAS] = {1.0f, 2.0f, 1.0f, 1.0f};
float bone_angle[NB_BRAS] = {0.0f, -90.0f, 90.0f, 0.0f};
float bone_angle_final[NB_BRAS];
arma::fvec effector_position;
float t = 0.0f;
int invertFactor = 1;

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
void anim( int NumTimer) ;
void compute_inverse_kinematic();
void bras(void);


void initColors()
{
  std::uniform_real_distribution<float> rgb(0,1);
  color.reserve(NB_BRAS);
  for(int i = 0; i < NB_BRAS; i++)
  {
    color[i] = arma::fvec{rgb(engine), rgb(engine), rgb(engine)};
  }
}

void initBones(void)
{
  float position = 0.0f;
  bones.reserve(NB_BRAS);
  for(int i = 0; i < NB_BRAS; i++)
  {
    float scale = bone_scale[i];
    Bone* b = new Bone(arma::fvec{0.f+position, 0.f,0.f}, arma::fvec{scale, .2f,.2f}, bone_angle[i], 'Z', color[i]);
    position = scale;
    if (i > 0)
      bones[i-1]->add_child(b);
    bones[i] = b;
  }
}

//Animation de l'interpolation
void anim( int NumTimer)
{
    using namespace std::chrono;
    static int i=0;
    static time_point<system_clock> refTime = system_clock::now()  ;

     time_point<system_clock> currentTime = system_clock::now(); // This and "end"'s type is std::chrono::time_point

      duration<double> deltaTime = currentTime - refTime;

int delatTemps = duration_cast<milliseconds>( deltaTime).count() ;

            t += invertFactor * 0.025f;
            if (t > 1.0f) invertFactor = -1;
            else if(t < 0.0f) invertFactor = 1;
           glutPostRedisplay();
          glutTimerFunc(100,anim,1 );

}


int main(int argc,char **argv)
{
  /* initialisation de glut et creation
     de la fenetre */
  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowPosition(200,200);
  glutInitWindowSize(1500,1500);
  glutCreateWindow("Bras Articule - Cinematique inverse");
  initColors();
  initBones();
  compute_inverse_kinematic();

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
  glutTimerFunc(200, anim, 1);

  glMatrixMode( GL_PROJECTION );
     glLoadIdentity();
   gluPerspective(60 ,1,.1,30.);

  /* Entree dans la boucle principale glut */
  glutMainLoop();
  return 0;
}

arma::fvec get_effector_position()
{
  // Récupération de l'origine du bras articulé
  arma::fvec origin = bones[0]->get_position();
  arma::fmat theta = arma::eye<arma::fmat>(4,4);
  for(int i = 0; i < NB_BRAS; i++)
  {
    theta *= bones[i]->get_bone_transform();
  }
  arma::fvec effector_position = theta * arma::fvec{origin(0), origin(1), origin(2), 1.0f};
  return arma::fvec{effector_position(0), effector_position(1), effector_position(2)};
}

arma::fmat compute_jacobian()
{
  //Initialisation de la Jacobienne
  arma::fmat jacobian_matrix(3, NB_BRAS, arma::fill::zeros);
  // Calcul des coefficients de la Jacobienne
  for(int i = 0; i < NB_BRAS; i++)
  {
    // Récupération de l'axe de rotation de l'os
    arma::fvec axis = bones[i]->get_axis();
    // Récupération de la position de l'os (position de l'extrémité gauche)
    arma::fvec position;
    if(i == 0)
      position = bones[i]->get_position();
    else
    {
      arma::fmat transform = arma::eye<arma::fmat>(4,4);
      for(int j = 0; j <= i; j++)
        transform *= bones[j]->get_bone_transform();
      arma::fvec position_temp = bones[0]->get_position();
      arma::fvec transformation = transform * arma::fvec{position_temp(0), position_temp(1), position_temp(2), 1.0f};
      position = arma::fvec{transformation(0), transformation(1), transformation(2)};
    }
    //std::cout << "position de l'os " << position(0) << " " << position(1) << " " << position(2) << std::endl; 
    // Calcul du produit vectoriel entre la position de l'os et l'axe de rotation de celui-ci
    arma::fvec normal = arma::normalise(arma::cross(axis, effector_position - position));
    // Ajout du vecteur colonne résultant dans la Jacobienne
    jacobian_matrix.col(i) = normal;
  }
  return jacobian_matrix;
}

void compute_inverse_kinematic()
{
  // Calcul de la position courante de l'effecteur
  effector_position = get_effector_position();
  std::cout << "Position initiale de l'effecteur : " << effector_position(0) << " " << effector_position(1) << " " << effector_position(2) << std::endl;
  // Calcul de l'erreur entre la position de l'effecteur et la cible (distance entre les deux points)
  arma::fvec E = target - effector_position;
  int k = 0;
  for(; k < K_MAX && arma::norm(E) > EPS; k++)
  {
    // Récupération de la Jacobienne
    arma::fmat J = compute_jacobian();
    // Calcul de sa pseudo-inverse
    arma::fmat J_plus = arma::pinv(J);
    // Calcul du produit entre la pseudo-inverse de la Jacobienne et la marge d'erreur entre la position de l'effecteur et la cible
    arma::fvec Lambda = J_plus * E;
    // Nous faisons ensuite en sorte qu'aucune valeur contenue dans le vecteur Lambda ne dépasse 2
    float max_lambda = arma::max(Lambda);
    if(max_lambda > 2.0f)
      Lambda *= (2.0f / max_lambda);
    // Mise à jour des angles des os du bras articulé en ajoutant les nouvelles valeurs présentes dans le vecteur Lambda
    for(int i = 0; i < NB_BRAS; i++)
    {
      bones[i]->add_rotation(Lambda(i) * 180.0f / PI);
    }
    // Récupération de la nouvelle position de l'effecteur
    effector_position = get_effector_position();
    // Calcul de la nouvelle erreur
    E = target - effector_position; 
  }
  
  std::cout << "Position finale de l'effecteur : " << effector_position(0) << " " << effector_position(1) << " " << effector_position(2) << std::endl;
  std::cout << "Position finale trouvee en " << k << " iterations." << std::endl; 
  // Une fois le calcul terminé, nous stockons, les valeurs d'angles finales obtenues par la cinématique inverse
  for(int i = 0; i < NB_BRAS; i++)
  {
    bone_angle_final[i] = bones[i]->get_rotation();
  }
}

void bras()
{
  // Calcul des valeurs interpolées des angles des os du bras articulé
  for(int i = 0; i < NB_BRAS; i++)
  {
    bones[i]->set_rotation((1-t)*bone_angle[i] + t*bone_angle_final[i]);
  }
  // Dessin du bras articulé
  bones[0]->draw_bone();
  // Calcul de la position courante de l'effecteur sur le bras articulé
  arma::fvec effector_pos = get_effector_position();
  // dessin de l'effecteur
  glPushMatrix();
    glTranslatef(effector_pos(0), effector_pos(1), effector_pos(2));
    glColor3f(1.0f, 1.0f, 1.0f);
    glutSolidSphere(0.125f, 16, 16);
  glPopMatrix();
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
  // Affichage de la cible à atteindre par le bras articulé
  glPushMatrix();
    glTranslatef(target(0), target(1), target(2));
    glColor3f(0.5f, 0.5f, 0.5f);
    glutSolidSphere(0.125f, 16, 16);
  glPopMatrix();
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
    case 'q' : /*la touche 'q' permet de quitter le programme */
      std::cout << "Suppression des os...\n";
      for(int i = 0; i < NB_BRAS; i++)
      {
        delete bones[i];
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

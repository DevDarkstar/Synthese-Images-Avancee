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
  GLuint creationRate = 5;
  GLfloat lifeTime = 5.0f;
} ParticleSystem;

typedef struct {
  GLfloat position[3] ;
  GLfloat velocity[3] ;
  GLfloat mass;
  GLfloat force[3];
  GLfloat age;
  GLfloat color[3];
} Particle ;

vector<Particle> particles;
ParticleSystem particleSystem;


void anim( int NumTimer) ;

// initialisations

void genereVBO();
void createParticles(int nbParticles ) ;
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
float sphereRadius = 0.01f;
// variables Handle d'opengl 
//--------------------------
GLuint programID;   // handle pour le shader
GLuint MatrixIDMVP,MatrixIDView,MatrixIDModel,MatrixIDPerspective;    // handle pour la matrice MVP
GLuint VBO_sommets,VBO_normales, VBO_indices,VBO_UVtext,VAO;

struct ParticleIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locSphereRadius; // Rayon des Particles
};

ParticleIDs particleIds;

// Initialisation de la génération de nombres pseudo-aléatoires
std::random_device rd;
// Création du moteur de génération
std::default_random_engine engine(rd());
std::uniform_real_distribution<float> angle(0.0f, 2.0f / 3.0f * PI); // Angle du cône à l'intérieur duquel sont projectées les Particles
static std::uniform_real_distribution<float> zDir(fountainHeight, (fountainHeight + 0.2*fountainHeight)); // Poussée verticale appliquée initialement sur les Particles

// location des VBO
//------------------
GLuint indexVertex=0;

//variable pour paramétrage eclairage
//--------------------------------------
vec3 cameraPosition(2.,0.,0.);
// le matériau
//---------------
vec3 materialSpecularColor(0.47,0.71,1.);  // couleur de la spéculaire (bleu ciel)
vec3 materialAmbientColor(1.,1.,1.); // couleur de la lumière ambiante (blanc)
vec3 materialDiffuseColor(0.,1.,1.); // couleur de la lumière diffuse (cyan)

// la lumière
//-----------
vec3 lightPosition(1.,0.,.5);

glm::mat4 MVP;      // justement la voilà
glm::mat4 Model, View, Projection;    // Matrices constituant MVP



int screenHeight = 1500;
int screenWidth = 1500;

//-------------------------
void creationParticles(int nbParticles)
{
  for(int i = 0; i < nbParticles; i++)
  {
    // Création d'une nouvelle particule
    Particle p;
    // On lui définit des propriétés
    p.mass = 0.01f; // Une mass
    GLfloat force[3] = {0.0f, 0.0f, -9.91f}; // Une force à laquelle la particule est soumise
    memcpy(p.force, force, 3 * sizeof(GLfloat));
    GLfloat position[3] = {0.0f, 0.0f, 0.0f}; // Une position initiale
    memcpy(p.position, position, 3 * sizeof(GLfloat));
    GLfloat velocity[3] = {coneRadius * sin(angle(engine)), coneRadius * cos(angle(engine)), zDir(engine)}; // ainsi qu'une velocity initiale
    memcpy(p.velocity, velocity, 3 * sizeof(GLfloat));
    p.age = 0.0f;
    // Ajout de la nouvelle particule dans la liste des Particles
    particles.push_back(p);
  }
}

void updateParticles(int delatTemps)
{
  // Conversion du temps en seconde
  GLfloat temps = delatTemps / 1000.0f;
  for(auto it = particles.begin(); it != particles.end();)
  {
    // Ajout du temps à l'âge de la particule
    (*it).age += temps;
    if ((*it).age > particleSystem.lifeTime)
      particles.erase(it);
    else
    { 
      // Calcul de l'accélération de la particule
      GLfloat acceleration[3] = {(*it).force[0]/(*it).mass, (*it).force[1]/(*it).mass, (*it).force[2]/(*it).mass};

      // Mise à jour de la velocity
      (*it).velocity[0] += acceleration[0]*t;
      (*it).velocity[1] += acceleration[1]*t;
      (*it).velocity[2] += acceleration[2]*t;

      // Mise à jour de la nouvelle position
      (*it).position[0] += (*it).velocity[0]*t;
      (*it).position[1] += (*it).velocity[1]*t;
      (*it).position[2] += (*it).velocity[2]*t;
      
      // Si la particule touche le sol (z = 0)
      if((*it).position[2] <= sphereRadius)
      {
        // On fait en sorte que la particule ne traverse pas le sol
        (*it).position[2] = sphereRadius;
        // On restitue 50% de sa vitesse initiale
        (*it).velocity[2] *= -0.5f;
      }
      ++it;
    }
  }
}

// Récupération des emplacements des variables uniformes pour le shader des particules
void getUniformLocationParticles(ParticleIDs& particle){
  //Chargement des vertex, geometry fragment shaders pour les particules
  particle.programID = LoadShadersWithGeom("ParticlesShader.vert", "ParticlesShader.geom", "ParticlesShader.frag");
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  particle.MatrixIDView = glGetUniformLocation(particle.programID, "VIEW");
  particle.MatrixIDModel = glGetUniformLocation(particle.programID, "MODEL");
  particle.MatrixIDPerspective = glGetUniformLocation(particle.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables uniformes du shader des particles
  particle.locSphereRadius = glGetUniformLocation(particle.programID, "radius");
}

// Affectation de valeurs pour les variables uniformes du shader des particules
void setParticlesUniformValues(ParticleIDs& particle){
  //on envoie les données necessaires aux shaders */
  glUniformMatrix4fv(particle.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(particle.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(particle.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);

  glUniform1f(particle.locSphereRadius, sphereRadius);
}


//----------------------------------------
void initOpenGL(void)
//----------------------------------------
{
  glCullFace (GL_BACK); // on spécifie queil faut éliminer les face arriere
  glEnable(GL_CULL_FACE); // on active l'élimination des faces qui par défaut n'est pas active
  glEnable(GL_DEPTH_TEST); 
 glEnable( GL_PROGRAM_POINT_SIZE );
 //  glPointSize(30.);
 glEnable(GL_BLEND);
 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  getUniformLocationParticles(particleIds);
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

  // AFAIRE renouvelez ici les Particles et réaliser le calcul de simulation
  // Création de nouvelles Particles
  creationParticles(particleSystem.creationRate);
  // Mise à jour des positions et vitesses de toutes les Particles
  updateParticles(delatTemps);
  // Affectation des nouvelles données des particules dans le VBO associé
  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);

  glBufferData(GL_ARRAY_BUFFER, particles.size()*sizeof(Particle),particles.data(), GL_DYNAMIC_DRAW);

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

   //creationParticles(particleSystem.creationRate);

   genereVBO();
  

  /* enregistrement des fonctions de rappel */
  glutDisplayFunc(affichage);
  glutKeyboardFunc(clavier);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotion);
  glutTimerFunc(25, anim, 1);
  /* Entree dans la boucle principale glut */
  glutMainLoop();

  return 0;
}

void genereVBO ()
{

  if(glIsBuffer(VBO_sommets) == GL_TRUE) glDeleteBuffers(1, &VBO_sommets);
  glGenBuffers(1, &VBO_sommets);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);

  glBufferData(GL_ARRAY_BUFFER, particles.size()*sizeof(Particle),particles.data() , GL_DYNAMIC_DRAW);
  glGenBuffers(1, &VAO);
  glEnableVertexAttribArray(indexVertex);

  glBindVertexArray(VAO); // ici on bind le VAO , c'est lui qui recupèrera les configurations des VBO glVertexAttribPointer , glEnableVertexAttribArray...
  glBindBuffer(GL_ARRAY_BUFFER, VBO_sommets);

  glVertexAttribPointer(indexVertex, 3, GL_FLOAT, GL_FALSE,sizeof(Particle),reinterpret_cast<void*>( offsetof(Particle, position)));
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

  glClearColor(0.7,0.7,0.7,1.0);
  glClearDepth(10.0f);                         // 0 is near, >0 is far
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glColor3f(1.,1.,1.);
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
 glUseProgram(particleIds.programID);
  setParticlesUniformValues(particleIds);

 
  //pour l'affichage

  glBindVertexArray(VAO); // on active le VAO
  glDrawArrays(GL_POINTS,0,particles.size());
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
    case '+' : /* Augmente le taux de création de Particles*/
      particleSystem.creationRate += 1;
      if(particleSystem.creationRate > 200) particleSystem.creationRate = 200;
      std::cout << "Taux de creation des Particles actuel : " << particleSystem.creationRate << std::endl;
      break;
    case '-' : /*Diminue le taux de création des Particles*/
      particleSystem.creationRate -= 1;
      if(particleSystem.creationRate < 0) particleSystem.creationRate = 0;
      std::cout << "Taux de creation des Particles actuel : " << particleSystem.creationRate << std::endl;
      break;
    case 'l' : /*Diminue la durée de vie des Particles*/
      particleSystem.lifeTime -= 0.5f;
      if(particleSystem.lifeTime < 0.1f) particleSystem.lifeTime = 0.1f;
      std::cout << "Duree de vie actuelle des Particles : " << particleSystem.lifeTime << " secondes" << std::endl;
      break;
    case 'L' : /*Augmente la durée de vie des Particles*/
      particleSystem.lifeTime += 0.5f;
      if(particleSystem.lifeTime > 10.0f) particleSystem.lifeTime = 10.0f;
      std::cout << "Duree de vie actuelle des Particles : " << particleSystem.lifeTime << " secondes" << std::endl;
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
      coneRadius -= 2.0f;
      if(coneRadius < 2.0f) coneRadius = 2.0f;
      std::cout << "diametre du jet de la fontaine : " << coneRadius << " unites" << std::endl;
      break;
    case 'R' : /*Augmente le rayon du cône du jet de la fontaine*/
      coneRadius += 2.0f;
      if(coneRadius > 30.0f) coneRadius = 30.0f;
      std::cout << "diametre du jet de la fontaine : " << coneRadius << " unites" << std::endl;
      break;
    case 'p' : /*Augmente le rayon des Particles*/
      sphereRadius += 0.005f;
      if(sphereRadius > 0.05f) sphereRadius = 0.05f;
      break;
    case 'm' : /*Diminue le rayon des Particles*/
      sphereRadius -= 0.005f;
      if(sphereRadius < 0.005f) sphereRadius = 0.005f;
      break;
    case 'q' : /*la touche 'q' permet de quitter le programme */
      std::cout << "Désactivation du VAO actif...\n";
      glBindVertexArray(0);
      std::cout << "Désactivation du shader program actif...\n";
      glUseProgram(0);
      std::cout << "Suppression des éléments du programme...\n";
      // Suppression des shader programs
      glDeleteProgram(programID);
      // Ainsi que des VAO, VBOs, framebuffers et textures utilisées dans le programme
      deleteVBO();
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

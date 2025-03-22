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
#define MAX_PARTICULES 10000


using namespace glm;
using namespace std;

typedef struct{
  GLuint creationRate = 10; // Taux de création des particules
} ParticleSystem;

typedef struct {
  GLfloat position[3];
  GLfloat velocity[3];
  GLfloat initialVelocity[3];
  GLfloat mass;
  GLfloat force[3];
  GLfloat age;
  GLfloat initialAge;
} Particle;

typedef struct{
  GLfloat minX = -0.5f;
  GLfloat maxX = 0.5f;
  GLfloat minY = -0.5f;
  GLfloat maxY = 0.5f;
  GLfloat minZ = 0.0f;
  GLfloat maxZ = 1.5f;
}ParticlesBounds;

vector<Particle> particles;
ParticleSystem particlesSystem;
ParticlesBounds particleBounds;


void anim( int NumTimer) ;

// initialisations

void genereSSBOParticles(void);
void createParticles(int nbParticles);
void deleteSSBOParticles();
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
float t = 0.001f; // Variable de discrétisation du temps
float coneRadius = 10.0f; // Rayon de projection des particules
static float fountainHeight = 60.0f; // Hauteur de la projection en +z des particules émises
float sphereRadius = 0.01f; // Rayon des particules
int nbParticles = 0; // Nombre de particules actuellement créées
int refreshedTime = 0; // temps écoulé en millisecondes depuis le dernier appel de anim
// variables Handle d'opengl 
//--------------------------
GLuint SSBO_particles; // Gestionnaire du SSBO

struct ParticlesIDs{
  GLuint programID; // Gestionnaire du "shader program"
  GLuint computeProgramID; // Gestionnaire du compute shader
  GLuint MatrixIDView,MatrixIDModel,MatrixIDPerspective; // Matrices modèle, vue et projection
  GLuint locSphereRadiusGeom; // Rayon des particules dans le geometry shader
  GLuint locSphereRadiusComp; // Rayon des particules dans le compute shader
  GLuint locDeltaTime; // Variable de discrétisation du temps
  GLuint locRefreshedTime; // Temps écoulé entre 2 appels de la fonction anim
  GLuint locMinX; // Limite de position des particules en -x
  GLuint locMaxX; // Limite de position des particules en +x
  GLuint locMinY; // Limite de position des particules en -y
  GLuint locMaxY; // Limite de position des particules en +y
  GLuint locMinZ; // Limite de position des particules en -z
  GLuint locMaxZ; // Limite de position des particules en +z
};

ParticlesIDs particlesIds;

// Initialisation de la génération de nombres pseudo-aléatoires
std::random_device rd;
// Création du moteur de génération
std::default_random_engine engine(rd());
std::uniform_real_distribution<float> angle(0.0f, 2.0f / 3.0f * PI); // Angle du cône à l'intérieur duquel sont projectées les particules
static std::uniform_real_distribution<float> zDir(fountainHeight, (fountainHeight + 0.2*fountainHeight)); // Poussée verticale appliquée initialement sur les particules
std::uniform_real_distribution<float> lifeTime(2.0f, 10.0f); // Durée de vie des particules

// location des VBO
//------------------
GLuint indexVertex=0;

//variable pour paramétrage eclairage
//--------------------------------------
vec3 cameraPosition(2.,0.,0.);

// la lumière
//-----------
vec3 lightPosition(1.,0.,.5);

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
void creationParticles(int nbParticles)
{
  for(int i = 0; i < nbParticles; i++)
  {
    // Création d'une nouvelle particule
    Particle p;
    // On lui définit des propriétés
    p.mass = 0.01f; // Une masse
    GLfloat force[3] = {0.0f, 0.0f, -9.91f}; // Une force à laquelle la particule est soumise
    memcpy(p.force, force, 3 * sizeof(GLfloat));
    GLfloat position[3] = {0.0f, 0.0f, 0.0f}; // Une position initiale
    memcpy(p.position, position, 3 * sizeof(GLfloat));
    GLfloat velocity[3] = {coneRadius * sin(angle(engine)), coneRadius * cos(angle(engine)), zDir(engine)}; // ainsi qu'une vitesse initiale
    memcpy(p.velocity, velocity, 3 * sizeof(GLfloat));
    memcpy(p.initialVelocity, velocity, 3 * sizeof(GLfloat));
    float lifeT = lifeTime(engine);
    p.age = lifeT;
    p.initialAge = lifeT;
    // Ajout de la nouvelle particule dans la liste des particules
    particles.push_back(p);
  }
}

// Récupération des emplacements des variables uniformes pour le shader des particules
void getUniformLocationParticles(ParticlesIDs& particle){
  //Chargement des vertex, geometry et fragment shaders des particules
  particle.programID = LoadShadersWithGeom("ParticlesShader.vert", "ParticlesShader.geom", "ParticlesShader.frag");
  // Ainsi que le compute shader permettant de mettre à jour les velocitys et les positions des nouvelles particules
  particle.computeProgramID = LoadComputeShader("ParticlesShader.comp");
  // Récupération des emplacements des matrices modèle, vue et projection dans les shaders
  particle.MatrixIDView = glGetUniformLocation(particle.programID, "VIEW");
  particle.MatrixIDModel = glGetUniformLocation(particle.programID, "MODEL");
  particle.MatrixIDPerspective = glGetUniformLocation(particle.programID, "PERSPECTIVE");

  // Récupération des emplacements des variables unfiformes du shader des particules
  particle.locSphereRadiusGeom = glGetUniformLocation(particle.programID, "radius");
  particle.locSphereRadiusComp = glGetUniformLocation(particle.computeProgramID, "radius");
  particle.locDeltaTime = glGetUniformLocation(particle.computeProgramID, "deltaTime");
  particle.locRefreshedTime = glGetUniformLocation(particle.computeProgramID, "refreshedTime");
  particle.locMinX = glGetUniformLocation(particle.computeProgramID, "bounds.minX");
  particle.locMaxX = glGetUniformLocation(particle.computeProgramID, "bounds.maxX");
  particle.locMinY = glGetUniformLocation(particle.computeProgramID, "bounds.minY");
  particle.locMaxY = glGetUniformLocation(particle.computeProgramID, "bounds.maxY");
  particle.locMinZ = glGetUniformLocation(particle.computeProgramID, "bounds.minZ");
  particle.locMaxZ = glGetUniformLocation(particle.computeProgramID, "bounds.maxZ");
}

// Affectation de valeurs pour les variables uniformes du shader des particules
void setParticlesUniformValues(ParticlesIDs& particle){
  //on envoie les données necessaires aux shaders */
  glUniformMatrix4fv(particle.MatrixIDView, 1, GL_FALSE,&View[0][0]);
  glUniformMatrix4fv(particle.MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
  glUniformMatrix4fv(particle.MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);

  glUniform1f(particle.locSphereRadiusGeom, sphereRadius);
}

// Affectation de valeurs pour les variables uniformes du compute shader
void setParticlesUniformValuesComputeShader(ParticlesIDs& particle){
  //on envoie les données necessaires au compute shader */
  glUniform1f(particle.locSphereRadiusComp, sphereRadius);
  glUniform1f(particle.locDeltaTime, t);
  glUniform1i(particle.locRefreshedTime, refreshedTime);
  glUniform1f(particle.locMinX, particleBounds.minX);
  glUniform1f(particle.locMaxX, particleBounds.maxX);
  glUniform1f(particle.locMinY, particleBounds.minY);
  glUniform1f(particle.locMaxY, particleBounds.maxY);
  glUniform1f(particle.locMinZ, particleBounds.minZ);
  glUniform1f(particle.locMaxZ, particleBounds.maxZ);
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
  getUniformLocationParticles(particlesIds);
  // Projection matrix : 65 Field of View, 1:1 ratio, display range : 1 unit <-> 1000 units
  // ATTENTIOn l'angle est donné en radians si f GLM_FORCE_RADIANS est défini sinon en degré
  Projection = glm::perspective( glm::radians(60.f), 1.0f, 1.0f, 1000.0f);


}

void anim( int NumTimer)
{
  using namespace std::chrono;
  static time_point<system_clock> refTime = system_clock::now()  ;

  time_point<system_clock> currentTime = system_clock::now(); // This and "end"'s type is std::chrono::time_point

  duration<double> deltaTime = currentTime - refTime;

  refreshedTime = duration_cast<milliseconds>( deltaTime).count() ; // temps  écoulé en millisecondes depuis le dernier appel de anim
  refTime =currentTime ;

  // Récupération du nombre de particules actuelles
  int currentNumberOfParticles = nbParticles;
  // Si le nombre de particules ne dépasse pas la limite de particules à créer
  if (currentNumberOfParticles < MAX_PARTICULES)
  {
    // Calcul du nombre de particules à créer
    int particlesToCreate = std::min((int)particlesSystem.creationRate, MAX_PARTICULES - currentNumberOfParticles);

    // AFAIRE renouvelez ici les particules et réaliser le calcul de simulation
    // Création de nouvelles particules
    creationParticles(particlesToCreate);

    // Mise à jour du SSBO en modifiant uniquement les données des nouvelles particules créées
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_particles);

    glNamedBufferSubData(SSBO_particles, currentNumberOfParticles * sizeof(Particle), particlesToCreate * sizeof(Particle), &particles[currentNumberOfParticles]);

    // Mise à jour du nombre de particules créées
    nbParticles += particlesToCreate;
  }

  // Mise à jour des positions et vitesses des particules actuellement créées
  // Utilisation du shader program du compute shader
  glUseProgram(particlesIds.computeProgramID);
  // Afffectation du contenu des varibles uniformes
  setParticlesUniformValuesComputeShader(particlesIds);
  glDispatchCompute(64, 1, 1);

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

  glutPostRedisplay();
  glutTimerFunc(25,anim,1);
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
  glutCreateWindow("FONTAINE A PARTICULES V2");


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

  genereSSBOParticles();
  

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

void genereSSBOParticles(void){
  glGenBuffers(1, &SSBO_particles);
  // Utilisation du SSBO
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_particles);
  // Affectation des données des particules au SSBO
  glNamedBufferStorage(SSBO_particles, sizeof(Particle) * MAX_PARTICULES, nullptr, GL_DYNAMIC_STORAGE_BIT);
  // On effectue le lien entre le SSBO et le point de binding 0 dans le shader program
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO_particles);
  // Désactivation du SSBO une fois la paramétrisation terminée
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 
}

void deleteSSBOParticles(void)
{
  glDeleteBuffers(1, &SSBO_particles);
}

//-------------------------------------
void traceObjet()
//-------------------------------------
{
  // Use  shader & MVP matrix   MVP = Projection * View * Model;
  glUseProgram(particlesIds.programID);
  setParticlesUniformValues(particlesIds);

 
  //pour l'affichage
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
    case '+' : /* Augmente le taux de création de particules*/
      particlesSystem.creationRate += 1;
      if(particlesSystem.creationRate > 200) particlesSystem.creationRate = 200;
      std::cout << "Taux de creation des particules actuel : " << particlesSystem.creationRate << std::endl;
      break;
    case '-' : /*Diminue le taux de création des particules*/
      particlesSystem.creationRate -= 1;
      if(particlesSystem.creationRate < 0) particlesSystem.creationRate = 0;
      std::cout << "Taux de creation des particules actuel : " << particlesSystem.creationRate << std::endl;
      break;
    case 'p' : /*Augmente le rayon des particules*/
      sphereRadius += 0.005f;
      if(sphereRadius > 0.05f) sphereRadius = 0.05f;
      break;
    case 'm' : /*Diminue le rayon des particules*/
      sphereRadius -= 0.005f;
      if(sphereRadius < 0.005f) sphereRadius = 0.005f;
      break;
    case 'q' : /*la touche 'q' permet de quitter le programme */
      std::cout << "Désactivation du shader program actif...\n";
      glUseProgram(0);
      std::cout << "Suppression des éléments du programme...\n";
      // Suppression des shader programs
      glDeleteProgram(particlesIds.programID);
      glDeleteProgram(particlesIds.computeProgramID);
      // Ainsi que le SSBO
      deleteSSBOParticles();
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

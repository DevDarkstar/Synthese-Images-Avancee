#include "Bone.hpp"
#ifdef __APPLE__
#include <GLUT/glut.h> /* Pour Mac OS X */
#else
#include <GL/glut.h>   /* Pour les autres systemes */
#endif 

Bone::Bone(const arma::fvec& position, const arma::fvec& size, const float angle, const char axis, const arma::fvec& color) : bone_children()
{
    this->position = position;
    this->size = size;
    this->axis = (axis == 'X') ? arma::fvec{1.,0.,0.} : (axis == 'Y') ? arma::fvec{0.,1.,0.} : arma::fvec{0.,0.,1.};
    this->color = color;
    this->angle = angle;
}

arma::fvec Bone::get_position()
{
    return position;
}

arma::fvec Bone::get_size()
{
    return size;
}

arma::fvec Bone::get_color()
{
    return color;
}

arma::fvec Bone::get_axis()
{
    return axis;
}

float Bone::get_rotation()
{
    return angle;
}

arma::fmat Bone::get_bone_transform()
{
    // application de la rotation de l'os
    arma::fmat rotation_matrix = arma::eye<arma::fmat>(4, 4);
    float cosinus = std::cos(angle * PI / 180.0f);
    float sinus = std::sin(angle * PI / 180.0f);

    rotation_matrix(0, 0) = cosinus;  
    rotation_matrix(0, 1) = -sinus;
    rotation_matrix(1, 0) = sinus;  
    rotation_matrix(1, 1) = cosinus;

    // application de la translation de l'os
    arma::fmat translation_matrix = arma::eye<arma::fmat>(4, 4);

    translation_matrix(0, 3) = this->get_size()(0);
    translation_matrix(1, 3) = 0.0f;
    translation_matrix(2, 3) = 0.0f;
    
    return rotation_matrix * translation_matrix;
}

void Bone::add_rotation(float value)
{
    this->angle += value;
}

void Bone::add_child(Bone* bone)
{
    this->bone_children.push_back(bone);
}

void Bone::draw_bone(void){
    // Récupération des données nécessaires au dessin de l'os
    arma::fvec position = this->get_position();
    arma::fvec size = this->get_size();
    arma::fvec color = this->get_color();
    arma::fvec axis = this->get_axis();
    glPushMatrix();
    glTranslatef(position(0), position(1), position(2));
    glRotatef(this->angle, axis(0), axis(1), axis(2));
    glPushMatrix();
        glScalef(size(0), size(1), size(2));
        glTranslatef(0.5f, 0.0f, 0.0f);
        glColor3f(color(0),color(1),color(2));  
        glutSolidCube(1.);
    glPopMatrix();  
    for(Bone* bone : this->bone_children){
        bone->draw_bone();
    };
    glPopMatrix();
}

#include "Bone.hpp"
#ifdef __APPLE__
#include <GLUT/glut.h> /* Pour Mac OS X */
#else
#include <GL/glut.h>   /* Pour les autres systemes */
#endif 

Bone::Bone(const glm::vec3& position, const glm::vec3& size, const float angle, const char axis, const glm::vec3& color) : bone_children()
{
    this->position = position;
    this->size = size;
    this->axis = (axis == 'X') ? glm::vec3(1.,0.,0.) : (axis == 'Y') ? glm::vec3(0.,1.,0.) : glm::vec3(0.,0.,1.);
    this->color = color;
    this->angle = angle;
}

glm::vec3 Bone::get_position()
{
    return position;
}

glm::vec3 Bone::get_size()
{
    return size;
}

glm::vec3 Bone::get_color()
{
    return color;
}

glm::vec3 Bone::get_axis()
{
    return axis;
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
    glm::vec3 position = this->get_position();
    glm::vec3 size = this->get_size();
    glm::vec3 color = this->get_color();
    glm::vec3 axis = this->get_axis();
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(this->angle, axis.x, axis.y, axis.z);
    glPushMatrix();
        glScalef(size.x, size.y, size.z);
        glTranslatef(0.5f, 0.0f, 0.0f);
        glColor3f(color.r,color.g,color.b);  
        glutSolidCube(1.);
    glPopMatrix();  
    for(Bone* bone : this->bone_children){
        bone->draw_bone();
    };
    glPopMatrix();
}
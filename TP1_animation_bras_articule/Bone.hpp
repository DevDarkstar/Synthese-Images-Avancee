#ifndef BONE_HPP
#define BONE_HPP

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

class Bone{
public:
    Bone(const glm::vec3& position, const glm::vec3& size, const float angle, const char axis, const glm::vec3& color);
    glm::vec3 get_position();
    glm::vec3 get_size();
    glm::vec3 get_color(); 
    glm::vec3 get_axis();
    void add_child(Bone* bone);
    void add_rotation(float value);
    void draw_bone();
    float angle;
private:
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 axis;
    std::vector<Bone*> bone_children;
    glm::vec3 color;
};

#endif

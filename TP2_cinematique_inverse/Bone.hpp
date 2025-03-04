#ifndef BONE_HPP
#define BONE_HPP

#define PI 3.141592653589793

#include <vector>
#include <armadillo>
#include <iostream>

class Bone{
public:
    Bone(const arma::fvec& position, const arma::fvec& size, const float angle, const char axis, const arma::fvec& color);
    arma::fvec get_position();
    arma::fvec get_size();
    arma::fvec get_color(); 
    arma::fvec get_axis();
    arma::fmat get_bone_transform();
    float get_rotation();
    void set_rotation(float value);
    void add_child(Bone* bone);
    void add_rotation(float value);
    void draw_bone(); 
private:
    arma::fvec position;
    arma::fvec size;
    arma::fvec axis;
    std::vector<Bone*> bone_children;
    arma::fvec color;
    float angle;
};

#endif

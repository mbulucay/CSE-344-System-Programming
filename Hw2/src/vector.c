#include "vector.h"

Vector3 createVector3_string(char* str){
    Vector3 v3;

    v3.x = str[0];
    v3.y = str[1];
    v3.z = str[2];

    return v3;
}


Vector3 createVector3_ints(int _x, int _y, int _z){

    Vector3 v3;

    v3.x = _x;
    v3.y = _y;
    v3.z = _z;

    return v3;
}


Matrix createMatrix(double var_x, double var_y, double var_z, double cov_xy, double cov_xz, double cov_yz){

    Matrix m;

    m.matrix[0][0] = var_x;
    m.matrix[0][1] = cov_xy;
    m.matrix[0][2] = cov_xz;

    m.matrix[1][0] = cov_xy;
    m.matrix[1][1] = var_y;
    m.matrix[1][2] = cov_yz;
    
    m.matrix[2][0] = cov_xz;
    m.matrix[2][1] = cov_yz;
    m.matrix[2][2] = var_z;

    return m;
}

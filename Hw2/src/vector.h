
#define SIZE 3

#ifndef VECTOR_H_
#define VECTOR_H_

    typedef struct Vector3{
        int x, y, z;
    }Vector3;

    Vector3 createVector3_string(char* str);
    Vector3 createVector3_ints(int _x, int _y, int _z);

    typedef struct Matrix{
        double matrix[SIZE][SIZE];
    }Matrix;

    Matrix createMatrix(double var_x, double var_y, double var_z, double cov_xy, double cov_xz, double cov_yz);

#endif // !VECTOR_H_


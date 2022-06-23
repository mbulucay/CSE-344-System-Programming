#ifndef STRUCTS_H_
#define STRUCTS_H_

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const double PI = 3.14159265358979323846;

typedef struct _thread_args{

    int start, end, id;

}thread_args;


typedef struct _thread_resp{

    int start, end;
    double **t_real_matrix;
    double **t_imag_matrix;

}_thread_resp;


typedef struct Matrix{

    int n;
    double** matrix;

}Matrix;


typedef struct complex{

    double real, imag;

}complex;


void free_response(_thread_resp* resp){

    for(int i=0; i<resp->end - resp->start; ++i){
        free(resp->t_real_matrix[i]);
        free(resp->t_imag_matrix[i]);
    }

    free(resp->t_imag_matrix);  
    free(resp->t_real_matrix);
    free(resp);
}

void free_matrix(Matrix* matrix){
    for(int i=0; i<matrix->n; ++i){
        free(matrix->matrix[i]);
    }
    free(matrix->matrix);
}


void init_response(_thread_resp* resp, int s, int e, int n){

    resp->start = s;
    resp->end = e;
    resp->t_real_matrix = (double**) calloc(sizeof(double*), (e - s));
    resp->t_imag_matrix = (double**) calloc(sizeof(double*), (e - s));

    for(int i=0; i<(e - s); ++i){
        resp->t_real_matrix[i] = (double*) calloc(sizeof(double), n);
        resp->t_imag_matrix[i] = (double*) calloc(sizeof(double), n);
    }

}

void init_matrix(Matrix* matrix, int n){

    matrix->n = n;
    matrix->matrix = (double**) calloc(sizeof(double*), matrix->n);
    for(int i=0; i<matrix->n; ++i){
        matrix->matrix[i] = (double*) calloc(sizeof(double), matrix->n);
    }
}


void place_the_matrix(Matrix* matrix, int n, char* matrix_content){

    for(int i=0; i<n; ++i){
        for(int j=0; j<n; ++j){
            matrix->matrix[i][j] = (double) matrix_content[(i*n) + j];
        }
    }
}

void print_matrix(Matrix* matrix){

    for(int i=0; i<matrix->n; ++i){
        for(int j=0; j<matrix->n; ++j){
            printf("%.0lf ", matrix->matrix[i][j]);
        }
        printf("\n");
    }

}

void print_complex_matrix(Matrix* result_real_matrix, Matrix* result_img_matrix){

    for(int i=0; i<result_real_matrix->n; ++i){
        for(int j=0; j<result_real_matrix->n; ++j){
            fprintf(stdout, "+%.3lf+", result_real_matrix->matrix[i][j]);
            fprintf(stdout, "%.3lfi ", result_img_matrix->matrix[i][j]);
            fprintf(stdout, "(%d, %d)\n", i, j);
        }
        printf("\n");
    }

}


#endif // !STRUCTS_H_#define

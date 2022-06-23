#include "structs.h"

void findCoFactor(int** mat, int** mat2, int p, int q, int n) {
   
    int i = 0, j = 0;

    for (int row = 0; row < n; row++) {
        for (int col = 0; col < n; col++) {
            if (row != p && col != q) {
                mat2[i][j++] = mat[row][col];
                if (j == n - 1) {
                    j = 0;
                    i++;
                }
            }
        }
    }
}


int getDeterminant(int** mat, int n) {
   int determinant = 0;
   if (n == 1)
      return mat[0][0];
   int** temp;
   temp = (int **) malloc(20 * sizeof(int*));
    
   for(int i =0; i< n ;i++){
      temp[i] = (int *)malloc(n *sizeof(int));
   } 

   int sign = 1;
   for (int f = 0; f < n; f++) {
      findCoFactor(mat, temp, 0, f, n);
      determinant += sign * mat[0][f] * getDeterminant(temp, n - 1);
      sign = -sign;
   }

   for(int k = 0;k < n ; k++){
      free(temp[k]);
   }
   free(temp);

   return determinant;
}

int isMatrixInvertible(int **mat, int n) {

   int r = getDeterminant(mat, n) != 0 ? 1 : 0;

   for(int i = 0; i< n ;i++)
      free(mat[i]);
   free(mat);
   return r;
}

int isMatrixInvert(int *arr, int size){

   int **a = (int **)malloc(size * sizeof(int *));
   for(int i = 0; i < size; i++){
      a[i] = (int *)malloc(size * sizeof(int));
   }

   for(int i = 0; i < size; i++){
      for(int j = 0; j < size; j++){
         a[i][j] = arr[i*size + j];
      }
   }

   return isMatrixInvertible(a, size);
}

int is_invertable(Matrix* matrix){
    return isMatrixInvert(matrix->array, matrix->size);
}

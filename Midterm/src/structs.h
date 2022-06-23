 #ifndef STRUTC_H_
#define STRUTC_H_


typedef struct{
    int array[4096];
    int size;
}Matrix;


typedef struct{
    Matrix matrix;
    int client_id;
}request;

typedef struct{
    int result;
    long sec;
    long msec;
    int worker_number;
}response;


#endif // !STRUTC_
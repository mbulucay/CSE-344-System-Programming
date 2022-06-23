#ifndef QUEUE_H
#define QUEUE_H


typedef struct node {
    struct node *next;
    int* data;
}node;


void enqueue(node **head, int* data);
int* dequeue(node **head);


#endif // QUEUE_H
#ifndef BRICKYARD_H
#define BRICKYARD_H
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/sem.h>

#define MAX_CONVEYOR_BRICKS_NUMBER 15
#define MAX_CONVEYOR_BRICKS_WEIGHT 22
#define CONVEYOR_TRANSPORT_TIME 15
#define WORKER_PICKUP_TIME_W1 1000000
#define WORKER_PICKUP_TIME_W2 2000000
#define WORKER_PICKUP_TIME_W3 3000000

extern key_t key_add, key_remove, key_capacity, key_weight;
extern int semid_add_brick, semid_remove_brick, semid_conveyor_capacity, semid_weight_capacity;

typedef struct Brick {
    int id;
    int weight;
    time_t added_time;
} Brick;

typedef struct ConveyorBelt {
    Brick bricks[MAX_CONVEYOR_BRICKS_NUMBER];
    int front;
    int rear;
    int lastBrickId;
} ConveyorBelt;

typedef struct Node {
    int workerId;
    int brickWeight;
    struct Node* next; 
} Node;

typedef struct Queue {
    Node* front; 
    Node* rear;  
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Queue;

extern Queue queue;

void initializeConveyor(ConveyorBelt* q);
void addBrick(ConveyorBelt* q, int id, int brick_weight);
void removeBrick(ConveyorBelt* q);
void checkAndUnloadBricks(ConveyorBelt* q);
void* monitorConveyor(void* arg);

void worker(int workerId, ConveyorBelt* conveyor, const int* worker_pickup_times);
void tryAddingBrick(int workerId, int brickWeight, ConveyorBelt* conveyor);
void* chceckQueue(void* arg);
void initializeQueue(Queue* q);
void addToQueue(Queue* q, int workerId, int brickWeight);
Node* removeFromQueue(Queue* q);
#endif

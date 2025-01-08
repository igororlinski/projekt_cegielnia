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
#define CONVEYOR_TRANSPORT_TIME 10
#define WORKER_PICKUP_TIME_W1 1000000
#define WORKER_PICKUP_TIME_W2 2000000
#define WORKER_PICKUP_TIME_W3 3000000
#define MAX_TRUCK_CAPACITY 80
#define TRUCK_RETURN_TIME 150
#define TRUCK_NUMBER 5

typedef struct Brick {
    int id;
    int weight;
    time_t added_time;
} Brick;

typedef struct ConveyorBelt {
    Brick bricks[MAX_CONVEYOR_BRICKS_NUMBER];
    int front;
    int rear;
    int last_brick_id;
} ConveyorBelt;

typedef struct Truck {
    int id;
    int current_weight;
    int max_capacity;
} Truck;

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
extern struct Truck trucks[TRUCK_NUMBER];
extern key_t key_add, key_remove, key_capacity, key_weight;
extern int semid_add_brick, semid_remove_brick, semid_conveyor_capacity, semid_weight_capacity;
extern volatile sig_atomic_t continue_production;

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

void initializeTrucks(int numTrucks);
void* truckWorker(void* arg);
Truck assignBrickToTruck(Brick brick);
void returnTruck(Truck* truck);

#endif

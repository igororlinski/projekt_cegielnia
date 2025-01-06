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
//#define MAX_CONVEYOR_BRICKS_WEIGHT 35
#define CONVEYOR_TRANSPORT_TIME 10
#define WORKER_PICKUP_TIME_W1 1000000
#define WORKER_PICKUP_TIME_W2 2100000
#define WORKER_PICKUP_TIME_W3 2430000

typedef struct Brick {
    int id;
    //int weight;
    time_t added_time;
} Brick;

typedef struct ConveyorBelt {
    Brick bricks[MAX_CONVEYOR_BRICKS_NUMBER];
    int front;
    int rear;
    int size;
    int lastBrickId;
} ConveyorBelt;

void initializeConveyor(ConveyorBelt* q);
void addBrick(ConveyorBelt* q, int id);
void removeBrick(ConveyorBelt* q);
void checkAndUnloadBricks(ConveyorBelt* q);
void* monitorConveyor(void* arg);
void worker(int workerId, ConveyorBelt* conveyor, const int* worker_pickup_times);

#endif

#ifndef BRICKYARD_H
#define BRICKYARD_H
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#define MAX_CONVEYOR_BRICKS_NUMBER 12
//#define MAX_CONVEYOR_BRICKS_WEIGHT 35
#define CONVEYOR_TRANSPORT_TIME 5

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
} ConveyorBelt;

void initializeConveyor(ConveyorBelt* q);
void addBrick(ConveyorBelt* q, int id);
void removeBrick(ConveyorBelt* q);
void checkAndUnloadBricks(ConveyorBelt* q);
void* monitorConveyor(void* arg);
void worker(int workerId, ConveyorBelt* conveyor);

#endif

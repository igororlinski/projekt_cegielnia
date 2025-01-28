#ifndef BRICKYARD_H
#define BRICKYARD_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <malloc.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h> 
#include <errno.h>

#define SLEEP_TIME 0
#define CONVEYOR_TRANSPORT_TIME 5
#define WORKER_MIN_TIME 10000
#define TRUCK_RETURN_TIME 100

#define BRICK_STORAGE_SIZE 200
#define MAX_BRICK_SIZE 3
#define MAX_CONVEYOR_BRICKS_NUMBER 15
#define MAX_CONVEYOR_BRICKS_WEIGHT 22
#define MAX_TRUCK_CAPACITY 20
#define TRUCK_NUMBER 5

typedef struct Brick {
    int id;
    char weight[MAX_BRICK_SIZE];
    struct timeval added_time;
    clock_t ad;
} Brick;

typedef struct ConveyorBelt {
    Brick bricks[MAX_CONVEYOR_BRICKS_NUMBER];
    int front;
    int rear;
    int last_brick_id;
    pthread_mutex_t mutex;
} ConveyorBelt;

struct MsgBuffer {
    long mtype;
    int workerId;
};

typedef struct Truck {
    int id;
    int current_weight;
    int max_capacity;
    int in_transit;
    pid_t pid;
    pthread_mutex_t mutex;
    size_t next;
} Truck;

typedef struct TruckQueue {
    size_t front;
    size_t rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TruckQueue;

struct sembuf p = {0, -1, 0};
struct sembuf v = {0, 1, 0};

int getBrickWeight(Brick* brick) {
    int brick_weight = 0;
    for (int i = 0; i < MAX_BRICK_SIZE; i++) {
        if (brick->weight[i] != '1')
            return brick_weight;
        brick_weight++;
    }
    return brick_weight;
}

Truck* get_truck(TruckQueue* queue, size_t offset, Truck* shm_base) {
    (void)queue;
    if (offset == 0) return NULL; 
    return (Truck*)((char*)shm_base + offset - 72);
}

#endif

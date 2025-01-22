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

#define SLEEP_TIME 100000
#define CONVEYOR_TRANSPORT_TIME 50
#define WORKER_PICKUP_TIME_W1 10
#define WORKER_PICKUP_TIME_W2 20
#define WORKER_PICKUP_TIME_W3 30
#define TRUCK_RETURN_TIME 100

#define BRICK_STORAGE_SIZE 90
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

typedef struct Truck {
    int id;
    int current_weight;
    int max_capacity;
    int msg_queue_id;
    pthread_mutex_t mutex;
    struct Truck* next;
} Truck;

typedef struct Node {
    int workerId;
    Brick* brick;
    struct Node* next; 
} Node;

typedef struct WorkerQueue {
    Node* front; 
    Node* rear;  
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} WorkerQueue;

struct MsgBuffer {
    long mtype;
    int msg_data;
};

typedef struct TruckQueue {
    Truck* front;
    Truck* rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TruckQueue;

extern struct sembuf p;
extern struct sembuf v;

extern ConveyorBelt* conveyor;
extern key_t key_add, key_remove, key_capacity, key_weight;
extern int semid_add_brick, semid_remove_brick, semid_conveyor_capacity, semid_weight_capacity;
extern volatile sig_atomic_t continue_production;
extern WorkerQueue worker_queue;
extern Truck sharedTrucks[TRUCK_NUMBER];
extern TruckQueue* truck_queue;
extern char *brick_storage;

void initializeConveyor(ConveyorBelt* q);
void addBrick(ConveyorBelt* q, int id, Brick* brick);
void conveyorCheckAndUnloadBricks(ConveyorBelt* q);

void worker(int workerId, ConveyorBelt* conveyor, const int* worker_pickup_times, int lower_limit, int upper_limit);
int tryAddingBrick(int workerId, ConveyorBelt* conveyor, char* storage, int lower_limit);
void* checkWorkerQueue(void* arg);
void initializeWorkerQueue(WorkerQueue* q);
void addWorkerToQueue(WorkerQueue* q, int workerId, Brick* brick);
Node* removeWorkerFromQueue(WorkerQueue* q);
int getBrickWeight(Brick* brick);

void initializeTrucks(Truck* sharedTrucks, int numTrucks);
void truckProcess(Truck* this_truck);
Truck* assignBrickToTruck(Brick* brick);
void addTruckToQueue(TruckQueue* q, Truck* truck);
Truck* removeTruckFromQueue(TruckQueue* q, Truck* truck);
void initializeTruckQueue(TruckQueue* q);
void sendTruck(Truck* truck);

//void* dispatcher(ConveyorBelt* conveyor);

#endif

#ifndef BRICKYARD_H
#define BRICKYARD_H
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/msg.h>


#define MAX_CONVEYOR_BRICKS_NUMBER 15
#define MAX_CONVEYOR_BRICKS_WEIGHT 22
#define CONVEYOR_TRANSPORT_TIME 2
#define WORKER_PICKUP_TIME_W1 1000000
#define WORKER_PICKUP_TIME_W2 1000000
#define WORKER_PICKUP_TIME_W3 1000000
#define MAX_TRUCK_CAPACITY 20
#define TRUCK_RETURN_TIME 1
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
    int msg_queue_id;
    pthread_mutex_t mutex;
    struct Truck* next;
} Truck;

typedef struct Node {
    int workerId;
    int brickWeight;
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

extern WorkerQueue worker_queue;
extern struct Truck sharedTrucks[TRUCK_NUMBER];
extern key_t key_add, key_remove, key_capacity, key_weight;
extern int semid_add_brick, semid_remove_brick, semid_conveyor_capacity, semid_weight_capacity;
extern volatile sig_atomic_t continue_production;
extern TruckQueue* truck_queue;

void initializeConveyor(ConveyorBelt* q);
void addBrick(ConveyorBelt* q, int id, int brick_weight);
void checkAndUnloadBricks(ConveyorBelt* q);

void worker(int workerId, ConveyorBelt* conveyor, const int* worker_pickup_times);
void tryAddingBrick(int workerId, int brickWeight, ConveyorBelt* conveyor);
void* chceckWorkerQueue(void* arg);
void initializeWorkerQueue(WorkerQueue* q);
void addWorkerToQueue(WorkerQueue* q, int workerId, int brickWeight);
Node* removeWorkerFromQueue(WorkerQueue* q);

void initializeTrucks(Truck* sharedTrucks, int numTrucks);
void* truckWorker(void* arg);
Truck* assignBrickToTruck(Brick brick);
void addTruckToQueue(TruckQueue* q, Truck* truck);
Truck* removeTruckFromQueue(TruckQueue* q, Truck* truck);
void initializeTruckQueue(TruckQueue* q);
void sendTruck(Truck* truck);

void* dispatcher(ConveyorBelt* conveyor);

#endif

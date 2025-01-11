#include "brickyard.h"

void worker(int workerId, ConveyorBelt* conveyor, const int* worker_pickup_times) {
    while (1) {
        usleep(worker_pickup_times[workerId-1]);
        tryAddingBrick(workerId, workerId, conveyor);
    }
}

void tryAddingBrick(int workerId, int brickWeight, ConveyorBelt* conveyor) {
    if (semctl(semid_conveyor_capacity, 0, GETVAL) == 0 || semctl(semid_weight_capacity, 0, GETVAL) < brickWeight) {
        if (semctl(semid_weight_capacity, 0, GETVAL) < brickWeight) 
            printf("Pracownik P%d czeka na dodanie cegły o wadze %d na taśmę z powodu przeciążenia taśmy.\n", workerId, brickWeight);
        else
            printf("Pracownik P%d czeka na dodanie cegły o wadze %d na taśmę z powodu zapełnienia taśmy.\n", workerId, brickWeight);

        addWorkerToQueue(&worker_queue, workerId, brickWeight); 
    }
    addBrick(conveyor, workerId, brickWeight);
}

void initializeWorkerQueue(WorkerQueue* q) {
    q->front = NULL;
    q->rear = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void addWorkerToQueue(WorkerQueue* q, int workerId, int brickWeight) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->workerId = workerId;
    newNode->brickWeight = brickWeight;
    newNode->next = NULL;

    pthread_mutex_lock(&q->mutex);

    if (q->rear == NULL) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }

    if (semctl(semid_conveyor_capacity, 0, GETVAL) > 0 && semctl(semid_weight_capacity, 0, GETVAL) >= brickWeight) {
        pthread_cond_signal(&q->cond);
    }

    pthread_mutex_unlock(&q->mutex);
}

Node* removeWorkerFromQueue(WorkerQueue* q) {
    pthread_mutex_lock(&q->mutex);

    while (q->front == NULL) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    Node* temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    pthread_mutex_unlock(&q->mutex);
    return temp;
}

void* chceckWorkerQueue(void* arg) {
    ConveyorBelt* conveyor = (ConveyorBelt*)arg;

    while (1) {
        Node* node = removeWorkerFromQueue(&worker_queue);

        if (node == NULL) {
            usleep(SLEEP_TIME);
            continue;
        }

        while (semctl(semid_conveyor_capacity, 0, GETVAL) <= 0 || semctl(semid_weight_capacity, 0, GETVAL) < node->brickWeight) {
            addWorkerToQueue(&worker_queue, node->workerId, node->brickWeight);
            free(node);
            usleep(10*SLEEP_TIME);
            continue;
        }

        addBrick(conveyor, node->workerId, node->brickWeight);
        free(node);

        pthread_mutex_lock(&worker_queue.mutex);
        pthread_cond_broadcast(&worker_queue.cond);
        pthread_mutex_unlock(&worker_queue.mutex);

        
        usleep(10*SLEEP_TIME);
    }

    return NULL;
}
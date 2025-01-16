#include "brickyard.h"

int getBrickWeight(Brick* brick) {
    return (malloc_usable_size(brick->weight)/8);
}

int getBrickId(ConveyorBelt* conveyor) {
    conveyor->last_brick_id++;
    return conveyor->last_brick_id;
}

void createBrick(Brick *brick, int workerId) {
    brick->id = getBrickId(conveyor);
    brick->weight = malloc(workerId); 
    if (getBrickWeight(brick) == 0) {
        perror("Błąd alokacji pamięci dla cegły");
        exit(EXIT_FAILURE);
    }

    memset(brick->weight, 0, workerId);
}

void tryAddingBrick(Brick* brick, int workerId, ConveyorBelt* conveyor) {
    int brickWeight = getBrickWeight(brick);
    if (semctl(semid_conveyor_capacity, 0, GETVAL) == 0 || semctl(semid_weight_capacity, 0, GETVAL) < brickWeight) {
        if (semctl(semid_weight_capacity, 0, GETVAL) < brickWeight) 
            printf("Pracownik P%d czeka na dodanie cegły o wadze %d na taśmę z powodu przeciążenia taśmy.\n", workerId, brickWeight);
        else
            printf("Pracownik P%d czeka na dodanie cegły o wadze %d na taśmę z powodu zapełnienia taśmy.\n", workerId, brickWeight);

        addWorkerToQueue(&worker_queue, workerId, brick); 
    }
    else {
    addBrick(conveyor, workerId, brick);
    }
}

void worker(int workerId, ConveyorBelt* conveyor, const int* worker_pickup_times) {
    while (1) {
        usleep(worker_pickup_times[workerId-1]);
        Brick newBrick;
        createBrick(&newBrick, workerId);
        tryAddingBrick(&newBrick, workerId, conveyor);
    }
}

void initializeWorkerQueue(WorkerQueue* q) {
    q->front = NULL;
    q->rear = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void addWorkerToQueue(WorkerQueue* q, int workerId, Brick* brick) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->workerId = workerId;
    newNode->brick = brick;
    newNode->next = NULL;

    pthread_mutex_lock(&q->mutex);

    if (q->rear == NULL) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }

    if (semctl(semid_conveyor_capacity, 0, GETVAL) > 0 && semctl(semid_weight_capacity, 0, GETVAL) >= getBrickWeight(brick)) {
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

        while (semctl(semid_conveyor_capacity, 0, GETVAL) <= 0 || semctl(semid_weight_capacity, 0, GETVAL) < getBrickWeight(node->brick)) {
            addWorkerToQueue(&worker_queue, node->workerId, node->brick);
            free(node);
            usleep(10*SLEEP_TIME);
            continue;
        }

        addBrick(conveyor, node->workerId, node->brick);
        free(node);

        pthread_mutex_lock(&worker_queue.mutex);
        pthread_cond_broadcast(&worker_queue.cond);
        pthread_mutex_unlock(&worker_queue.mutex);

        
        usleep(10*SLEEP_TIME);
    }

    return NULL;
}
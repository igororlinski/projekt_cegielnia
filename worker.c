#include "brickyard.h"

int getBrickWeight(Brick* brick) {
    int brick_weight = 0;
    for (int i = 0; i < MAX_BRICK_SIZE; i++) {
        if (brick->weight[i] != '1')
            return brick_weight;
        brick_weight++;
    }
    return brick_weight;
}

int getBrickId(ConveyorBelt* conveyor) {
    conveyor->last_brick_id++;
    return conveyor->last_brick_id;
}

int createBrick(Brick* brick, int workerId, char* storage, int lower_limit) {
    for (int i = 0; i < workerId; i++) {
        brick->weight[i] = storage[lower_limit];
        storage[lower_limit] = '\0';
        lower_limit++;
    }
    return lower_limit;
}

int tryAddingBrick(int workerId, ConveyorBelt* conveyor, char* storage, int lower_limit) {
    Brick newBrick;
    
    lower_limit = createBrick(&newBrick, workerId, storage, lower_limit);
    int brick_weight = getBrickWeight(&newBrick);
    struct sembuf op_weight = {0, -brick_weight, 0};

    if(semctl(semid_conveyor_capacity, 0, GETVAL) == 0) {
        printf("Pracownik P%d czeka na dodanie cegły o wadze %d na taśmę z powodu zapełnienia taśmy.\n", workerId, brick_weight);
    }
    else if(semctl(semid_weight_capacity, 0, GETVAL) < brick_weight) {
        printf("Pracownik P%d czeka na dodanie cegły o wadze %d na taśmę z powodu przeciążenia taśmy.\n", workerId, brick_weight);
    }

    semop(semid_add_brick, &p, 1);
    semop(semid_weight_capacity, &op_weight, 1);
    semop(semid_conveyor_capacity, &p, 1);
    addBrick(conveyor, workerId, &newBrick);

    return lower_limit;
}

void worker(int workerId, ConveyorBelt* conveyor, const int* worker_pickup_times, int lower_limit, int upper_limit) {
    while (1) {
        usleep(worker_pickup_times[workerId-1]);
        if (upper_limit - lower_limit < workerId) {
            printf("Pracownik %d załadował wszystkie dostępne cegły i kończy pracę.\n", workerId);
            break;
        }
        lower_limit = tryAddingBrick(workerId, conveyor, brick_storage, lower_limit);
    }
}

void initializeWorkerQueue(WorkerQueue* q) {
    q->front = NULL;
    q->rear = NULL;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&q->mutex, &attr);
    pthread_cond_init(&q->cond, NULL);

    pthread_mutexattr_destroy(&attr);
}

void addWorkerToQueue(WorkerQueue* q, int workerId, Brick* brick) {
    Node* newNode = malloc(sizeof(Node));
    if(newNode == NULL){
        perror("Błąd alokacji pamięci dla Node");
        exit(EXIT_FAILURE);
    }
    newNode->workerId = workerId;
    newNode->brick = brick;
    newNode->next = NULL;

    pthread_mutex_lock(&q->mutex);
    if(q->rear == NULL) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    // Powiadomienie, że do kolejki dodano nowy element
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

// Funkcja usuwająca węzeł z kolejki (standardowo blokująca, gdy kolejka jest pusta)
Node* removeWorkerFromQueue(WorkerQueue* q) {
    Node* node = NULL;
    pthread_mutex_lock(&q->mutex);
    while(q->front == NULL) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    node = q->front;
    q->front = q->front->next;
    if(q->front == NULL)
        q->rear = NULL;
    pthread_mutex_unlock(&q->mutex);
    return node;
}

void* checkWorkerQueue(void* arg) {
    ConveyorBelt* conveyor = (ConveyorBelt*)arg;

    while (continue_production) {
        pthread_mutex_lock(&worker_queue.mutex);
        
        while(worker_queue.front == NULL ||
              semctl(semid_conveyor_capacity, 0, GETVAL) == 0 ||
              semctl(semid_weight_capacity, 0, GETVAL) < getBrickWeight(worker_queue.front->brick)) {
            // Jeśli kolejka jest pusta, lub brak miejsca/wagi, to oczekujemy
            pthread_cond_wait(&worker_queue.cond, &worker_queue.mutex);
        }

       Node* node = worker_queue.front;
        worker_queue.front = node->next;
        if(worker_queue.front == NULL)
            worker_queue.rear = NULL;
        pthread_mutex_unlock(&worker_queue.mutex);

        // Teraz można bezpiecznie dodać cegłę na taśmę – operacja addBrick sama korzysta z semaforów
        addBrick(conveyor, node->workerId, node->brick);
        free(node);

        // Po zmianie stanu taśmy (addBrick) mogą się zmienić warunki dla kolejki – zasygnalizuj oczekującym
        pthread_mutex_lock(&worker_queue.mutex);
        pthread_cond_broadcast(&worker_queue.cond);
        pthread_mutex_unlock(&worker_queue.mutex);
    }
    return NULL;
}
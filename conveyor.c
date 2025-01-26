#include "brickyard.h"

struct sembuf p = {0, -1, 0};
struct sembuf v = {0, 1, 0};

key_t key_add, key_remove, key_capacity, key_weight;
int semid_add_brick, semid_remove_brick, semid_conveyor_capacity, semid_weight_capacity;

void initializeConveyor(ConveyorBelt* q) {
    q->front = 0;
    q->rear = -1;
    q->last_brick_id = 0; 

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&q->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
    key_add = ftok(".", 'A');
    key_remove = ftok(".", 'B');
    key_capacity = ftok(".", 'C');
    key_weight = ftok(".", 'D'); 

    semid_add_brick = semget(key_add, 1, IPC_CREAT | 0600);
    semid_remove_brick = semget(key_remove, 1, IPC_CREAT | 0600);
    semid_conveyor_capacity = semget(key_capacity, 1, IPC_CREAT | 0600); 
    semid_weight_capacity = semget(key_weight, 1, IPC_CREAT | 0600); 

    semctl(semid_add_brick, 0, SETVAL, 1);
    semctl(semid_remove_brick, 0, SETVAL, 1);
    semctl(semid_conveyor_capacity, 0, SETVAL, MAX_CONVEYOR_BRICKS_NUMBER);
    semctl(semid_weight_capacity, 0, SETVAL, MAX_CONVEYOR_BRICKS_WEIGHT);
}

void addBrick(ConveyorBelt* q, int workerId, Brick* brick) {
    q->rear = (q->rear + 1) % MAX_CONVEYOR_BRICKS_NUMBER;
    q->last_brick_id++;
    brick->id = q->last_brick_id;
    brick->ad = clock();

    q->bricks[q->rear] = *brick;
    printf("Pracownik P%d dodał cegłę o wadze %d na taśmę. ID cegły: %d        Liczba cegieł na taśmie: %d      Łączna waga cegieł na taśmie: %d\n", workerId, getBrickWeight(brick), brick->id, MAX_CONVEYOR_BRICKS_NUMBER - semctl(semid_conveyor_capacity, 0, GETVAL), MAX_CONVEYOR_BRICKS_WEIGHT - semctl(semid_weight_capacity, 0, GETVAL));

    semop(semid_add_brick, &v, 1);
}

void removeBrick(ConveyorBelt* q) {
    pthread_mutex_lock(&q->mutex);
    struct sembuf op;

    if (semctl(semid_conveyor_capacity, 0, GETVAL)  == MAX_CONVEYOR_BRICKS_NUMBER) {
        printf("Taśma jest pusta!\n");
        semop(semid_remove_brick, &v, 1);
        return;
    }

    int brick_weight = getBrickWeight(&q->bricks[q->front]);
    int brick_id = q->bricks[q->front].id;
    Truck* assigned_truck;

    while (1) {
    if ((truck_queue->front->max_capacity - truck_queue->front->current_weight) < brick_weight) {
        continue;
    }
    else {
        assigned_truck = assignBrickToTruck(&q->bricks[q->front]);
        break;
    }
    }
    printf("Cegła o wadze %d wpada do ciężarówki nr %d. Zapełnienie ciężarówki: %d/%d. ID cegły: %d\n", brick_weight, assigned_truck->id, assigned_truck->current_weight, assigned_truck->max_capacity, brick_id);
    q->front = (q->front + 1) % MAX_CONVEYOR_BRICKS_NUMBER;
    pthread_mutex_unlock(&q->mutex);

    semop(semid_conveyor_capacity, &v, 1);

    op.sem_num = 0;
    op.sem_op = brick_weight;
    op.sem_flg = 0;
    semop(semid_weight_capacity, &op, 1);

    semop(semid_remove_brick, &v, 1);
}

void conveyorCheckAndUnloadBricks(ConveyorBelt* q) {
    pthread_mutex_lock(&q->mutex);
    if (semctl(semid_conveyor_capacity, 0, GETVAL) == MAX_CONVEYOR_BRICKS_NUMBER || truck_queue->front == NULL) {
        pthread_mutex_unlock(&q->mutex);
        return;
    }   
    double time_taken;
    clock_t n = clock();
    Brick* front_brick = &q->bricks[q->front];
    time_taken = (double)(n - front_brick->ad) * 100 / (double)CLOCKS_PER_SEC;
    pthread_mutex_unlock(&q->mutex);

    if ((semctl(semid_conveyor_capacity, 0, GETVAL) < MAX_CONVEYOR_BRICKS_NUMBER) && (time_taken >= (double)(CONVEYOR_TRANSPORT_TIME * SLEEP_TIME)/1000000)) {
        removeBrick(q);
    }
}
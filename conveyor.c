#include "brickyard.h"

key_t key_add, key_remove, key_capacity;
int semid_add_brick;
int semid_remove_brick;
int semid_conveyor_capacity;

void initializeConveyor(ConveyorBelt* q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    q->lastBrickId = 0; 

    key_add = ftok(".", 'A');
    key_remove = ftok(".", 'B');
    key_capacity = ftok(".", 'C');

    semid_add_brick = semget(key_add, 1, IPC_CREAT | 0600);
    semid_remove_brick = semget(key_remove, 1, IPC_CREAT | 0600);
    semid_conveyor_capacity = semget(key_capacity, 1, IPC_CREAT | 0600); 

    semctl(semid_add_brick, 0, SETVAL, 1);
    semctl(semid_remove_brick, 0, SETVAL, 1);
    semctl(semid_conveyor_capacity, 0, SETVAL, MAX_CONVEYOR_BRICKS_NUMBER);
}

void addBrick(ConveyorBelt* q, int workerId) {
    struct sembuf op;
 
    op.sem_num = 0;
    op.sem_op = -1;
    semop(semid_add_brick, &op, 1);

    if (semctl(semid_conveyor_capacity, 0, GETVAL) == 0) {
        printf("Taśma jest pełna! Pracownik %d nie mógł dodać kolejnej cegły.\n", workerId);
        op.sem_op = 1;
        semop(semid_add_brick, &op, 1);
        return;
    }

    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    semop(semid_conveyor_capacity, &op, 1); 
    
    q->lastBrickId++;
    q->rear = (q->rear + 1) % MAX_CONVEYOR_BRICKS_NUMBER;
    q->bricks[q->rear].id = q->lastBrickId;
    q->bricks[q->rear].added_time = time(NULL);

    printf("Pracownik %d dodał cegłę o ID %d na taśmę. Liczba cegieł na taśmie: %d\n", workerId, q->lastBrickId, MAX_CONVEYOR_BRICKS_NUMBER - semctl(semid_conveyor_capacity, 0, GETVAL));
    
    op.sem_op = 1;
    semop(semid_add_brick, &op, 1);
}

void removeBrick(ConveyorBelt* q) {
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    semop(semid_remove_brick, &op, 1);

    if (semctl(semid_conveyor_capacity, 0, GETVAL)  == MAX_CONVEYOR_BRICKS_NUMBER) {
        printf("Taśma jest pusta! Nic do wrzucenia do ciężarówki.\n");
        op.sem_op = 1;
        semop(semid_remove_brick, &op, 1);
        return;
    }

    printf("\nCegła o ID %d wpada do ciężarówki.\n", q->bricks[q->front].id);
    q->front = (q->front + 1) % MAX_CONVEYOR_BRICKS_NUMBER;
    q->size--;

    op.sem_num = 0;
    op.sem_op = 1;
    semop(semid_conveyor_capacity, &op, 1);

    op.sem_op = 1;
    semop(semid_remove_brick, &op, 1);
}

void checkAndUnloadBricks(ConveyorBelt* q) {
    time_t now = time(NULL);
    while ((semctl(semid_conveyor_capacity, 0, GETVAL) < MAX_CONVEYOR_BRICKS_NUMBER) && (difftime(now, q->bricks[q->front].added_time) >= CONVEYOR_TRANSPORT_TIME)) {
        removeBrick(q);
    }
}

void* monitorConveyor(void* arg) {
    ConveyorBelt* q = (ConveyorBelt*)arg;

    while (1) {
        checkAndUnloadBricks(q);
        usleep(10000);
    }

    return NULL;
}


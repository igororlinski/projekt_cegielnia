#include "brickyard.h"

key_t key_add, key_remove, key_capacity, key_weight;
int semid_add_brick, semid_remove_brick, semid_conveyor_capacity, semid_weight_capacity;

void initializeConveyor(ConveyorBelt* q) {
    q->front = 0;
    q->rear = -1;
    q->last_brick_id = 0; 

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

void addBrick(ConveyorBelt* q, int workerId, int brick_weight) {
    struct sembuf op;
 
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    semop(semid_add_brick, &op, 1);

    op.sem_num = 0;
    op.sem_op = -1;
    semop(semid_conveyor_capacity, &op, 1); 

    op.sem_num = 0;
    op.sem_op = -brick_weight;
    semop(semid_weight_capacity, &op, 1); 
    
    q->last_brick_id++;
    q->rear = (q->rear + 1) % MAX_CONVEYOR_BRICKS_NUMBER;
    q->bricks[q->rear].id = q->last_brick_id;
    q->bricks[q->rear].added_time = clock();
    q->bricks[q->rear].weight = brick_weight;

    printf("Pracownik P%d dodał cegłę o wadze %d na taśmę. ID cegły: %d        Liczba cegieł na taśmie: %d      Łączna waga cegieł na taśmie: %d\n", workerId, brick_weight, q->last_brick_id, MAX_CONVEYOR_BRICKS_NUMBER - semctl(semid_conveyor_capacity, 0, GETVAL), MAX_CONVEYOR_BRICKS_WEIGHT - semctl(semid_weight_capacity, 0, GETVAL));
    
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
        printf("Taśma jest pusta!\n");
        op.sem_op = 1;
        semop(semid_remove_brick, &op, 1);
        return;
    }

    Truck* assigned_truck = assignBrickToTruck(q->bricks[q->front]);
    printf("Cegła o wadze %d wpada do ciężarówki nr %d. Zapełnienie ciężarówki: %d/%d. ID cegły: %d\n", q->bricks[q->front].weight, assigned_truck->id, assigned_truck->current_weight, assigned_truck->max_capacity, q->bricks[q->front].id);
    q->front = (q->front + 1) % MAX_CONVEYOR_BRICKS_NUMBER;

    op.sem_num = 0;
    op.sem_op = 1;
    semop(semid_conveyor_capacity, &op, 1);

    op.sem_num = 0;
    op.sem_op = q->bricks[q->front].weight;
    semop(semid_weight_capacity, &op, 1);

    op.sem_op = 1;
    semop(semid_remove_brick, &op, 1);
}

void conveyorCheckAndUnloadBricks(ConveyorBelt* q) {
    clock_t now = clock();
    if ((semctl(semid_conveyor_capacity, 0, GETVAL) < MAX_CONVEYOR_BRICKS_NUMBER) && (((double)(now - q->bricks[q->front].added_time) / CLOCKS_PER_SEC) * 1000 >= CONVEYOR_TRANSPORT_TIME)) {
        removeBrick(q);
    }
}
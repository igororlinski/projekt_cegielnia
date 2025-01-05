#include "brickyard.h"

sem_t add_brick;
sem_t remove_brick;

void initializeConveyor(ConveyorBelt* q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    q->lastBrickId = 0; 

    sem_init(&add_brick, 1, 1);
    sem_init(&remove_brick, 1, 1);
}

void addBrick(ConveyorBelt* q, int workerId) {
    if (q->size == MAX_CONVEYOR_BRICKS_NUMBER) {
        printf("Taśma jest pełna! Pracownik %d nie mógł dodać kolejnej cegły.\n", workerId);
        return;
    }

    sem_wait(&add_brick);
    q->lastBrickId++;

    q->rear = (q->rear + 1) % MAX_CONVEYOR_BRICKS_NUMBER;
    q->bricks[q->rear].id = q->lastBrickId;
    q->bricks[q->rear].added_time = time(NULL);
    q->size++;
    printf("Pracownik %d dodał cegłę o ID %d na taśmę. Liczba cegieł na taśmie: %d\n", workerId, q->lastBrickId, q->size);
    
    sem_post(&add_brick);
}

void removeBrick(ConveyorBelt* q) {
    if (q->size == 0) {
        printf("Taśma jest pusta! Nic do wrzucenia do ciężarówki.\n");
        return;
    }
    sem_wait(&remove_brick);
    printf("\nCegła o ID %d wpada do ciężarówki.\n", q->bricks[q->front].id);
    q->front = (q->front + 1) % MAX_CONVEYOR_BRICKS_NUMBER;
    q->size--;
    sem_post(&remove_brick);
}

void checkAndUnloadBricks(ConveyorBelt* q) {
    time_t now = time(NULL);
    while ((q->size > 0) && (difftime(now, q->bricks[q->front].added_time) >= CONVEYOR_TRANSPORT_TIME)) {
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


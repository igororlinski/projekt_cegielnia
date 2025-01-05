#include "brickyard.h"

void initializeConveyor(ConveyorBelt* q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

void addBrick(ConveyorBelt* q, int id) {
    if (q->size == MAX_CONVEYOR_BRICKS_NUMBER) {
        printf("Taśma jest pełna! Nie można dodać więcej cegieł.\n");
        return;
    }
    q->rear = (q->rear + 1);
    q->bricks[q->rear].id = id;
    q->bricks[q->rear].added_time = time(NULL);
    q->size++;
    printf("Dodano cegłę o ID %d na taśmę.\n", id);
}

void removeBrick(ConveyorBelt* q) {
    if (q->size == 0) {
        printf("Taśma jest pusta! Nic do wrzucenia do ciężarówki.\n");
        return;
    }
    printf("\nCegła o ID %d wpada do ciężarówki.\n", q->bricks[q->front].id);
    q->front = (q->front + 1) % MAX_CONVEYOR_BRICKS_NUMBER;
    q->size--;
}

void checkAndUnloadBricks(ConveyorBelt* q) {
    time_t now = time(NULL);
    while ((q->size != 0) && difftime(now, q->bricks[q->front].added_time) >= CONVEYOR_TRANSPORT_TIME) {
        removeBrick(q);
    }
}

void* monitorConveyor(void* arg) {
    ConveyorBelt* q = (ConveyorBelt*)arg;

    while (1) {
        time_t now = time(NULL);
        while ((q->size != 0) && difftime(now, q->bricks[q->front].added_time) >= CONVEYOR_TRANSPORT_TIME) {
            removeBrick(q);
        }
        usleep(100000); 
    }

    return NULL;
}
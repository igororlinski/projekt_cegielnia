#include "brickyard.h"

void worker(int workerId, ConveyorBelt* conveyor) {
    srand(time(NULL) ^ workerId);

    while (1) {
        int brickId = rand() % 1000; 
        usleep(3500000);

        if (conveyor->size < MAX_CONVEYOR_BRICKS_NUMBER) {
            addBrick(conveyor, brickId);
            printf("Pracownik %d: dodano cegłę o ID %d.\n", workerId, brickId);
        } else {
            printf("Pracownik %d: Taśma jest pełna! Nie można dodać cegły.\n", workerId);
        }
    }
}
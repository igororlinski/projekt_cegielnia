#include "brickyard.h"

void worker(int workerId, ConveyorBelt* conveyor) {
    srand(time(NULL) ^ workerId);

    while (1) {
        int sleepTime = BRICK_PICKUP_TIME + workerId*300000;
        usleep(sleepTime);

        addBrick(conveyor, workerId);
    }
}
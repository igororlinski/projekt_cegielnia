#include "brickyard.h"

void worker(int workerId, ConveyorBelt* conveyor, const int* worker_pickup_times) {
    while (1) {
        usleep(worker_pickup_times[workerId-1]);
        addBrick(conveyor, workerId);
    }
}
#include "brickyard.h"

void initializeTrucks(int numTrucks) {
    for (int i = 0; i < numTrucks; i++) {
        trucks[i].id = i + 1;
        trucks[i].current_weight = 0;
        trucks[i].max_capacity = MAX_TRUCK_CAPACITY;
    }
}

void* truckWorker(void* arg) {
    Truck* truck = (Truck*)arg;

    while (1) {
        if (truck->current_weight == truck->max_capacity) {
            printf("Ciężarówka nr %d odjeżdża z %d jednostkami cegieł.\n", truck->id, truck->current_weight);

            sleep(TRUCK_RETURN_TIME);

            printf("Ciężarówka nr %d wróciła do fabryki.\n", truck->id);
            truck->current_weight = 0;
        }

        usleep(10000);
    }

    return NULL;
}

Truck assignBrickToTruck(Brick brick) {
    int truck_num;
    for (int i = 0; i < TRUCK_NUMBER; i++) {
        if (trucks[i].current_weight < trucks[i].max_capacity) {
        trucks[i].current_weight += brick.weight;
        truck_num = i;
        break;
        }
    }
    return trucks[truck_num];
}

void returnTruck(Truck* truck) {
    printf("Ciężarówka %d powraca po %d sekundach.\n", truck->id, TRUCK_RETURN_TIME);
    sleep(TRUCK_RETURN_TIME);
    truck->current_weight = 0;
}

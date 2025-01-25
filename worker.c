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
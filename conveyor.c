#include "brickyard.h"
int semid_add_brick, semid_remove_brick, semid_conveyor_capacity, semid_weight_capacity;
ConveyorBelt* conveyor;
Truck* sharedTrucks;
TruckQueue* truck_queue;

void attach_to_memory(ConveyorBelt** conveyor, int *semid_conveyor_capacity, int *semid_weight_capacity, int* semid_add_brick, int* semid_remove_brick, Truck **sharedTrucks, TruckQueue **truck_queue);
void removeBrick(ConveyorBelt* q);
void conveyorCheckAndUnloadBricks(ConveyorBelt* q);
Truck* assignBrickToTruck(Brick* brick);

int main() {
    attach_to_memory(&conveyor, &semid_conveyor_capacity, &semid_weight_capacity, &semid_add_brick, &semid_remove_brick, &sharedTrucks, &truck_queue);
    while (1) {
        conveyorCheckAndUnloadBricks(conveyor);
        usleep(SLEEP_TIME/10);
    }
    return 1;
}

void removeBrick(ConveyorBelt* q) {
    pthread_mutex_lock(&q->mutex);
     if (semop(semid_remove_brick, &p, 1) == -1) {
        perror("Error decrementing semaphore for removing bricks");
        pthread_mutex_unlock(&q->mutex);
        return;
    }

    struct sembuf op;

    if (semctl(semid_conveyor_capacity, 0, GETVAL)  == MAX_CONVEYOR_BRICKS_NUMBER) {
        printf("\033[38;5;90m[C] \033[38;5;136mTaśma jest pusta!\033[0m\n");
        pthread_mutex_unlock(&q->mutex);
        return;
    }

    int brick_weight = getBrickWeight(&q->bricks[q->front]);
    int brick_id = q->bricks[q->front].id;
    Truck* assigned_truck;

    while (1) {
    if (get_truck(truck_queue, truck_queue->front, sharedTrucks)->max_capacity - (get_truck(truck_queue, truck_queue->front, sharedTrucks)->current_weight) < brick_weight) {
        continue;
    }
    else {
        assigned_truck = assignBrickToTruck(&q->bricks[q->front]);
        break;
    }
    }
    printf("\033[38;5;124m[-] \033[38;5;242mCegła o wadze\033[0m %d\033[38;5;242m wpada do ciężarówki nr\033[0m %d\033[38;5;242m. Zapełnienie ciężarówki:\033[0m %d\033[38;5;88m/\033[0m%d\033[38;5;242m. ID cegły:\033[0m %d\n", brick_weight, assigned_truck->id, assigned_truck->current_weight, assigned_truck->max_capacity, brick_id);
    q->front = (q->front + 1) % MAX_CONVEYOR_BRICKS_NUMBER;
    pthread_mutex_unlock(&q->mutex);
    if (semop(semid_conveyor_capacity, &v, 1) == -1) {
        perror("Error incrementing conveyor capacity semaphore");
    }

    op.sem_num = 0;
    op.sem_op = brick_weight;
    op.sem_flg = 0;
    if (semop(semid_weight_capacity, &op, 1) == -1) {
        perror("Error incrementing weight capacity semaphore");
    }

    if (semop(semid_remove_brick, &v, 1) == -1) {
        perror("Error incrementing weight capacity semaphore");
    }
}

void conveyorCheckAndUnloadBricks(ConveyorBelt* q) {
    pthread_mutex_lock(&q->mutex);
    if (semctl(semid_conveyor_capacity, 0, GETVAL) == MAX_CONVEYOR_BRICKS_NUMBER || get_truck(truck_queue, truck_queue->front, sharedTrucks) == NULL) {
        pthread_mutex_unlock(&q->mutex);
        return;
    }   
    double time_taken;
    Brick* front_brick = &q->bricks[q->front];
    time_taken =  get_current_time() - front_brick->ad;
    pthread_mutex_unlock(&q->mutex);
    if ((semctl(semid_conveyor_capacity, 0, GETVAL) < MAX_CONVEYOR_BRICKS_NUMBER) && (time_taken >= (double)(CONVEYOR_TRANSPORT_TIME * SLEEP_TIME)/1000000)) {
        removeBrick(q);
    }
}

Truck* assignBrickToTruck(Brick* brick) {
    pthread_mutex_lock(&truck_queue->mutex);
    pthread_mutex_lock(&(get_truck(truck_queue, truck_queue->front, sharedTrucks))->mutex);

    Truck* front_truck = get_truck(truck_queue, truck_queue->front, sharedTrucks);
    front_truck->current_weight += getBrickWeight(brick);
    
    pthread_mutex_unlock(&(get_truck(truck_queue, truck_queue->front, sharedTrucks))->mutex);
    pthread_mutex_unlock(&truck_queue->mutex);
    return front_truck;
}

void attach_to_memory(ConveyorBelt** conveyor, int *semid_conveyor_capacity, int *semid_weight_capacity, int* semid_add_brick, int* semid_remove_brick, Truck **sharedTrucks, TruckQueue **truck_queue) {
    key_t conveyor_key = ftok(".", 'Y');
    key_t key_capacity = ftok(".", 'C');
    key_t key_add = ftok(".", 'A');
    key_t key_weight = ftok(".", 'D');
    key_t key_remove = ftok(".", 'B');
    key_t truck_queue_key = ftok (".", 'Z');
    key_t shared_trucks_key = ftok(".", 'U');
    
    int truck_queue_shm_id = shmget(truck_queue_key, sizeof(TruckQueue), 0600);
    if (truck_queue_shm_id < 0) {
        perror("shmget error");
        exit(1);
    }

    *truck_queue = (TruckQueue*)shmat(truck_queue_shm_id, NULL, 0);
    if (truck_queue == (void*)-1) {
        perror("shmat error");
        exit(1);
    }

    int shmTrucksId = shmget(shared_trucks_key, sizeof(Truck) * (TRUCK_NUMBER + 1), 0600);
    if (shmTrucksId < 0) {
        perror("shmget error");
        exit(1);
    }

    *sharedTrucks = (Truck*)shmat(shmTrucksId, NULL, 0);
    if (sharedTrucks == (void*)-1) {
       perror("shmat error");
      exit(1);
    }
    int conveyor_shm_id = shmget(conveyor_key, sizeof(ConveyorBelt), 0600);
    if (conveyor_shm_id < 0) {
        perror("shmget error");
        exit(1);
    }

    *conveyor = (ConveyorBelt*)shmat(conveyor_shm_id, NULL, 0);
    if (conveyor == (void*)-1) {
        perror("shmat error");
        exit(1);
    }

    *semid_conveyor_capacity = semget(key_capacity, 1, 0600);
    *semid_weight_capacity = semget(key_weight, 1, 0600);
    *semid_add_brick = semget(key_add, 1, 0600);
    *semid_remove_brick = semget(key_remove, 1, 0600);
}



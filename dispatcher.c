#include "brickyard.h"
void* monitorTheProduction(void* arg);
int isQueueFull(TruckQueue* queue);

void* dispatcher(ConveyorBelt* conveyor) {  
    pthread_t monitor_thread;

    if (pthread_create(&monitor_thread, NULL, monitorTheProduction, NULL) != 0) {
        perror("Nie udało się utworzyć wątku monitorującego");
        return NULL;
    }

    while (continue_production) {
        pthread_mutex_lock(&truck_queue->mutex);

        if (truck_queue->front == NULL) {
            if (truck_queue->rear == NULL) {
                pthread_mutex_lock(&conveyor->mutex);
                pthread_mutex_unlock(&truck_queue->mutex);
                printf("Brak ciężarówek w fabryce, taśma zostaje wstrzymana.\n");
                while (truck_queue->front == NULL && truck_queue->rear == NULL) {
                    pthread_cond_wait(&truck_queue->cond, &truck_queue->mutex);
                }
                pthread_mutex_unlock(&conveyor->mutex);
            }
            else {
            printf("Nowa ciężarówka nie została jeszcze podstawiona.\n");
            pthread_mutex_unlock(&truck_queue->mutex);
            continue;
            }
        }

        Truck* frontTruck = truck_queue->front;
        pthread_mutex_lock(&frontTruck->mutex);
        pthread_mutex_unlock(&truck_queue->mutex);
        //pthread_mutex_lock(&conveyor->mutex);

        if (!frontTruck->in_transit && getBrickWeight(&conveyor->bricks[conveyor->front]) > (frontTruck->max_capacity - frontTruck->current_weight)) { 
            frontTruck->in_transit = 1;
            kill(frontTruck->pid, SIGUSR1); 
        }

        //pthread_mutex_unlock(&conveyor->mutex);
        pthread_mutex_unlock(&frontTruck->mutex);  
        usleep(SLEEP_TIME); 
    }

    pthread_cancel(monitor_thread);
    pthread_join(monitor_thread, NULL);
    
    return NULL;
}

void* monitorTheProduction(void* arg) {
    (void)arg;
    int workers_done = 1;
    int not_all_workers_done = 1;
    while (not_all_workers_done) {
        usleep(SLEEP_TIME);
        struct MsgBuffer received_msg;

        if (msgrcv(msg_queue_id, &received_msg, sizeof(int), 0, 0) > 0) {
            workers_done++;
            if (workers_done == 3) {
                not_all_workers_done = 0;
            }
        }
    }
    while (continue_production) {
    printf("workers %d, semafor %d\n", workers_done, semctl(semid_conveyor_capacity, 0, GETVAL));
    if (semctl(semid_conveyor_capacity, 0, GETVAL) == MAX_CONVEYOR_BRICKS_NUMBER && truck_queue->front->current_weight > 0) {
            kill(truck_queue->front->pid, SIGUSR1);
            break;
        }
        else if (semctl(semid_conveyor_capacity, 0, GETVAL) == MAX_CONVEYOR_BRICKS_NUMBER && truck_queue->front->current_weight == 0) {
            break;
            printf("workers %d, semafor %d, kolejka %d\n", workers_done, semctl(semid_conveyor_capacity, 0, GETVAL), isQueueFull(truck_queue));
        }
    }
    while(continue_production) {
        if (isQueueFull(truck_queue) == 1) {
            kill(getppid(), SIGUSR2);
        }
    }
    return NULL;
} 

int isQueueFull(TruckQueue* truck_queue) {
    pthread_mutex_lock(&truck_queue->mutex);
    Truck* current = truck_queue->front;

    int counted_trucks[TRUCK_NUMBER];
    for (int i = 0; i < TRUCK_NUMBER; i++) {
        counted_trucks[i] = 0;
    }

    for (int i = 0; i < TRUCK_NUMBER; i++) {
        if (current->id == counted_trucks[i])
            break;
        if (counted_trucks[i] == 0) {
            counted_trucks[i] = current->id;
            current = current->next;
        }
    }

    pthread_mutex_unlock(&truck_queue->mutex);
    for (int i = 0; i < TRUCK_NUMBER; i++) {
        if (counted_trucks[i] == 0) {
            return 0;
        }
    }
    return 1;
}
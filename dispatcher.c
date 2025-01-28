#include "brickyard.h"
void* monitorTheProduction(void* arg);
int isQueueFull(TruckQueue* queue);
void attach_to_memory(ConveyorBelt** conveyor, TruckQueue** truck_queue, int *msg_queue_id, int *semid_conveyor_capacity, Truck **sharedTrucks);

Truck* sharedTrucks;
ConveyorBelt* conveyor;
TruckQueue* truck_queue;
int msg_queue_id;
int semid_conveyor_capacity;

int main() {  
    attach_to_memory(&conveyor, &truck_queue, &msg_queue_id, &semid_conveyor_capacity, &sharedTrucks);
    pthread_t monitor_thread;

    if (pthread_create(&monitor_thread, NULL, monitorTheProduction, NULL) != 0) {
        perror("Nie udało się utworzyć wątku monitorującego");
        return 1;
    }

    while (1) {
        pthread_mutex_lock(&truck_queue->mutex);

        if (get_truck(truck_queue, truck_queue->front, sharedTrucks) == NULL) {
            if (get_truck(truck_queue, truck_queue->rear, sharedTrucks) == NULL) {
                pthread_mutex_lock(&conveyor->mutex);
                printf("\033[38;5;136m[T] \033[38;5;88mBrak ciężarówek w fabryce, taśma zostaje wstrzymana.\033[0m\n");
                while (get_truck(truck_queue, truck_queue->front, sharedTrucks) == NULL && get_truck(truck_queue, truck_queue->rear, sharedTrucks) == NULL) {
                    pthread_cond_wait(&truck_queue->cond, &truck_queue->mutex);
                }
                pthread_mutex_unlock(&conveyor->mutex);
            }
            else {
            printf("\033[38;5;136m[T] \033[38;5;88mNowa ciężarówka nie została jeszcze podstawiona.\033[0m\n");
            pthread_mutex_unlock(&truck_queue->mutex);
            continue;
            }
        }

        Truck* front_truck = get_truck(truck_queue, truck_queue->front, sharedTrucks);
        pthread_mutex_lock(&front_truck->mutex);

        if (!front_truck->in_transit && getBrickWeight(&conveyor->bricks[conveyor->front]) > (front_truck->max_capacity - front_truck->current_weight)) { 
            front_truck->in_transit = 1;
            kill(front_truck->pid, SIGUSR1); 
        }

        pthread_mutex_unlock(&front_truck->mutex);  
        pthread_mutex_unlock(&truck_queue->mutex);

        usleep(SLEEP_TIME); 
    }

    pthread_cancel(monitor_thread);
    pthread_join(monitor_thread, NULL);
    
    return 1;
}

void* monitorTheProduction(void* arg) {
    (void)arg;
    int workers_done = 0;
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
    Truck* last_truck;
    while (1) {
        usleep(SLEEP_TIME);
        if (semctl(semid_conveyor_capacity, 0, GETVAL) == MAX_CONVEYOR_BRICKS_NUMBER && get_truck(truck_queue, truck_queue->front, sharedTrucks)->current_weight > 0) {
            last_truck = get_truck(truck_queue, truck_queue->front, sharedTrucks);
            pthread_mutex_lock(&truck_queue->mutex);
            pthread_mutex_lock(&last_truck->mutex);
            kill(get_truck(truck_queue, truck_queue->front, sharedTrucks)->pid, SIGUSR1); 
            pthread_mutex_unlock(&last_truck->mutex);  
            pthread_mutex_unlock(&truck_queue->mutex);
            break;
        }
        else if (semctl(semid_conveyor_capacity, 0, GETVAL) == MAX_CONVEYOR_BRICKS_NUMBER && get_truck(truck_queue, truck_queue->front, sharedTrucks)->current_weight == 0) {
            break;
        }
    }
    while(1) {
        usleep(SLEEP_TIME);
        if (isQueueFull(truck_queue) == 1) {
            kill(getppid(), SIGUSR2);
            break;
        }
    }

    return NULL;
} 

int isQueueFull(TruckQueue* truck_queue) {
    pthread_mutex_lock(&truck_queue->mutex);
    Truck* current = get_truck(truck_queue, truck_queue->front, sharedTrucks);

    int counted_trucks[TRUCK_NUMBER];
    for (int i = 0; i < TRUCK_NUMBER; i++) {
        counted_trucks[i] = 0;
    }

    for (int i = 0; i < TRUCK_NUMBER; i++) {
        if (current->id == counted_trucks[i])
            break;
        if (counted_trucks[i] == 0) {
            counted_trucks[i] = current->id;
            current = get_truck(truck_queue, current->next, sharedTrucks);
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

void attach_to_memory(ConveyorBelt** conveyor, TruckQueue** truck_queue, int *msg_queue_id, int *semid_conveyor_capacity, Truck **sharedTrucks) {
    key_t msg_key = ftok(".", 'Q');
    key_t conveyor_key = ftok(".", 'Y');
    key_t truck_queue_key = ftok (".", 'Z');
    key_t key_capacity = ftok(".", 'C');
    key_t shared_trucks_key = ftok(".", 'U');

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

    if (msg_key == -1) {
        perror("ftok failed");
        exit(1);
    }

    *msg_queue_id = msgget(msg_key, 0600);
    if (*msg_queue_id < 0) {
        perror("Nie udało się połączyć z kolejką komunikatów");
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


    *semid_conveyor_capacity = semget(key_capacity, 1, 0600); 
}
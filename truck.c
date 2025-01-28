#define _POSIX_C_SOURCE 199309L
#include <unistd.h>
#include <signal.h>
#include "brickyard.h"
static Truck* sent_truck = NULL;
Truck* sharedTrucks;
TruckQueue* truck_queue;

void attach_to_memory(Truck **sharedTrucks, TruckQueue **truck_queue);
void addTruckToQueue(TruckQueue* truck_queue, Truck* truck, void* sharedTrucks);
Truck* removeTruckFromQueue(TruckQueue* queue, Truck* to_be_removed_truck);
void sendTruck(Truck* truck);

void truckSignalHandler(int sig) {
    switch(sig) {
        case SIGUSR1:
            sendTruck(sent_truck); 
            break;
    }
}

int main(int argc __attribute__((unused)), char *argv[]) {
    attach_to_memory(&sharedTrucks, &truck_queue);
    int this_truck_id = atoi(argv[1]);
    Truck* this_truck = &sharedTrucks[this_truck_id];
    sent_truck = this_truck;

    struct sigaction sa;
    sa.sa_handler = truckSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction error");
        exit(1);
    }
    
    while (1) {
        pthread_mutex_lock(&this_truck->mutex);
 
        if (this_truck->current_weight == this_truck->max_capacity) {
            this_truck->in_transit = 1;
            pthread_mutex_unlock(&this_truck->mutex);
            kill(getpid(), SIGUSR1);
        }
        else {
            pthread_mutex_unlock(&this_truck->mutex);
        }
    }
}


void addTruckToQueue(TruckQueue* truck_queue, Truck* truck, void* sharedTrucks) {
    pthread_mutex_lock(&truck_queue->mutex);

    size_t truck_offset = (char*)truck - (char*)sharedTrucks + 72;

    if (truck_queue->rear == 0) {
        truck_queue->front = truck_offset;
        truck_queue->rear = truck_offset;
    } else {
        Truck* rear_truck = get_truck(truck_queue, truck_queue->rear, sharedTrucks);
        rear_truck->next = truck_offset;
        truck_queue->rear = truck_offset;
    }

    pthread_cond_signal(&truck_queue->cond); 
    pthread_mutex_unlock(&truck_queue->mutex);
}

Truck* removeTruckFromQueue(TruckQueue* queue, Truck* to_be_removed_truck) {
    pthread_mutex_lock(&queue->mutex);

    size_t current_offset = queue->front;
    size_t previous_offset = 0;

    Truck* current = NULL;
    Truck* previous = NULL;

    while (current_offset != 0) {
        current = (get_truck(truck_queue, queue->front, sharedTrucks));
        if (current->id == to_be_removed_truck->id) {
            if (previous == 0) {
                queue->front = current->next;
            } else {
                previous= get_truck(queue, previous_offset, sharedTrucks);
                if (previous != NULL) {
                    previous ->next = current->next;
                }
            }
            if (current->next == 0) {
                queue->rear = previous_offset;
            }

            pthread_mutex_unlock(&queue->mutex);
            return current;
        }

        previous_offset = current_offset;
        current_offset = current->next;
    }

    pthread_mutex_unlock(&queue->mutex);
    return NULL;
}

void sendTruck(Truck* truck) {
    removeTruckFromQueue(truck_queue, truck); 
    printf("Ciężarówka nr %d odjeżdża z %d jednostkami cegieł.\n", truck->id, truck->current_weight);

    usleep(TRUCK_RETURN_TIME*SLEEP_TIME);

    printf("Ciężarówka nr %d wróciła do fabryki.\n", truck->id);
    truck->current_weight = 0;
    truck->in_transit = 0;
    addTruckToQueue(truck_queue, truck, (void*)sharedTrucks);
}

void attach_to_memory(Truck **sharedTrucks, TruckQueue **truck_queue) {
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

}
#define _POSIX_C_SOURCE 199309L
#include <unistd.h>
#include <signal.h>
#include "brickyard.h"

void initializeTrucks(Truck* sharedTrucks, int numTrucks) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    for (int i = 0; i < numTrucks; i++) {
        sharedTrucks[i].id = i + 1;
        sharedTrucks[i].current_weight = 0;
        sharedTrucks[i].max_capacity = MAX_TRUCK_CAPACITY;
        sharedTrucks[i].in_transit = 0;
        pthread_mutex_init(&sharedTrucks[i].mutex, &attr);
        sharedTrucks[i].next = NULL;
    }

    pthread_mutexattr_destroy(&attr);
}

void initializeTruckQueue(TruckQueue* truck_queue) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&truck_queue->mutex, &attr);
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED); 

    pthread_cond_init(&truck_queue->cond, &cond_attr);

    truck_queue->front = NULL;
    truck_queue->rear = NULL;

    pthread_mutexattr_destroy(&attr);
    pthread_condattr_destroy(&cond_attr);
}

static Truck *sent_truck = NULL;

void truckSignalHandler(int sig) {
    switch(sig) {
        case SIGUSR1:
            sendTruck(sent_truck); 
            break;
    }
}

void truckProcess(Truck* this_truck) {
    sent_truck = this_truck;
    struct sigaction sa;
    sa.sa_handler = truckSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction error");
        exit(1);
    }
    
    while (continue_production) {
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

Truck* assignBrickToTruck(Brick* brick) {
    pthread_mutex_lock(&truck_queue->mutex);
    pthread_mutex_lock(&truck_queue->front->mutex);

    truck_queue->front->current_weight += getBrickWeight(brick);
    
    pthread_mutex_unlock(&truck_queue->front->mutex);
    pthread_mutex_unlock(&truck_queue->mutex);
    return truck_queue->front;
}

void addTruckToQueue(TruckQueue* q, Truck* truck) {
    pthread_mutex_lock(&q->mutex);

    if (q->rear == NULL) {
        q->front = truck;
        q->rear = truck;
    } else {
        q->rear->next = truck;
        q->rear = truck;
    }

    pthread_cond_signal(&q->cond); 
    pthread_mutex_unlock(&q->mutex);
}

Truck* removeTruckFromQueue(TruckQueue* queue, Truck* to_be_removed_truck) {
    pthread_mutex_lock(&queue->mutex);

    Truck* current = queue->front;
    Truck* previous = NULL;

    while (current != NULL) {
        if (current->id == to_be_removed_truck->id) {
            if (previous == NULL) {
                queue->front = current->next;
            } else {
                previous->next = current->next;
            }
            if (current == queue->rear) {
                queue->rear = previous;
            }

            pthread_mutex_unlock(&queue->mutex);
            return current;
        }

        previous = current;
        current = current->next;
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
    addTruckToQueue(truck_queue, truck);
}
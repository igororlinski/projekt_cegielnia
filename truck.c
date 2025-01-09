#include "brickyard.h"

void initializeTrucks(Truck* sharedTrucks, int numTrucks) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    
    for (int i = 0; i < numTrucks; i++) {
        sharedTrucks[i].id = i + 1;
        sharedTrucks[i].current_weight = 0;
        sharedTrucks[i].max_capacity = MAX_TRUCK_CAPACITY;
        sharedTrucks[i].msg_queue_id = msgget(IPC_PRIVATE, 0600 | IPC_CREAT);
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

void* truckWorker(void* arg) {
    Truck* this_truck = (Truck*)arg;
    struct MsgBuffer msg;

    while (1) {
        msgrcv(this_truck->msg_queue_id, &msg, sizeof(msg), this_truck->id, 0);

        pthread_mutex_lock(&this_truck->mutex);
        if (this_truck->current_weight == this_truck->max_capacity || msg.msg_data == 1) {
            sendTruck(this_truck);
        }

        pthread_mutex_unlock(&this_truck->mutex);
    }

    return NULL;
}

Truck* assignBrickToTruck(Brick brick) {
    pthread_mutex_lock(&truck_queue->front->mutex);

    truck_queue->front->current_weight += brick.weight;

    pthread_mutex_unlock(&truck_queue->front->mutex);
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

    sleep(TRUCK_RETURN_TIME);

    printf("Ciężarówka nr %d wróciła do fabryki.\n", truck->id);
    truck->current_weight = 0;
    addTruckToQueue(truck_queue, truck);
}
#include "brickyard.h"

void* dispatcher(ConveyorBelt* conveyor) {  
    while (continue_production) {
        pthread_mutex_lock(&truck_queue->mutex);

        if (truck_queue->front == NULL) {
            pthread_mutex_unlock(&truck_queue->mutex);
            continue;
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
        //usleep(1000); 
    }
    return NULL;
}

#include "brickyard.h"

void* dispatcher(ConveyorBelt* conveyor) {  
    struct MsgBuffer msg;

    while (continue_production) {
        pthread_mutex_lock(&truck_queue->mutex);

        if (truck_queue->front == NULL) {
            pthread_mutex_unlock(&truck_queue->mutex);
            usleep(SLEEP_TIME);
            continue;
        }

        Truck* frontTruck = truck_queue->front;
        pthread_mutex_lock(&frontTruck->mutex);
        pthread_mutex_unlock(&truck_queue->mutex);
        
        if (getBrickWeight(conveyor->bricks[conveyor->front]) > (frontTruck->max_capacity - frontTruck->current_weight)) {
            msg.mtype = truck_queue->front->id;
            msg.msg_data = 1;

            msgsnd(truck_queue->front->msg_queue_id, &msg, sizeof(msg), 0);
        }

        pthread_mutex_unlock(&frontTruck->mutex);   
        
        usleep(SLEEP_TIME);
    }
    return NULL;
}

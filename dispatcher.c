#include "brickyard.h"

void* dispatcher(ConveyorBelt* conveyor) {
    struct MsgBuffer msg;

    while (1) {
        pthread_mutex_lock(&truck_queue->front->mutex);

        if (conveyor->bricks[conveyor->front].weight > (truck_queue->front->max_capacity - truck_queue->front->current_weight)) {
            msg.mtype = truck_queue->front->id;
            msg.msg_data = 1;

            msgsnd(truck_queue->front->msg_queue_id, &msg, sizeof(msg), 0);
        }

        pthread_mutex_unlock(&truck_queue->front->mutex);   
        
        usleep(SLEEP_TIME);
    }
    return NULL;
}

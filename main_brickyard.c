#include "brickyard.h"
volatile sig_atomic_t continue_production = 1;
Truck sharedTrucks[TRUCK_NUMBER];
TruckQueue* truck_queue;
ConveyorBelt* conveyor;
char *brick_storage;
int msg_queue_id;
key_t msg_key;

void signalHandler(int sig) {
    switch(sig) {
        case SIGINT:
            continue_production = 0;
            break;
        case SIGUSR2:
            printf("\033[0;31mOtrzymano!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\033[0m\n");
            continue_production = 0;
            break;
    }
}

int main() {
    msg_key = ftok(".", 'Q');

    msg_queue_id = msgget(msg_key, IPC_CREAT | 0600);
    if (msg_queue_id < 0) {
        perror("Nie udało się utworzyć kolejki komunikatów");
        exit(1);
    }

    /*struct msqid_ds buf;
    if (msgctl(msg_queue_id, IPC_STAT, &buf) == -1) {
        perror("msgctl error");
        exit(1);
    }

    while (buf.msg_qnum > 0) {  // Jeśli są jakieś wiadomości w kolejce
        struct MsgBuffer msg;
        msgrcv(msg_queue_id, &msg, sizeof(msg.workerId), 0, IPC_NOWAIT);  // Opróżnij kolejkę
        if (msgctl(msg_queue_id, IPC_STAT, &buf) == -1) {
            perror("msgctl error");
            exit(1);
        }
    }*/

    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        perror("Nie udało się zarejestrować obsługi sygnału");
        exit(1);
    }

    if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
    perror("Nie udało się zarejestrować obsługi sygnału");
    exit(1);
    }

    int shmId = shmget(IPC_PRIVATE, sizeof(ConveyorBelt), IPC_CREAT | 0600);
    if (shmId < 0) {
        perror("shmget error");
        exit(1);
    }

    conveyor = (ConveyorBelt*)shmat(shmId, NULL, 0);
    if (conveyor == (void*)-1) {
        perror("shmat error");
        exit(1);
    }

    initializeConveyor(conveyor);

    int truck_queue_id = shmget(IPC_PRIVATE, sizeof(TruckQueue), IPC_CREAT | 0600);
    if (truck_queue_id < 0) {
        perror("shmget error");
        exit(1);
    }

    truck_queue = (TruckQueue*)shmat(truck_queue_id, NULL, 0);
    if (truck_queue == (void*)-1) {
        perror("shmat error");
        exit(1);
    }

    initializeTruckQueue(truck_queue);

    int shmTrucksId = shmget(IPC_PRIVATE, sizeof(Truck) * TRUCK_NUMBER, IPC_CREAT | 0600);
    if (shmTrucksId < 0) {
        perror("shmget error");
        exit(1);
    }

    Truck* sharedTrucks = (Truck*)shmat(shmTrucksId, NULL, 0);
    if (sharedTrucks == (void*)-1) {
       perror("shmat error");
      exit(1);
    }

    initializeTrucks(sharedTrucks,TRUCK_NUMBER);
    pid_t truck_pids[TRUCK_NUMBER];
    for (int i = 0; i < TRUCK_NUMBER; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork error (truck)");
            exit(1);
        } else if (pid == 0) {
            truckProcess(&sharedTrucks[i]);
            _exit(0);
        } else {
            truck_pids[i] = pid;
            sharedTrucks[i].pid = pid;
            addTruckToQueue(truck_queue, &sharedTrucks[i]);
        }
    }

    int brick_storage_id = shmget(IPC_PRIVATE, BRICK_STORAGE_SIZE, IPC_CREAT | 0600);
    if (brick_storage_id < 0) {
        perror("shmget error");
        exit(1);
    }

    brick_storage = shmat(brick_storage_id, NULL, 0);
    if (brick_storage == (void*)-1) {
       perror("shmat error");
      exit(1);
    }

    for (int i = 0; i < BRICK_STORAGE_SIZE; i++) {
        brick_storage[i] = '1';
    }

    pid_t dispatcher_pid = fork();
    if (dispatcher_pid == 0) {
        dispatcher(conveyor);
        _exit(0);
    }

    int worker_lower_limits[3] = {(5*BRICK_STORAGE_SIZE)/6, 0.5*BRICK_STORAGE_SIZE, 0};
    int worker_upper_limits[3] ={BRICK_STORAGE_SIZE, (5*BRICK_STORAGE_SIZE)/6, 0.5*BRICK_STORAGE_SIZE};

    pid_t worker_pids[3];
    for (int i = 0; i < 3; i++) {
        pid_t worker_pid = fork();
        if (worker_pid < 0) {
            perror("Błąd");
            exit(1);
        } else if (worker_pid == 0) {
            worker(i + 1, conveyor, worker_lower_limits[i], worker_upper_limits[i]);
            _exit(0);
        } else {
            worker_pids[i] = worker_pid;
        }
    }

    while (continue_production) {
        conveyorCheckAndUnloadBricks(conveyor);
        usleep(SLEEP_TIME/10);
    }

    for (int i = 0; i < 3; i++) {
        kill(worker_pids[i], SIGTERM); 
        waitpid(worker_pids[i], NULL, 0);
    }
    
    kill(dispatcher_pid, SIGTERM);
    waitpid(dispatcher_pid, NULL, 0);

    for (int i = 0; i < TRUCK_NUMBER; i++) {
        kill(truck_pids[i], SIGTERM);
        waitpid(truck_pids[i], NULL, 0);
    }
    
    for (int i = 0; i < TRUCK_NUMBER; i++) {
    pthread_mutex_destroy(&sharedTrucks[i].mutex);
    }
    pthread_mutex_destroy(&truck_queue->mutex);
    pthread_cond_destroy(&truck_queue->cond);

    printf("\nZakończenie programu... Zwolnienie zasobów.\n");

    if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1) {
    perror("msgctl(IPC_RMID) error");
    exit(EXIT_FAILURE);
    }

    shmdt(conveyor);
    shmctl(shmId, IPC_RMID, NULL);

    shmdt(brick_storage);
    shmctl(brick_storage_id, IPC_RMID, NULL);

    shmdt(truck_queue);
    shmctl(truck_queue_id, IPC_RMID, NULL);

    shmdt(sharedTrucks);
    shmctl(shmTrucksId, IPC_RMID, NULL);

    return 0;
}
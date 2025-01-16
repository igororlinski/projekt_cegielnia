#include "brickyard.h"
volatile sig_atomic_t continue_production = 1;
WorkerQueue worker_queue;
Truck sharedTrucks[TRUCK_NUMBER];
TruckQueue* truck_queue;
ConveyorBelt* conveyor;

void signalHandler(int sig) {
    (void)sig;
    continue_production = 0;
}

int main() {
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
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
            addTruckToQueue(truck_queue, &sharedTrucks[i]);
        }
    }

    initializeWorkerQueue(&worker_queue);
    pthread_t queue_checking_thread;
    pthread_create(&queue_checking_thread, NULL, chceckWorkerQueue, (void*)&conveyor);

    pid_t dispatcher_pid = fork();
    if (dispatcher_pid == 0) {
        dispatcher(conveyor);
        _exit(0);
    }

    const int worker_pickup_times[3] = {WORKER_PICKUP_TIME_W1*SLEEP_TIME, WORKER_PICKUP_TIME_W2*SLEEP_TIME, WORKER_PICKUP_TIME_W3*SLEEP_TIME};

    pid_t worker_pids[3];
    for (int i = 0; i < 3; i++) {
        pid_t worker_pid = fork();
        if (worker_pid < 0) {
            perror("Błąd");
            exit(1);
        } else if (worker_pid == 0) {
            worker(i + 1, conveyor, worker_pickup_times);
            _exit(0);
        } else {
            worker_pids[i] = worker_pid;
        }
    }

    while (continue_production) {
        conveyorCheckAndUnloadBricks(conveyor);
        usleep(10*SLEEP_TIME);
    }

    for (int i = 0; i < 3; i++) {
        kill(worker_pids[i], SIGTERM); 
        waitpid(worker_pids[i], NULL, 0);
    }
    
    //kill(dispatcher_pid, SIGTERM);
    //waitpid(dispatcher_pid, NULL, 0);

    for (int i = 0; i < TRUCK_NUMBER; i++) {
        kill(truck_pids[i], SIGTERM);
        waitpid(truck_pids[i], NULL, 0);
    }

    pthread_cancel(queue_checking_thread);
    pthread_join(queue_checking_thread, NULL);
    
    for (int i = 0; i < TRUCK_NUMBER; i++) {
    pthread_mutex_destroy(&sharedTrucks[i].mutex);
    }
    pthread_mutex_destroy(&truck_queue->mutex);
    pthread_cond_destroy(&truck_queue->cond);

    printf("\nZakończenie programu... Zwolnienie zasobów.\n");

    shmdt(conveyor);
    shmctl(shmId, IPC_RMID, NULL);

    shmdt(truck_queue);
    shmctl(truck_queue_id, IPC_RMID, NULL);

    shmdt(sharedTrucks);
    shmctl(shmTrucksId, IPC_RMID, NULL);

    return 0;
}
#include "brickyard.h"

volatile sig_atomic_t continue_production = 1;

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

    ConveyorBelt* conveyor = (ConveyorBelt*)shmat(shmId, NULL, 0);
    if (conveyor == (void*)-1) {
        perror("shmat error");
        exit(1);
    }

    initializeConveyor(conveyor);

    const int worker_pickup_times[3] = {WORKER_PICKUP_TIME_W1, WORKER_PICKUP_TIME_W2, WORKER_PICKUP_TIME_W3};

    pid_t worker_pids[3];
    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork error");
            exit(1);
        } else if (pid == 0) {
            worker(i + 1, conveyor, worker_pickup_times);
            exit(0);
        } else {
            worker_pids[i] = pid;
        }
    }

    while (continue_production) {
        checkAndUnloadBricks(conveyor);
        usleep(100000);
    }

    for (int i = 0; i < 3; i++) {
        kill(worker_pids[i], SIGTERM); 
    }
    
    printf("\nZakończenie programu... Zwolnienie zasobów.\n");

    shmdt(conveyor);
    shmctl(shmId, IPC_RMID, NULL);

    return 0;
}
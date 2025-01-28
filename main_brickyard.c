#include "brickyard.h"
volatile sig_atomic_t continue_production = 1;
TruckQueue* truck_queue;
ConveyorBelt* conveyor;
Truck* sharedTrucks;
char *brick_storage;
int msg_queue_id;
key_t msg_key, conveyor_key, truck_queue_key, storage_key, shared_trucks_key, key_add, key_remove, key_capacity, key_weight;
int semid_add_brick, semid_remove_brick, semid_conveyor_capacity, semid_weight_capacity;

void initializeTrucks(Truck* sharedTrucks, int numTrucks);
void initializeTruckQueue(TruckQueue* truck_queue);
void initializeConveyor(ConveyorBelt* q);
void addTruckToQueue(TruckQueue* q, Truck* truck, void* sharedTrucks);
void cleanup(pid_t worker_pids[], pid_t truck_pids[], pid_t dispatcher_pid, pid_t conveyor_pid, int conveyor_shm_id, int brick_storage_id, int truck_queue_shm_id, int shmTrucksId);

void signalHandler(int sig) {
    switch(sig) {
        case SIGINT:
            continue_production = 0;
            break;
        case SIGUSR2:
            continue_production = 0;
            break;
    }
}

int main() {
    msg_key = ftok(".", 'Q');
    conveyor_key = ftok(".", 'Y');
    storage_key = ftok(".", 'S');
    truck_queue_key = ftok (".", 'Z');
    shared_trucks_key = ftok(".", 'U');
    
    msg_queue_id = msgget(msg_key, IPC_CREAT | 0600);
    if (msg_queue_id < 0) {
        perror("Nie udało się utworzyć kolejki komunikatów");
        exit(1);
    }

    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        perror("Nie udało się zarejestrować obsługi sygnału");
        exit(1);
    }

    if (signal(SIGUSR2, signalHandler) == SIG_ERR) {
    perror("Nie udało się zarejestrować obsługi sygnału");
    exit(1);
    }

    int conveyor_shm_id = shmget(conveyor_key, sizeof(ConveyorBelt), IPC_CREAT | 0600);
    if (conveyor_shm_id < 0) {
        perror("shmget error");
        exit(1);
    }

    conveyor = (ConveyorBelt*)shmat(conveyor_shm_id, NULL, 0);
    if (conveyor == (void*)-1) {
        perror("shmat error");
        exit(1);
    }

    initializeConveyor(conveyor);

    int brick_storage_id = shmget(storage_key, BRICK_STORAGE_SIZE, IPC_CREAT | 0600);
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

    int truck_queue_shm_id = shmget(truck_queue_key, sizeof(TruckQueue), IPC_CREAT | 0600);
    if (truck_queue_shm_id < 0) {
        perror("shmget error");
        exit(1);
    }

    truck_queue = (TruckQueue*)shmat(truck_queue_shm_id, NULL, 0);
    if (truck_queue == (void*)-1) {
        perror("shmat error");
        exit(1);
    }

    initializeTruckQueue(truck_queue);

    int shmTrucksId = shmget(shared_trucks_key, sizeof(Truck) * (TRUCK_NUMBER + 1), IPC_CREAT | 0600);
    if (shmTrucksId < 0) {
        perror("shmget error");
        exit(1);
    }

    sharedTrucks = (Truck*)shmat(shmTrucksId, NULL, 0);
    if (sharedTrucks == (void*)-1) {
       perror("shmat error");
      exit(1);
    }

    initializeTrucks(sharedTrucks, TRUCK_NUMBER);
    pid_t truck_pids[TRUCK_NUMBER];
    for (int i = 0; i < TRUCK_NUMBER; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork error (truck)");
            exit(1);
        } else if (pid == 0) {
            char truck_id_str[10];
            snprintf(truck_id_str, sizeof(truck_id_str), "%d", i);
            execl("./truck", "truck", truck_id_str, (char *)NULL); 
            perror("execl error (truck)");
            _exit(0);
        } else {
            truck_pids[i] = pid;
            sharedTrucks[i].pid = pid;
            addTruckToQueue(truck_queue, &sharedTrucks[i], (void*)sharedTrucks);
        }
    }

    pid_t dispatcher_pid = fork();
    if (dispatcher_pid < 0) {
        perror("fork failed (dispatcher)");
        return 1;
    } else if (dispatcher_pid == 0) {
        execl("./dispatcher", "dispatcher", (char *)NULL); 
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
            char worker_id_str[10];
            char lower_limit_str[10];
            char upper_limit_str[10];

            snprintf(worker_id_str, sizeof(worker_id_str), "%d", i + 1);
            snprintf(lower_limit_str, sizeof(lower_limit_str), "%d", worker_lower_limits[i]);
            snprintf(upper_limit_str, sizeof(upper_limit_str), "%d", worker_upper_limits[i]);

            execl("./worker", "worker", worker_id_str, lower_limit_str, upper_limit_str, (char *)NULL);
            perror("execl error (worker)");
            _exit(1);
        } else {
            worker_pids[i] = worker_pid;
        }
    }

    pid_t conveyor_pid = fork();
    if (conveyor_pid < 0) {
        perror("fork error (conveyor)");
        exit(1);
    } else if (conveyor_pid == 0) {
        execl("./conveyor", "conveyor", (char *)NULL);
        perror("execl error (conveyor)");
        _exit(1);
    }

    while (continue_production) {
        usleep(SLEEP_TIME);
    }

    cleanup(worker_pids, truck_pids, dispatcher_pid, conveyor_pid, conveyor_shm_id, brick_storage_id, truck_queue_shm_id, shmTrucksId);

    return 0;
}

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
        sharedTrucks[i].next = 0;
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

    truck_queue->front = 0;
    truck_queue->rear = 0;

    pthread_mutexattr_destroy(&attr);
    pthread_condattr_destroy(&cond_attr);
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

void initializeConveyor(ConveyorBelt* q) {
    q->front = 0;
    q->rear = -1;
    q->last_brick_id = 0; 

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&q->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
    key_add = ftok(".", 'A');
    key_remove = ftok(".", 'B');
    key_capacity = ftok(".", 'C');
    key_weight = ftok(".", 'D'); 

    semid_add_brick = semget(key_add, 1, IPC_CREAT | 0600);
    semid_remove_brick = semget(key_remove, 1, IPC_CREAT | 0600);
    semid_conveyor_capacity = semget(key_capacity, 1, IPC_CREAT | 0600); 
    semid_weight_capacity = semget(key_weight, 1, IPC_CREAT | 0600); 

    semctl(semid_add_brick, 0, SETVAL, 1);
    semctl(semid_remove_brick, 0, SETVAL, 1);
    semctl(semid_conveyor_capacity, 0, SETVAL, MAX_CONVEYOR_BRICKS_NUMBER);
    semctl(semid_weight_capacity, 0, SETVAL, MAX_CONVEYOR_BRICKS_WEIGHT);
}

void cleanup(pid_t worker_pids[], pid_t truck_pids[], pid_t dispatcher_pid, pid_t conveyor_pid, int conveyor_shm_id, int brick_storage_id, int truck_queue_shm_id, int shmTrucksId) {
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

    kill(conveyor_pid, SIGTERM);
    waitpid(conveyor_pid, NULL, 0);

    printf("\n\033[38;5;160mCegielnia kończy pracę... Zwolnienie zasobów.\033[0m\n");

    if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1) {
    perror("msgctl(IPC_RMID) error");
    exit(EXIT_FAILURE);
    }

    if (semctl(semid_add_brick, 0, IPC_RMID) == -1) {
        perror("Nie udało się usunąć semafora semid_add_brick");
    }
    if (semctl(semid_remove_brick, 0, IPC_RMID) == -1) {
        perror("Nie udało się usunąć semafora semid_remove_brick");
    }
    if (semctl(semid_conveyor_capacity, 0, IPC_RMID) == -1) {
        perror("Nie udało się usunąć semafora semid_conveyor_capacity");
    }
    if (semctl(semid_weight_capacity, 0, IPC_RMID) == -1) {
        perror("Nie udało się usunąć semafora semid_weight_capacity");
    }

    shmdt(conveyor);
    shmctl(conveyor_shm_id, IPC_RMID, NULL);

    shmdt(brick_storage);
    shmctl(brick_storage_id, IPC_RMID, NULL);

    shmdt(truck_queue);
    shmctl(truck_queue_shm_id, IPC_RMID, NULL);

    shmdt(sharedTrucks);
    shmctl(shmTrucksId, IPC_RMID, NULL);
}
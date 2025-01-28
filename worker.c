#include "brickyard.h"
ConveyorBelt* conveyor;
int msg_queue_id;
int semid_add_brick, semid_conveyor_capacity, semid_weight_capacity;
char *brick_storage;

void attach_to_memory(ConveyorBelt** conveyor, int *msg_queue_id,  char **brick_storage, int *semid_conveyor_capacity, int *semid_weight_capacity, int* semid_add_brick);
int createBrick(Brick* brick, int workerId, char* storage, int lower_limit);
int tryAddingBrick(int workerId, ConveyorBelt* conveyor, char* storage, int lower_limit);
void addBrick(ConveyorBelt* q, int workerId, Brick* brick);

int main(int argc __attribute__((unused)), char *argv[]) {
    int random_time;
    int workerId = atoi(argv[1]);
    int lower_limit = atoi(argv[2]);
    int upper_limit = atoi(argv[3]);
    attach_to_memory(&conveyor, &msg_queue_id, &brick_storage, &semid_conveyor_capacity, &semid_weight_capacity, &semid_add_brick);

    while (1) {
        srand(time(NULL) + getpid());
        random_time = WORKER_MIN_TIME + rand() % (SLEEP_TIME - WORKER_MIN_TIME + 1);
        //printf("Czas %d - %d\n", workerId, random_time);
        usleep(random_time);
        if (upper_limit - lower_limit < workerId) {
            printf("Pracownik %d załadował wszystkie dostępne cegły i kończy pracę.\n", workerId);

            struct MsgBuffer msg;
            msg.mtype = 1;
            msg.workerId = workerId;

            if (msgsnd(msg_queue_id, &msg, sizeof(int), 0) == -1) {
                perror("Nie udało się wysłać wiadomości");
            }
            
            break;
        }
        lower_limit = tryAddingBrick(workerId, conveyor, brick_storage, lower_limit);
    }
    return 1;
}

int createBrick(Brick* brick, int workerId, char* storage, int lower_limit) {
    for (int i = 0; i < workerId; i++) {
        brick->weight[i] = storage[lower_limit];
        storage[lower_limit] = '\0';
        lower_limit++;
    }
    return lower_limit;
}

int tryAddingBrick(int workerId, ConveyorBelt* conveyor, char* storage, int lower_limit) {
    Brick newBrick;
    
    lower_limit = createBrick(&newBrick, workerId, storage, lower_limit);
    int brick_weight = getBrickWeight(&newBrick);
    struct sembuf op_weight = {0, -brick_weight, 0};

    if(semctl(semid_conveyor_capacity, 0, GETVAL) == 0) {
        printf("Pracownik P%d czeka na dodanie cegły o wadze %d na taśmę z powodu zapełnienia taśmy.\n", workerId, brick_weight);
    }
    else if(semctl(semid_weight_capacity, 0, GETVAL) < brick_weight) {
        printf("Pracownik P%d czeka na dodanie cegły o wadze %d na taśmę z powodu przeciążenia taśmy.\n", workerId, brick_weight);
    }
    
    semop(semid_add_brick, &p, 1);
    semop(semid_weight_capacity, &op_weight, 1);
    semop(semid_conveyor_capacity, &p, 1);
    addBrick(conveyor, workerId, &newBrick);    

    return lower_limit;
}

void attach_to_memory(ConveyorBelt** conveyor, int *msg_queue_id, char **brick_storage, int *semid_conveyor_capacity, int *semid_weight_capacity, int* semid_add_brick) {
    key_t msg_key = ftok(".", 'Q');
    key_t conveyor_key = ftok(".", 'Y');
    key_t key_capacity = ftok(".", 'C');
    key_t key_add = ftok(".", 'A');
    key_t key_weight = ftok(".", 'D');
    key_t storage_key = ftok(".", 'S');

    int conveyor_shm_id = shmget(conveyor_key, sizeof(ConveyorBelt), 0600);
    if (conveyor_shm_id < 0) {
        perror("shmget error");
        exit(1);
    }

    *conveyor = (ConveyorBelt*)shmat(conveyor_shm_id, NULL, 0);
    if (conveyor == (void*)-1) {
        perror("shmat error");
        exit(1);
    }

    if (msg_key == -1) {
        perror("ftok failed");
        exit(1);
    }

    *msg_queue_id = msgget(msg_key, 0600);
    if (*msg_queue_id < 0) {
        perror("Nie udało się połączyć z kolejką komunikatów");
        exit(1);
    }

    int brick_storage_id = shmget(storage_key, BRICK_STORAGE_SIZE, 0600);
    if (brick_storage_id < 0) {
        perror("shmget error");
        exit(1);
    }

    *brick_storage = shmat(brick_storage_id, NULL, 0);
    if (brick_storage == (void*)-1) {
       perror("shmat error");
      exit(1);
    }

    *semid_conveyor_capacity = semget(key_capacity, 1, 0600); 
    *semid_weight_capacity = semget(key_weight, 1, 0600);
    *semid_add_brick = semget(key_add, 1, 0600);
}

void addBrick(ConveyorBelt* q, int workerId, Brick* brick) {
    q->rear = (q->rear + 1) % MAX_CONVEYOR_BRICKS_NUMBER;
    q->last_brick_id++;
    brick->id = q->last_brick_id;
    brick->ad = clock();

    q->bricks[q->rear] = *brick;
    printf("Pracownik P%d dodał cegłę o wadze %d na taśmę. ID cegły: %d        Liczba cegieł na taśmie: %d      Łączna waga cegieł na taśmie: %d\n", workerId, getBrickWeight(brick), brick->id, MAX_CONVEYOR_BRICKS_NUMBER - semctl(semid_conveyor_capacity, 0, GETVAL), MAX_CONVEYOR_BRICKS_WEIGHT - semctl(semid_weight_capacity, 0, GETVAL));

    semop(semid_add_brick, &v, 1);
}
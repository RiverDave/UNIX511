#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define KEY 1234 // Key for semaphore set, choose any integer

// Semaphore union for semctl system call
union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

// Function to perform semaphore operations
void performSemaphoreOperation(int sem_id, int sem_num, int sem_op) {
    struct sembuf semaphore;
    semaphore.sem_num = sem_num;
    semaphore.sem_op = sem_op;
    semaphore.sem_flg = 0; // No special flags

    if (semop(sem_id, &semaphore, 1) == -1) {
        perror("Semaphore operation failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int sem_id, num_processes = 5;

    // Create a semaphore set with a single semaphore
    if ((sem_id = semget(KEY, 1, IPC_CREAT | 0666)) == -1) {
        perror("Semaphore creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the semaphore value to 1 (binary semaphore)
    union semun arg;
    arg.val = 1;
    if (semctl(sem_id, 0, SETVAL, arg) == -1) {
        perror("Semaphore initialization failed");
        exit(EXIT_FAILURE);
    }

    // Fork multiple processes to demonstrate semaphore usage
    for (int i = 0; i < num_processes; ++i) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            printf("Child %d attempting to enter critical section...\n", i);

            // Wait (decrement) on the semaphore
            performSemaphoreOperation(sem_id, 0, -1);

            // Critical section: simulate some work
            printf("Child %d in critical section\n", i);
            sleep(2);

            // Signal (increment) the semaphore to release the critical section
            performSemaphoreOperation(sem_id, 0, 1);

            exit(EXIT_SUCCESS);
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < num_processes; ++i) {
        wait(NULL);
    }

    // Remove the semaphore set
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("Semaphore removal failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}

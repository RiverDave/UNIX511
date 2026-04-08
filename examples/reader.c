#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 1234
#define SHM_SIZE 1024  // Size of shared memory in bytes

int main() {
    // Get the shared memory segment
    int shm_id = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach the shared memory segment
    char *shm_ptr = (char *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (char *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Read data from shared memory
    printf("Data read from shared memory: %s\n", shm_ptr);

    // Detach the shared memory segment
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    // Optionally remove the shared memory segment (uncomment to remove)
    // if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
    //     perror("shmctl");
    //     exit(EXIT_FAILURE);
    // }

    return 0;
}

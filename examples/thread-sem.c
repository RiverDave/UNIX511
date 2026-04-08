#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define NUM_THREADS 5

int sharedCounter = 0;
sem_t semaphore;

void* incrementCounter(void* threadId) {
    int id = *((int*)threadId);

    for (int i = 0; i < 3; ++i) {
        // Wait on the semaphore
        sem_wait(&semaphore);

        // Critical section: increment shared counter
        sharedCounter++;
        printf("Thread %d incremented the counter to %d\n", id, sharedCounter);

        // Signal the semaphore
        sem_post(&semaphore);

        // Simulate some work outside the critical section
        sleep(1);
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    int threadIds[NUM_THREADS];

    // Initialize semaphore
    sem_init(&semaphore, 0, 1); // 1 is the initial value of the semaphore

    // Create threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        threadIds[i] = i;
        if (pthread_create(&threads[i], NULL, incrementCounter, (void*)&threadIds[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Destroy the semaphore
    sem_destroy(&semaphore);

    printf("Final counter value: %d\n", sharedCounter);

    return 0;
}

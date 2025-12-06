/*
 * Race Condition FIXED Version
 *
 * This program demonstrates the SOLUTION to race conditions using semaphores.
 * Multiple processes increment a shared counter WITH proper synchronization,
 * ensuring CORRECT results.
 *
 * Result: counter = NUM_PROCESSES * INCREMENTS_PER_PROCESS (Always correct!)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#define SHM_NAME "/race_fixed"
#define SEM_NAME "/race_sem"
#define NUM_PROCESSES 5
#define INCREMENTS_PER_PROCESS 10000

typedef struct {
    int counter;
    int expected_value;
} shared_data_t;

void child_process(shared_data_t* data, sem_t* mutex, int process_id) {
    printf("Process %d: Starting increments...\n", process_id);

    for (int i = 0; i < INCREMENTS_PER_PROCESS; i++) {
        // ENTER CRITICAL SECTION
        sem_wait(mutex);  // Lock

        // Now safe to read-modify-write
        int temp = data->counter;
        temp++;
        data->counter = temp;

        // LEAVE CRITICAL SECTION
        sem_post(mutex);  // Unlock

        if (i % 1000 == 0) {
            usleep(1);
        }
    }

    printf("Process %d: Finished\n", process_id);
}

int main() {
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║     RACE CONDITION FIXED (With Synchronization)       ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");

    // Create shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate");
        return 1;
    }

    shared_data_t* data = mmap(NULL, sizeof(shared_data_t),
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Create semaphore for synchronization
    sem_unlink(SEM_NAME);
    sem_t* mutex = sem_open(SEM_NAME, O_CREAT, 0644, 1);  // Binary semaphore
    if (mutex == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    // Initialize
    data->counter = 0;
    data->expected_value = NUM_PROCESSES * INCREMENTS_PER_PROCESS;

    printf("Configuration:\n");
    printf("  - Number of processes: %d\n", NUM_PROCESSES);
    printf("  - Increments per process: %d\n", INCREMENTS_PER_PROCESS);
    printf("  - Expected final value: %d\n", data->expected_value);
    printf("  - Synchronization: Semaphore (mutex)\n\n");

    printf("Starting %d processes WITH synchronization...\n\n", NUM_PROCESSES);

    // Fork multiple processes
    pid_t pids[NUM_PROCESSES];
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            // Child process
            child_process(data, mutex, i + 1);
            sem_close(mutex);
            exit(0);
        } else if (pids[i] < 0) {
            perror("fork");
            return 1;
        }
    }

    // Wait for all children
    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(pids[i], NULL, 0);
    }

    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                   RESULTS                              ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("Expected value: %d\n", data->expected_value);
    printf("Actual value:   %d\n", data->counter);
    printf("Difference:     %d\n", data->expected_value - data->counter);
    printf("Accuracy:       %.2f%%\n",
           (data->counter * 100.0) / data->expected_value);

    if (data->counter == data->expected_value) {
        printf("\n✅ SUCCESS! No race conditions.\n");
        printf("   Semaphore synchronization prevented all lost updates.\n");
    } else {
        printf("\n❌ UNEXPECTED: Still has errors!\n");
        printf("   This should not happen with proper synchronization.\n");
    }

    // Cleanup
    sem_close(mutex);
    sem_unlink(SEM_NAME);
    munmap(data, sizeof(shared_data_t));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}

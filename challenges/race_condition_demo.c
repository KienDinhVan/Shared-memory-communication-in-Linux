/*
 * Race Condition Demo - BUGGY VERSION
 *
 * This program demonstrates a RACE CONDITION problem in shared memory.
 * Multiple processes increment a shared counter WITHOUT synchronization,
 * leading to INCORRECT results due to race conditions.
 *
 * Expected Result (correct): counter = NUM_PROCESSES * INCREMENTS_PER_PROCESS
 * Actual Result (buggy):     counter < expected (due to lost updates)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#define SHM_NAME "/race_demo"
#define NUM_PROCESSES 5
#define INCREMENTS_PER_PROCESS 10000

typedef struct {
    int counter;
    int expected_value;
} shared_data_t;

void child_process(shared_data_t* data, int process_id) {
    printf("Process %d: Starting increments...\n", process_id);

    for (int i = 0; i < INCREMENTS_PER_PROCESS; i++) {
        // RACE CONDITION HERE!
        // Multiple processes read-modify-write without synchronization
        int temp = data->counter;  // Read
        temp++;                     // Modify
        data->counter = temp;       // Write

        // This tiny delay makes race conditions MORE visible
        if (i % 1000 == 0) {
            usleep(1);
        }
    }

    printf("Process %d: Finished\n", process_id);
}

int main() {
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║        RACE CONDITION DEMONSTRATION (BUGGY)           ║\n");
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

    // Initialize
    data->counter = 0;
    data->expected_value = NUM_PROCESSES * INCREMENTS_PER_PROCESS;

    printf("Configuration:\n");
    printf("  - Number of processes: %d\n", NUM_PROCESSES);
    printf("  - Increments per process: %d\n", INCREMENTS_PER_PROCESS);
    printf("  - Expected final value: %d\n\n", data->expected_value);

    printf("Starting %d processes WITHOUT synchronization...\n\n", NUM_PROCESSES);

    // Fork multiple processes
    pid_t pids[NUM_PROCESSES];
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            // Child process
            child_process(data, i + 1);
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
    printf("Lost updates:   %d\n", data->expected_value - data->counter);
    printf("Accuracy:       %.2f%%\n",
           (data->counter * 100.0) / data->expected_value);

    if (data->counter < data->expected_value) {
        printf("\n❌ RACE CONDITION DETECTED!\n");
        printf("   Multiple processes overwrote each other's updates.\n");
        printf("   Solution: Use synchronization (see race_condition_fixed.c)\n");
    } else {
        printf("\n⚠️  Race condition might not be visible with current timing.\n");
        printf("   Try running multiple times or increase INCREMENTS_PER_PROCESS.\n");
    }

    // Cleanup
    munmap(data, sizeof(shared_data_t));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}

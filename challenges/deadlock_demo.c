/*
 * Deadlock Demonstration and Prevention
 *
 * This program demonstrates:
 * 1. How deadlock can occur with multiple semaphores
 * 2. How to prevent deadlock with lock ordering
 *
 * Run with argument:
 *   - "buggy" to see deadlock scenario (will hang!)
 *   - "fixed" to see deadlock prevention
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>

#define SHM_NAME "/deadlock_demo"
#define SEM_A "/sem_resource_a"
#define SEM_B "/sem_resource_b"

typedef struct {
    int resource_a;
    int resource_b;
    int transfers_completed;
} shared_data_t;

sem_t *sem_a, *sem_b;
shared_data_t* data;

void cleanup_handler(int sig) {
    printf("\n\n⚠️  Process interrupted! Cleaning up...\n");
    sem_close(sem_a);
    sem_close(sem_b);
    sem_unlink(SEM_A);
    sem_unlink(SEM_B);
    munmap(data, sizeof(shared_data_t));
    shm_unlink(SHM_NAME);
    exit(1);
}

// BUGGY VERSION: Can cause deadlock
void transfer_a_to_b_buggy(int process_id) {
    for (int i = 0; i < 5; i++) {
        printf("Process %d: Trying to lock A...\n", process_id);
        sem_wait(sem_a);  // Lock A first
        printf("Process %d: Locked A, trying to lock B...\n", process_id);

        sleep(1);  // Simulate work and increase chance of deadlock

        sem_wait(sem_b);  // Then lock B
        printf("Process %d: Locked both A and B\n", process_id);

        // Transfer
        data->resource_a -= 10;
        data->resource_b += 10;
        data->transfers_completed++;
        printf("Process %d: Transfer complete (%d)\n", process_id, i + 1);

        sem_post(sem_b);
        sem_post(sem_a);
        sleep(1);
    }
}

void transfer_b_to_a_buggy(int process_id) {
    for (int i = 0; i < 5; i++) {
        printf("Process %d: Trying to lock B...\n", process_id);
        sem_wait(sem_b);  // Lock B first
        printf("Process %d: Locked B, trying to lock A...\n", process_id);

        sleep(1);  // Simulate work

        sem_wait(sem_a);  // Then lock A - DEADLOCK!
        printf("Process %d: Locked both B and A\n", process_id);

        // Transfer
        data->resource_b -= 10;
        data->resource_a += 10;
        data->transfers_completed++;
        printf("Process %d: Transfer complete (%d)\n", process_id, i + 1);

        sem_post(sem_a);
        sem_post(sem_b);
        sleep(1);
    }
}

// FIXED VERSION: Always lock in same order
void transfer_a_to_b_fixed(int process_id) {
    for (int i = 0; i < 5; i++) {
        printf("Process %d: Trying to lock A then B (ordered)...\n", process_id);
        sem_wait(sem_a);  // Always lock A first
        sem_wait(sem_b);  // Then B
        printf("Process %d: Locked both (ordered)\n", process_id);

        data->resource_a -= 10;
        data->resource_b += 10;
        data->transfers_completed++;
        printf("Process %d: Transfer A→B complete (%d)\n", process_id, i + 1);

        sem_post(sem_b);  // Release in reverse order
        sem_post(sem_a);
        usleep(100000);
    }
}

void transfer_b_to_a_fixed(int process_id) {
    for (int i = 0; i < 5; i++) {
        printf("Process %d: Trying to lock A then B (ordered)...\n", process_id);
        sem_wait(sem_a);  // Always lock A first (same order!)
        sem_wait(sem_b);  // Then B
        printf("Process %d: Locked both (ordered)\n", process_id);

        data->resource_b -= 10;
        data->resource_a += 10;
        data->transfers_completed++;
        printf("Process %d: Transfer B→A complete (%d)\n", process_id, i + 1);

        sem_post(sem_b);
        sem_post(sem_a);
        usleep(100000);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [buggy|fixed]\n", argv[0]);
        return 1;
    }

    int use_buggy = (strcmp(argv[1], "buggy") == 0);

    signal(SIGINT, cleanup_handler);

    printf("╔════════════════════════════════════════════════════════╗\n");
    if (use_buggy) {
        printf("║           DEADLOCK DEMONSTRATION (BUGGY)               ║\n");
        printf("║  ⚠️  WARNING: This will likely DEADLOCK!              ║\n");
        printf("║     Press Ctrl+C to terminate if it hangs             ║\n");
    } else {
        printf("║         DEADLOCK PREVENTION (FIXED)                    ║\n");
        printf("║           Using Lock Ordering                          ║\n");
    }
    printf("╚════════════════════════════════════════════════════════╝\n\n");

    // Create shared memory
    shm_unlink(SHM_NAME);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shared_data_t));
    data = mmap(NULL, sizeof(shared_data_t),
                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    data->resource_a = 100;
    data->resource_b = 100;
    data->transfers_completed = 0;

    // Create semaphores
    sem_unlink(SEM_A);
    sem_unlink(SEM_B);
    sem_a = sem_open(SEM_A, O_CREAT, 0644, 1);
    sem_b = sem_open(SEM_B, O_CREAT, 0644, 1);

    printf("Initial state: A=%d, B=%d\n\n", data->resource_a, data->resource_b);

    // Fork two processes with opposite locking orders
    pid_t pid1 = fork();
    if (pid1 == 0) {
        if (use_buggy)
            transfer_a_to_b_buggy(1);
        else
            transfer_a_to_b_fixed(1);
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        if (use_buggy)
            transfer_b_to_a_buggy(2);
        else
            transfer_b_to_a_fixed(2);
        exit(0);
    }

    // Wait for both (will timeout if deadlocked)
    int status1, status2;
    pid_t result1 = waitpid(pid1, &status1, 0);
    pid_t result2 = waitpid(pid2, &status2, 0);

    if (result1 > 0 && result2 > 0) {
        printf("\n╔════════════════════════════════════════════════════════╗\n");
        printf("║                   RESULTS                              ║\n");
        printf("╚════════════════════════════════════════════════════════╝\n");
        printf("Final state: A=%d, B=%d\n", data->resource_a, data->resource_b);
        printf("Transfers completed: %d\n", data->transfers_completed);

        if (use_buggy) {
            printf("\n⚠️  If you see this, deadlock didn't occur this time.\n");
            printf("   Try running again - deadlock is probabilistic.\n");
        } else {
            printf("\n✅ SUCCESS! No deadlock occurred.\n");
            printf("   Lock ordering prevented deadlock.\n");
        }
    }

    // Cleanup
    sem_close(sem_a);
    sem_close(sem_b);
    sem_unlink(SEM_A);
    sem_unlink(SEM_B);
    munmap(data, sizeof(shared_data_t));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}

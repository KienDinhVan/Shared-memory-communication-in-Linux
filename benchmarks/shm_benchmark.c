#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define TEST_SIZE (1024 * 1024) // 1 MB
#define NUM_ITERATIONS 1000

typedef struct {
    double posix_create_time;
    double posix_attach_time;
    double posix_write_time;
    double posix_read_time;
    double posix_cleanup_time;
    
    double sysv_create_time;
    double sysv_attach_time;
    double sysv_write_time;
    double sysv_read_time;
    double sysv_cleanup_time;
    
    double throughput_posix_mb_s;
    double throughput_sysv_mb_s;
} benchmark_results_t;

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void benchmark_posix(benchmark_results_t* results) {
    double start, end;
    
    // Create
    start = get_time_ms();
    int shm_fd = shm_open("/benchmark_posix", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, TEST_SIZE);
    end = get_time_ms();
    results->posix_create_time = end - start;
    
    // Attach/Map
    start = get_time_ms();
    char* ptr = mmap(0, TEST_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    end = get_time_ms();
    results->posix_attach_time = end - start;
    
    // Write benchmark
    start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        memset(ptr, i % 256, TEST_SIZE);
    }
    end = get_time_ms();
    results->posix_write_time = end - start;
    results->throughput_posix_mb_s = (TEST_SIZE * NUM_ITERATIONS / 1024.0 / 1024.0) / 
                                     (results->posix_write_time / 1000.0);
    
    // Read benchmark
    start = get_time_ms();
    volatile char dummy;
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        for (int j = 0; j < TEST_SIZE; j += 4096) {
            dummy = ptr[j];
        }
    }
    end = get_time_ms();
    results->posix_read_time = end - start;
    
    // Cleanup
    start = get_time_ms();
    munmap(ptr, TEST_SIZE);
    close(shm_fd);
    shm_unlink("/benchmark_posix");
    end = get_time_ms();
    results->posix_cleanup_time = end - start;
}

void benchmark_sysv(benchmark_results_t* results) {
    double start, end;
    key_t key = ftok(".", 'B');
    
    // Create
    start = get_time_ms();
    int shmid = shmget(key, TEST_SIZE, 0666 | IPC_CREAT);
    end = get_time_ms();
    results->sysv_create_time = end - start;
    
    // Attach
    start = get_time_ms();
    char* ptr = (char*) shmat(shmid, NULL, 0);
    end = get_time_ms();
    results->sysv_attach_time = end - start;
    
    // Write benchmark
    start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        memset(ptr, i % 256, TEST_SIZE);
    }
    end = get_time_ms();
    results->sysv_write_time = end - start;
    results->throughput_sysv_mb_s = (TEST_SIZE * NUM_ITERATIONS / 1024.0 / 1024.0) / 
                                    (results->sysv_write_time / 1000.0);
    
    // Read benchmark
    start = get_time_ms();
    volatile char dummy;
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        for (int j = 0; j < TEST_SIZE; j += 4096) {
            dummy = ptr[j];
        }
    }
    end = get_time_ms();
    results->sysv_read_time = end - start;
    
    // Cleanup
    start = get_time_ms();
    shmdt(ptr);
    shmctl(shmid, IPC_RMID, NULL);
    end = get_time_ms();
    results->sysv_cleanup_time = end - start;
}

void print_results(benchmark_results_t* results) {
    printf("\n=================================\n");
    printf("   BENCHMARK RESULTS\n");
    printf("=================================\n");
    printf("Test Size: %d MB\n", TEST_SIZE / (1024*1024));
    printf("Iterations: %d\n\n", NUM_ITERATIONS);
    
    printf("%-20s %-12s %-12s\n", "Operation", "POSIX (ms)", "System V (ms)");
    printf("%-20s %-12s %-12s\n", "---------", "----------", "------------");
    printf("%-20s %-12.3f %-12.3f\n", "Create", results->posix_create_time, results->sysv_create_time);
    printf("%-20s %-12.3f %-12.3f\n", "Attach", results->posix_attach_time, results->sysv_attach_time);
    printf("%-20s %-12.3f %-12.3f\n", "Write", results->posix_write_time, results->sysv_write_time);
    printf("%-20s %-12.3f %-12.3f\n", "Read", results->posix_read_time, results->sysv_read_time);
    printf("%-20s %-12.3f %-12.3f\n", "Cleanup", results->posix_cleanup_time, results->sysv_cleanup_time);
    
    printf("\n%-20s %-12s %-12s\n", "Metric", "POSIX", "System V");
    printf("%-20s %-12s %-12s\n", "------", "-----", "--------");
    printf("%-20s %-12.2f %-12.2f\n", "Throughput (MB/s)", 
           results->throughput_posix_mb_s, results->throughput_sysv_mb_s);
    
    double total_posix = results->posix_create_time + results->posix_attach_time + 
                        results->posix_write_time + results->posix_read_time + 
                        results->posix_cleanup_time;
    double total_sysv = results->sysv_create_time + results->sysv_attach_time + 
                       results->sysv_write_time + results->sysv_read_time + 
                       results->sysv_cleanup_time;
    
    printf("%-20s %-12.3f %-12.3f\n", "Total Time (ms)", total_posix, total_sysv);
    
    printf("\n=================================\n");
    
    if (total_posix < total_sysv) {
        printf("Winner: POSIX (%.1f%% faster)\n", 
               ((total_sysv - total_posix) / total_sysv) * 100);
    } else {
        printf("Winner: System V (%.1f%% faster)\n", 
               ((total_posix - total_sysv) / total_posix) * 100);
    }
    printf("=================================\n");
}

int main() {
    printf("Starting Shared Memory Benchmark...\n");
    printf("This may take a few seconds...\n");
    
    benchmark_results_t results = {0};
    
    printf("\nBenchmarking POSIX Shared Memory...\n");
    benchmark_posix(&results);
    
    printf("Benchmarking System V Shared Memory...\n");
    benchmark_sysv(&results);
    
    print_results(&results);
    
    return 0;
}
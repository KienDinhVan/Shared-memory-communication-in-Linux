#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define SHM_NAME "/sync_shm"
#define SEM_MUTEX "/shm_mutex"
#define SEM_ITEMS "/shm_items"
#define SEM_SPACES "/shm_spaces"
#define BUFFER_SIZE 10

typedef struct {
    int buffer[BUFFER_SIZE];
    int in;   // Producer index
    int out;  // Consumer index
    int count; // Number of items
} shared_buffer_t;

// Producer
int producer(int num_items) {
    printf("Producer starting...\n");
    
    // Open shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    
    shared_buffer_t* buffer = mmap(0, sizeof(shared_buffer_t),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, shm_fd, 0);
    
    // Open semaphores
    sem_t* mutex = sem_open(SEM_MUTEX, 0);
    sem_t* items = sem_open(SEM_ITEMS, 0);
    sem_t* spaces = sem_open(SEM_SPACES, 0);
    
    // Produce items
    for (int i = 0; i < num_items; i++) {
        int item = i * 10;
        
        // Wait for empty space
        sem_wait(spaces);
        
        // Enter critical section
        sem_wait(mutex);
        
        // Add item to buffer
        buffer->buffer[buffer->in] = item;
        printf("Producer: Produced item %d at index %d\n", item, buffer->in);
        buffer->in = (buffer->in + 1) % BUFFER_SIZE;
        buffer->count++;
        
        // Leave critical section
        sem_post(mutex);
        
        // Signal that item is available
        sem_post(items);
        
        usleep(100000); // 100ms
    }
    
    printf("Producer: Finished producing %d items\n", num_items);
    
    // Cleanup
    munmap(buffer, sizeof(shared_buffer_t));
    close(shm_fd);
    sem_close(mutex);
    sem_close(items);
    sem_close(spaces);
    
    return 0;
}

// Consumer
int consumer(int num_items) {
    printf("Consumer starting...\n");
    
    // Open shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    
    shared_buffer_t* buffer = mmap(0, sizeof(shared_buffer_t),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, shm_fd, 0);
    
    // Open semaphores
    sem_t* mutex = sem_open(SEM_MUTEX, 0);
    sem_t* items = sem_open(SEM_ITEMS, 0);
    sem_t* spaces = sem_open(SEM_SPACES, 0);
    
    // Consume items
    for (int i = 0; i < num_items; i++) {
        // Wait for available item
        sem_wait(items);
        
        // Enter critical section
        sem_wait(mutex);
        
        // Remove item from buffer
        int item = buffer->buffer[buffer->out];
        printf("Consumer: Consumed item %d from index %d\n", item, buffer->out);
        buffer->out = (buffer->out + 1) % BUFFER_SIZE;
        buffer->count--;
        
        // Leave critical section
        sem_post(mutex);
        
        // Signal that space is available
        sem_post(spaces);
        
        usleep(150000); // 150ms (slower than producer)
    }
    
    printf("Consumer: Finished consuming %d items\n", num_items);
    
    // Cleanup
    munmap(buffer, sizeof(shared_buffer_t));
    close(shm_fd);
    sem_close(mutex);
    sem_close(items);
    sem_close(spaces);
    
    return 0;
}

// Initialize shared memory and semaphores
int initialize() {
    printf("Initializing shared memory and semaphores...\n");
    
    // Create shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    
    ftruncate(shm_fd, sizeof(shared_buffer_t));
    
    shared_buffer_t* buffer = mmap(0, sizeof(shared_buffer_t),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, shm_fd, 0);
    
    // Initialize buffer
    buffer->in = 0;
    buffer->out = 0;
    buffer->count = 0;
    memset(buffer->buffer, 0, sizeof(buffer->buffer));
    
    munmap(buffer, sizeof(shared_buffer_t));
    close(shm_fd);
    
    // Create semaphores
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_ITEMS);
    sem_unlink(SEM_SPACES);
    
    sem_t* mutex = sem_open(SEM_MUTEX, O_CREAT, 0644, 1); // Binary semaphore (mutex)
    sem_t* items = sem_open(SEM_ITEMS, O_CREAT, 0644, 0); // Initially 0 items
    sem_t* spaces = sem_open(SEM_SPACES, O_CREAT, 0644, BUFFER_SIZE); // All spaces free
    
    if (mutex == SEM_FAILED || items == SEM_FAILED || spaces == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
    
    sem_close(mutex);
    sem_close(items);
    sem_close(spaces);
    
    printf("Initialization complete!\n");
    return 0;
}

// Cleanup
int cleanup() {
    printf("Cleaning up...\n");
    
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_ITEMS);
    sem_unlink(SEM_SPACES);
    
    printf("Cleanup complete!\n");
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [init|producer|consumer|cleanup] [num_items]\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "init") == 0) {
        return initialize();
    } else if (strcmp(argv[1], "cleanup") == 0) {
        return cleanup();
    } else if (strcmp(argv[1], "producer") == 0) {
        int num_items = (argc > 2) ? atoi(argv[2]) : 20;
        return producer(num_items);
    } else if (strcmp(argv[1], "consumer") == 0) {
        int num_items = (argc > 2) ? atoi(argv[2]) : 20;
        return consumer(num_items);
    } else {
        fprintf(stderr, "Invalid command\n");
        return 1;
    }
}
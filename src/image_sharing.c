#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480
#define MAX_IMAGES 5
#define SHM_NAME "/image_shm"

typedef struct {
    unsigned char data[IMAGE_WIDTH * IMAGE_HEIGHT * 3]; // RGB
    int image_id;
    struct timespec timestamp;
    int processed;
} image_t;

typedef struct {
    image_t images[MAX_IMAGES];
    int write_idx;
    int read_idx;
    pthread_mutex_t mutex;
    pthread_cond_t image_ready;
    pthread_cond_t space_available;
    int active_images;
} image_buffer_t;

// Producer: Camera capture simulation
void* image_producer(void* arg) {
    image_buffer_t* buffer = (image_buffer_t*)arg;
    
    for (int i = 0; i < 100; i++) {
        pthread_mutex_lock(&buffer->mutex);
        
        // Wait for space
        while (buffer->active_images >= MAX_IMAGES) {
            pthread_cond_wait(&buffer->space_available, &buffer->mutex);
        }
        
        // "Capture" image (fill with pattern)
        image_t* img = &buffer->images[buffer->write_idx];
        img->image_id = i;
        clock_gettime(CLOCK_REALTIME, &img->timestamp);
        
        // Fill with test pattern
        for (int y = 0; y < IMAGE_HEIGHT; y++) {
            for (int x = 0; x < IMAGE_WIDTH; x++) {
                int idx = (y * IMAGE_WIDTH + x) * 3;
                img->data[idx] = (i + x) % 256;     // R
                img->data[idx+1] = (i + y) % 256;   // G
                img->data[idx+2] = (i) % 256;       // B
            }
        }
        img->processed = 0;
        
        printf("Producer: Captured image %d\n", i);
        
        buffer->write_idx = (buffer->write_idx + 1) % MAX_IMAGES;
        buffer->active_images++;
        
        pthread_cond_signal(&buffer->image_ready);
        pthread_mutex_unlock(&buffer->mutex);
        
        usleep(50000); // 50ms per frame = 20 FPS
    }
    
    return NULL;
}

// Consumer: Image processing
void* image_consumer(void* arg) {
    image_buffer_t* buffer = (image_buffer_t*)arg;
    
    while (1) {
        pthread_mutex_lock(&buffer->mutex);
        
        // Wait for image
        while (buffer->active_images == 0) {
            pthread_cond_wait(&buffer->image_ready, &buffer->mutex);
        }
        
        // Process image
        image_t* img = &buffer->images[buffer->read_idx];
        
        if (img->processed) {
            pthread_mutex_unlock(&buffer->mutex);
            break;
        }
        
        printf("Consumer: Processing image %d...\n", img->image_id);
        
        // Simulate processing (e.g., edge detection, filtering)
        long sum = 0;
        for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT * 3; i++) {
            sum += img->data[i];
        }
        
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        long latency_us = (now.tv_sec - img->timestamp.tv_sec) * 1000000 +
                         (now.tv_nsec - img->timestamp.tv_nsec) / 1000;
        
        printf("Consumer: Processed image %d, checksum=%ld, latency=%ld us\n",
               img->image_id, sum, latency_us);
        
        img->processed = 1;
        buffer->read_idx = (buffer->read_idx + 1) % MAX_IMAGES;
        buffer->active_images--;
        
        pthread_cond_signal(&buffer->space_available);
        pthread_mutex_unlock(&buffer->mutex);
        
        usleep(100000); // 100ms processing time
    }

    return NULL;
}

int main() {
    printf("=== Image Sharing with Shared Memory ===\n");
    printf("Simulating camera capture and processing...\n\n");

    // Create shared memory for image buffer
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Set size
    if (ftruncate(shm_fd, sizeof(image_buffer_t)) == -1) {
        perror("ftruncate");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    // Map to memory
    image_buffer_t* buffer = mmap(NULL, sizeof(image_buffer_t),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, shm_fd, 0);
    if (buffer == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    // Initialize buffer
    memset(buffer, 0, sizeof(image_buffer_t));
    buffer->write_idx = 0;
    buffer->read_idx = 0;
    buffer->active_images = 0;

    // Initialize mutex and condition variables with PTHREAD_PROCESS_SHARED
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&buffer->mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&buffer->image_ready, &cond_attr);
    pthread_cond_init(&buffer->space_available, &cond_attr);
    pthread_condattr_destroy(&cond_attr);

    // Create threads
    pthread_t producer_thread, consumer_thread;

    if (pthread_create(&producer_thread, NULL, image_producer, buffer) != 0) {
        perror("pthread_create producer");
        goto cleanup;
    }

    if (pthread_create(&consumer_thread, NULL, image_consumer, buffer) != 0) {
        perror("pthread_create consumer");
        pthread_cancel(producer_thread);
        goto cleanup;
    }

    // Wait for threads to complete
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    printf("\n=== Image processing completed ===\n");

cleanup:
    // Cleanup
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->image_ready);
    pthread_cond_destroy(&buffer->space_available);
    munmap(buffer, sizeof(image_buffer_t));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}
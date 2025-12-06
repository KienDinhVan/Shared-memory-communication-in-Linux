#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define SHM_NAME "/my_shared_memory"
#define SHM_SIZE 4096

typedef struct {
    int counter;
    char message[256];
} shared_data_t;

// Writer process
int writer() {
    // Create shared memory object
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    
    // Configure size
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        return 1;
    }
    
    // Map to address space
    shared_data_t* data = mmap(0, SHM_SIZE, 
                                PROT_READ | PROT_WRITE, 
                                MAP_SHARED, 
                                shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    
    // Write data
    data->counter = 0;
    while (data->counter < 10) {
        sprintf(data->message, "Message #%d from writer", data->counter);
        printf("Writer: %s\n", data->message);
        data->counter++;
        sleep(1);
    }
    
    // Cleanup
    if (munmap(data, SHM_SIZE) == -1) {
        perror("munmap");
        return 1;
    }

    if (close(shm_fd) == -1) {
        perror("close");
        return 1;
    }

    printf("Writer: Cleanup completed\n");
    return 0;
}

// Reader process
int reader() {
    // Open existing shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    
    // Map to address space (read-only)
    shared_data_t* data = mmap(0, SHM_SIZE, 
                                PROT_READ, 
                                MAP_SHARED, 
                                shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    
    // Read data
    int last_counter = -1;
    while (data->counter < 10) {
        if (data->counter != last_counter) {
            printf("Reader: %s (counter=%d)\n", data->message, data->counter);
            last_counter = data->counter;
        }
        usleep(100000); // 100ms
    }
    
    // Cleanup
    if (munmap(data, SHM_SIZE) == -1) {
        perror("munmap");
        return 1;
    }

    if (close(shm_fd) == -1) {
        perror("close");
        return 1;
    }

    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink");
        return 1;
    }

    printf("Reader: Cleanup completed and shared memory removed\n");
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [writer|reader]\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "writer") == 0) {
        return writer();
    } else if (strcmp(argv[1], "reader") == 0) {
        return reader();
    } else {
        fprintf(stderr, "Invalid argument. Use 'writer' or 'reader'\n");
        return 1;
    }
}
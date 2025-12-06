#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_KEY 0x1234
#define SHM_SIZE 4096

typedef struct {
    int counter;
    char message[256];
} shared_data_t;

int writer() {
    // Create shared memory segment
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }
    
    printf("Writer: Created shared memory segment with ID %d\n", shmid);
    
    // Attach to address space
    shared_data_t* data = (shared_data_t*) shmat(shmid, NULL, 0);
    if (data == (void*) -1) {
        perror("shmat");
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
    
    // Detach
    if (shmdt(data) == -1) {
        perror("shmdt");
        return 1;
    }

    printf("Writer: Detached from shared memory\n");
    return 0;
}

int reader() {
    // Access existing segment
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }
    
    printf("Reader: Attached to shared memory segment with ID %d\n", shmid);
    
    // Attach (read-only using SHM_RDONLY)
    shared_data_t* data = (shared_data_t*) shmat(shmid, NULL, SHM_RDONLY);
    if (data == (void*) -1) {
        perror("shmat");
        return 1;
    }
    
    // Read data
    int last_counter = -1;
    while (data->counter < 10) {
        if (data->counter != last_counter) {
            printf("Reader: %s (counter=%d)\n", data->message, data->counter);
            last_counter = data->counter;
        }
        usleep(100000);
    }
    
    // Detach
    if (shmdt(data) == -1) {
        perror("shmdt");
        return 1;
    }

    printf("Reader: Detached from shared memory\n");

    // Remove segment (last process should do this)
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        return 1;
    }
    printf("Reader: Removed shared memory segment\n");

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
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }
}

#include <sys/shm.h>
#include <sys/stat.h>

#include "errExit.h"
#include "shared_memory.h"

 int alloc_shared_memory(key_t shmKey, size_t size) {
    // get, or create, a shared memory segment
    int shmid = shmget(shmKey, size, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (shmid == -1)
        errExit("shmget failed");

    return shmid;
}

void *get_shared_memory(int shmid, int shmflg) {
    // attach the shared memory
    void *ptr_sh = shmat(shmid, NULL, shmflg);
    if (ptr_sh == (void *)-1)
        errExit("shmat failed");

    return ptr_sh;
}

void free_shared_memory(void *ptr_sh) {
    // detach the shared memory segments
    if (shmdt(ptr_sh) == -1)
        errExit("shmdt failed");
}

void remove_shared_memory(int shmid) {
    // delete the shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        errExit("shmctl failed");
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "shared_memory.h"
#include "semaphore.h"
#include "errExit.h"

#define BUFFER_SZ 100

#define REQUEST      0
#define DATA_READY   1

int create_sem_set(key_t semkey) {
    // Create a semaphore set with 2 semaphores
    int semid = semget(semkey, 2, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1)
        errExit("semget failed");

    // Initialize the semaphore set
    union semun arg;
    unsigned short values[] = {0, 0};
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1)
        errExit("semctl SETALL failed");

    return semid;
}

void copy_file(const char *pathname, char *buffer) {
    // open in read only mode the file
    int file = open(pathname, O_RDONLY);
    if (file == -1) {
        printf("File %s does not exist\n", pathname);
        buffer[0] = -1;
        return;
    }

    // read a chunks of BUFFER_SZ - 1 characters
    ssize_t bR = read(file, buffer, BUFFER_SZ - 1);
    if (bR > 0)
        buffer[bR] = '\0'; // end the lie with '\0'
    else
        printf("read failed\n");

    // close the file descriptor
    close(file);
}

int main (int argc, char *argv[]) {

    // check command line input arguments
    if (argc != 3) {
        printf("Usage: %s shared_memory_key semaphore_key\n", argv[0]);
        exit(1);
    }

    // read the shared memory key defined by user
    key_t shmKey = atoi(argv[1]);
    if (shmKey <= 0) {
        printf("The shared_memory_key must be greater than zero!\n");
        exit(1);
    }

    // read the semaphore set key defined by user
    key_t semkey = atoi(argv[2]);
    if (semkey <= 0) {
        printf("The semaphore_key must be greater than zero!\n");
        exit(1);
    }

    // allocate a shared memory segment
    printf("<Server> allocating a shared memory segment...\n");
    int shmidServer = alloc_shared_memory(shmKey, sizeof(struct Request));

    // attach the shared memory segment
    printf("<Server> attaching the shared memory segment...\n");
    struct Request *request = (struct Request*)get_shared_memory(shmidServer, 0);

    // create a semaphore set
    printf("<Server> creating a semaphore set...\n");
    int semid = create_sem_set(semkey);

    // wait for a Request
    printf("<Server> waiting for a request...\n");
    semOp(semid, REQUEST, -1);

    // allocate a shared memory segment
    printf("<Server> getting the client's shared memory segment...\n");
    int shmidClient = alloc_shared_memory(request->shmKey, sizeof(char) * BUFFER_SZ);

    // attach the shared memory segment
    printf("<Server> attaching the client's shared memory segment...\n");
    char *buffer = (char *)get_shared_memory(shmidClient, 0);

    // copy file into the shared memory
    printf("<Server> coping a file into the client's shared memory...\n");
    copy_file(request->pathname, buffer);

    // notify that data was stored into client's shared memory
    printf("<Server> notifing data is ready...\n");
    semOp(semid, DATA_READY, 1);

    // detach the shared memory segment
    printf("<Client> detaching the client's shared memory segment...\n");
    free_shared_memory(buffer);

    // detach the shared memory segment
    printf("<Server> detaching the shared memory segment...\n");
    free_shared_memory(request);

    // remove the shared memory segment
    printf("<Server> removing the shared memory segment...\n");
    remove_shared_memory(shmidServer);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "shared_memory.h"
#include "semaphore.h"
#include "errExit.h"

#define BUFFER_SZ 100

#define REQUEST      0
#define DATA_READY   1

int main (int argc, char *argv[]) {

    // check command line input arguments
    if (argc != 4) {
        printf("Usage: %s server_shared_memory_key semaphore_key client_shared_memory_key\n", argv[0]);
        exit(1);
    }

    // read the server's shared memory key
    key_t shmKeyServer = atoi(argv[1]);
    if (shmKeyServer <= 0) {
        printf("The server_shared_memory_key must be greater than zero!\n");
        exit(1);
    }

    // read the semaphore key defined by user
    key_t semkey = atoi(argv[2]);
    if (semkey <= 0) {
        printf("The semaphore_key must be greater than zero!\n");
        exit(1);
    }

    // read the semaphore key defined by user
    key_t shmKeyClient = atoi(argv[3]);
    if (shmKeyClient <= 0) {
        printf("The client_shared_memory_key must be greater than zero!\n");
        exit(1);
    }

    // get the server's shared memory segment
    printf("<Client> getting the server's shared memory segment...\n");
    int shmidServer = alloc_shared_memory(shmKeyServer, sizeof(struct Request));

    // attach the shared memory segment
    printf("<Client> attaching the server's shared memory segment...\n");
    struct Request *request = (struct Request *)get_shared_memory(shmidServer, 0);

    // read a pathname from user
    printf("<Client> Insert pathname: ");
    scanf("%s", request->pathname);

    // allocate a shared memory segment
    printf("<Client> allocating a shared memory segment...\n");
    int shmidClient = alloc_shared_memory(shmKeyClient, sizeof(char) * BUFFER_SZ);

    // attach the shared memory segment
    printf("<Client> attaching the shared memory segment...\n");
    char *buffer = (char *)get_shared_memory(shmidClient, 0);

    // copy shmKeyClient into the server's shared memory segment
    request->shmKey = shmKeyClient;

    // get the server's semaphore set
    int semid = semget(semkey, 2, S_IRUSR | S_IWUSR);
    if (semid > 0) {
        // unlock the server
        semOp(semid, REQUEST, 1);
        // wait for data
        semOp(semid, DATA_READY, -1);

        printf("<Client> reading data from the shared memory segment...\n");
        if (buffer[0] == -1)
            printf("File %s does not exist\n", request->pathname);
        else
            printf("%s\n", buffer);
    } else
        printf("semget failed");

    // detach the shared memory segment
    printf("<Client> detaching the server's shared memory segment...\n");
    free_shared_memory(buffer);

    // detach the server's shared memory segment
    printf("<Client> detaching the server's shared memory segment...\n");
    free_shared_memory(request);

    // remove the shared memory segment
    printf("<Client> removing the shared memory segment...\n");
    remove_shared_memory(shmidClient);

    // remove the created semaphore set
    printf("<Client> removing the semaphore set...\n");
    if (semctl(semid, 0 /*ignored*/, IPC_RMID, NULL) == -1)
        errExit("semctl IPC_RMID failed");

    return 0;
}

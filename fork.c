#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "errExit.h"

#define BUFFER_SZ 150

int main (int argc, char *argv[]) {

    // Get the username of user
    char *username = getenv("USER");
    if (username == NULL)
        username = "unknown";

    // Get the home directory of the user
    char *homeDir  = getenv("HOME");
    if (homeDir == NULL) {
        printf("unknown home directory\n");
        exit(0);
    }

    // Get the current process's working directory
    char buffer[BUFFER_SZ];
    if (getcwd(buffer, sizeof(buffer)) == NULL) {
        printf("getcwd failed\n");
        exit(1);
    }

    // check if the current process's working directory is a sub directory of
    // the user's home directory
    // solution 1
    int subDir = strncmp(buffer, homeDir, strlen(homeDir));
    // solution 2
    //int subDir = (strstr(buffer, homeDir) == &buffer[0])? 0 : 1;

    if (subDir == 0)
        printf("Caro %s, sono gia' nel posto giusto!\n", username);
    else {
        // move the process into the user's home directory
        if (chdir(homeDir) == -1)
            errExit("chdir failed");

        // create an empty file
        int fd = open("empty_file.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd == -1)
            errExit("open failed");

        // close the file descriptor of the empty file
        if (close(fd) == -1)
            errExit("close failed");

        printf("Caro %s, sono dentro la tua home!\n", username);
    }

    return 0;
}
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "errExit.h"

const char *fileName = "myfile1";

int main (int argc, char *argv[]) {

    // check command line input arguments
    if (argc <= 1) {
        printf("Usage: %s cmd [arguments]\n", argv[0]);
        return 0;
    }

    switch (fork()) {
        case -1: {
            errExit("fork failed");
        }
        case 0: {
            // close the standard output and error stream
            close(STDOUT_FILENO); // close file descriptor 1
            close(STDERR_FILENO); // close file descriptor 2

            // create a new file. The value returned by open will be 1 as it is
            // lowest available index
            int fd = open(fileName,
                          O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

            if (fd == -1)
                errExit("open failed");

            // clone the file descriptor 1. The value returned by dup will be 2
            // as it is  lowest available index
            dup(fd);

            // both file descriptors 1 and 2 are pointing to the created file now.

            // replace the current process image with a new process image
            execvp(argv[1], &argv[1]);
            errExit("execvp failed");
        }
        default: {
            int status;
            // wait the termination of the child process
            if (wait(&status) == -1)
                errExit("wait failed");

            printf("Command %s exited with status %d\n", argv[1], WEXITSTATUS(status));
        }
    }

    return 0;
}

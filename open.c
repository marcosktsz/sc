#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "errExit.h"

#define BUFFER_SZ 100
char buffer[BUFFER_SZ + 1];

int main (int argc, char *argv[]) {

    // For each file provided as input argument
    for (int i = 1; i < argc; ++i) {
        // open the file in read only mode
        int file = open(argv[i], O_RDONLY);
        if (file == -1) {
            printf("File %s does not exist\n", argv[i]);
            continue;
        }

        ssize_t bR = 0;
        do {
            // read the file in chunks
            bR = read(file, buffer, BUFFER_SZ);
            if (bR > 0) {
                // add the character '\0' to let printf know where a
                // string ends
                buffer[bR] = '\0';
                printf("%s", buffer);
            }
        } while (bR > 0);

        // close the file descriptor
        close(file);
    }

    return 0;
}
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "errExit.h"

#define BUFFER_SZ 100
char buffer[BUFFER_SZ + 1];

int main (int argc, char *argv[]) {

    // For each file provided as input argument
    for (int i = 1; i < argc; ++i) {
        // open the file in read only mode
        int file = open(argv[i], O_RDONLY);
        if (file == -1) {
            printf("File %s does not exist\n", argv[i]);
            continue;
        }
        // move the offset location to the end of the file
        off_t currentOffset = lseek(file, -1, SEEK_END);
        char c;
        while (currentOffset >= 0) {
            // read a char from the file
            ssize_t bR = read(file, &c, sizeof(char));
            //printf("c-> [%c]\n", c);
            // write the read char on standard output
            if (bR == sizeof(char)) {
                if (write(STDOUT_FILENO, &c, sizeof(char)) != sizeof(char))
                    errExit("write failed");
            }
            currentOffset = lseek(file, -2, SEEK_CUR);
        }
        // close the file descriptor
        close(file);
        // print a new line before starting the next file
        c = '\n';
        write(STDOUT_FILENO, &c, sizeof(char));
    }

    return 0;
}

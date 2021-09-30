#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/msg.h>

#include "order.h"
#include "errExit.h"


// the message queue identifier
int msqid = -1;

void signTermHandler(int sig) {
    // do we have a valid message queue identifier?
    if (msqid > 0) {
        if (msgctl(msqid, IPC_RMID, NULL) == -1)
            errExit("msgctl failed");
        else
            printf("<Server> message queue removed successfully\n");
    }

    // terminate the server process
    exit(0);
}

int main (int argc, char *argv[]) {
    // check command line input arguments
    if (argc != 2) {
        printf("Usage: %s message_queue_key\n", argv[0]);
        exit(1);
    }

    // read the message queue key defined by user
    int msgKey = atoi(argv[1]);
    if (msgKey <= 0) {
        printf("The message queue key must be greater than zero!\n");
        exit(1);
    }

    // set of signals (N.B. it is not initialized!)
    sigset_t mySet;
    // initialize mySet to contain all signals
    sigfillset(&mySet);
    // remove SIGTERM from mySet
    sigdelset(&mySet, SIGTERM);
    // blocking all signals but SIGTERM
    sigprocmask(SIG_SETMASK, &mySet, NULL);

    // set the function sigHandler as handler for the signal SIGTERM
    if (signal(SIGTERM, signTermHandler) == SIG_ERR)
        errExit("change signal handler failed");

    printf("<Server> Making MSG queue...\n");
    // get the message queue, or create a new one if it does not exist
    msqid = msgget(msgKey, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (msqid == -1)
        errExit("msgget failed");

    struct order order;

    // endless loop
    while (1) {
        // read a message from the message queue
        size_t mSize = sizeof(struct order) - sizeof(long);
        if (msgrcv(msqid, &order, mSize, 0, 0) == -1)
            errExit("msgget failed");

        // print the order on standard output
        printOrder(&order);
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/msg.h>

#include "order.h"
#include "errExit.h"

// The readInt reads a integer value from an array of chars
// It checks that only a number n is provided as an input parameter,
// and that n is greater than 0
int readInt(const char *s) {
    char *endptr = NULL;

    errno = 0;
    long int res = strtol(s, &endptr, 10);

    if (errno != 0 || *endptr != '\n' || res < 0) {
        printf("invalid input argument\n");
        exit(1);
    }

    return res;
}

int main (int argc, char *argv[]) {

    // check command line input arguments
    if (argc != 2) {
        printf("Usage: %s message_queue_key\n", argv[0]);
        exit(1);
    }

    // read the message queue key defined by user
    int msgKey = atoi(argv[1]);
    if (msgKey <= 0) {
        printf("The message queue key must be greater than zero!\n");
        exit(1);
    }

    // get the message queue identifier
    int msqid = msgget(msgKey, S_IRUSR | S_IWUSR);
    if (msqid == -1)
        errExit("msgget failed");

    char buffer[10];
    size_t len;

    // crea an order data struct
    struct order order;

    // by default, the order has type 1
    order.mtype = 1;

    // read the code of the client's order
    printf("Insert order code: ");
    fgets(buffer, sizeof(buffer), stdin);
    order.code = readInt(buffer);

    // read a description of the order
    printf("Insert a description: ");
    fgets(order.description, sizeof(order.description), stdin);
    len = strlen(order.description);
    order.description[len - 1] = '\0';

    // read a quantity
    printf("Insert quantity: ");
    fgets(buffer, sizeof(buffer), stdin);
    order.quantity = readInt(buffer);

    // read an e-mail
    printf("Insert an e-mail: ");
    fgets(order.email, sizeof(order.email), stdin);
    len = strlen(order.email);
    order.email[len - 1] = '\0';

    // send the order to the server through the message queue
    printf("Sending the order...\n");
    size_t mSize = sizeof(struct order) - sizeof(long);
    if (msgsnd(msqid, &order, mSize, 0) == -1)
        errExit("msgsnd failed");

    printf("Done\n");
    return 0;
}

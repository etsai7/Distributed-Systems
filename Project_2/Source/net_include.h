#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h> 
#include <netdb.h>

#include <errno.h>

#define PORT	     10030//5555
//#define PORT	     11011

#define MAX_MESS_LEN 1300
#define MAX_MACHINES 10
#define MAX_NACKS 320
//#define NAME_LENGTH 80
#define MAX_BURST 32
#define TOKEN_ID 1
#define MESS_ID 2
#define TOKEN_ACK_ID 3
#define EMPTY -1
#define COMPLETE 1
#define INCOMPLETE 0

typedef struct{
    int token_id;
    int next_sender;
	int aru;
    int max_id_sent;
	int last_lowerer;
    int nacks[MAX_NACKS];
	int completed[MAX_MACHINES + 1];
}Tok;

typedef struct{
	int mess_id;
    int sender_id;
	int originator_id;
    int seq;
    int rand_id;
    char data[MAX_MESS_LEN];
}Message;

typedef struct{
	int token_ack_id;
}TokAck;

/* Performs left - right and returns the result as a struct timeval.
In case of negative result (right > left), zero elapsed time is returned */
struct timeval diffTime(struct timeval left, struct timeval right)
{
	struct timeval diff;

	diff.tv_sec  = left.tv_sec - right.tv_sec;
	diff.tv_usec = left.tv_usec - right.tv_usec;

	if (diff.tv_usec < 0) {
		diff.tv_usec += 1000000;
		diff.tv_sec--;
	}

	if (diff.tv_sec < 0) {
		printf("WARNING: diffTime has negative result, returning 0!\n");
		diff.tv_sec = diff.tv_usec = 0;
	}

	return diff;
}

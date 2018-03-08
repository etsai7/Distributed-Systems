#include <stdio.h>

#include <stdlib.h>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h> 
#include <netdb.h>

#include <errno.h>

#define PORT	     10030
#define WINDOW_SIZE     16
#define MAX_PACK_SIZE 1300

typedef struct {
  unsigned int size;
  unsigned int wid;
  unsigned int pid;
  char data[MAX_PACK_SIZE];
} Message;

typedef struct {
  int busy;
  int ack;
  int nacks;
  int nack_pack[WINDOW_SIZE];
} Response;

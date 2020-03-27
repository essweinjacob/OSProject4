#ifndef SHARED_H
#define SHARED_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>	// Fork
#include <sys/shm.h>	// Shmget
#include <unistd.h>	// Windows stuff
#include <sys/wait.h>	// waitpid
#include <stdbool.h>	// Bool
#include <sys/msg.h>	// Shared Messaging
#include <sys/sem.h>	// Semaphores

#define MAX_PROCESS 18

// Time
struct Clock{
	unsigned int sec;
	unsigned int nsec;
};

// Proccess Block
struct ProcBlock{	
	int simPID;
	pid_t pid;
	int startTimeSec;
	int startTimeNSec;
	bool inProg;
	int prio;
};

struct Msg{
	long mtype;
	char msgtxt[200];
};

struct QNode{
	int index;
	struct QNode *next;
};

struct Queue{
	struct QNode *front;
	struct QNode *rear;
};

#endif

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
#include <string.h>	// Bitmap
#include <sys/time.h>	// Timer
#include <time.h>
#include <signal.h>	// Signal handleing
#include <stdint.h>	// uint32_t

#define MAX_PROC 18

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

// Process
struct Process{
	int index;
	pid_t pid;
	int prio;
};

// Shared memory message
struct Msg{
	long type;
	int flag;
	int index;
	pid_t childPID;
	int prio;
	unsigned int burstTime;
	unsigned int spawnSec;
	unsigned int spawnNSec;
	unsigned int waitSec;
	unsigned int waitNSec;
	double waitTime;
	unsigned int sec;
	unsigned int nsec;
	char message[1024];
};

// Node in queue
struct QNode{
	int index;
	struct QNode *next;
};

// Queue
struct Queue{
	struct QNode *front;
	struct QNode *rear;
};

// Function to create a queue
struct Queue *createQueue(){
	struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
	q->front = NULL;
	q->rear = NULL;
	return q;
};

#endif

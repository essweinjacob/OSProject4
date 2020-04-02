#ifndef SHARED_H
#define SHARED_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>	// Fork
#include <sys/shm.h>	// Shmget
#include <unistd.h>	// Windows
#include <sys/wait.h>	// waitpid
#include <stdbool.h>	// Boolean expressions
#include <sys/msg.h>	// Message queues
#include <sys/sem.h>	// Semaphores
#include <string.h>	// Strings
#include <sys/time.h>	// Time
#include <time.h>	// Time
#include <signal.h>	// Signal Handeling
#include <stdint.h>	// unit32_t for bitmap
#include <errno.h>	// errno

#define MAX_PROC 18
#define QUANTUM 10000

//Time
struct Clock{
	unsigned int sec;
	unsigned int nsec;
};

// PCB
struct ProcessControlBlock{
	unsigned int index;
	pid_t pid;
	unsigned int prio;
	bool hasStarted;
	bool isFinished;
	unsigned int startSec;
	unsigned int startNSec;
};

struct Msg{
	unsigned int index;
	pid_t pid;
	unsigned int prio;
	unsigned int burstTime;
	//char message[1024];
};

// Nodes for queues
struct QNode{
	int index;
	struct QNode *next;
};

// Queues
struct Queue{
	struct QNode *front;
	struct QNode *rear;
};

// Function to create nodes
struct Queue *createQueue(){
	struct Queue *q = (struct Queue*)malloc(sizeof(struct Queue));
	q->front = NULL;
	q->rear = NULL;
	return q;
};


#endif

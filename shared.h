#ifndef SHARED_H
#define SHARED_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>	// Fork
#include <sys/shm.h>	// Shmget
#include <unistd.h>	// Windows stuff
#include <sys/wait.h>	// waitpid

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
};

#endif

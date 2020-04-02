#include "shared.h"

// Functions
void getMsgQueue();
void getClock();
void getSema();
void getPCB();

// Global Variables
// Message Queue Variables
key_t msgQueueKey = -1;
int msgQueueID = -1;
struct Msg msgInfo;
// Message Queue 2
key_t cMsgQueueKey = -1;
int cMsgQueueID = -1;
struct Msg cMsgInfo;

// Clock Variables
key_t clockKey = -1;
int clockID = -1;
struct Clock *timer;
int startTimeSec;
int startTimeNSec;

// Semaphore Variables
key_t semaKey = -1;
int semaID = -1;
struct sembuf semOp;

// PCB Variables
key_t pcbKey = -1;
int pcbID = -1;
struct ProcessControlBlock *pcb;

int main(int argc, char *argv[]){
	// Get all the shared memeory
	getMsgQueue();
	getClock();
	getSema();
	getPCB();
	
	pid_t pid = getpid();
	
	msgrcv(msgQueueID, &msgInfo, (sizeof(struct Msg) - sizeof(long)), 0, 0);
	printf("USER RCV RESPONE: %s\n", strerror(errno));
	cMsgInfo.index = msgInfo.index;
	cMsgInfo.pid = msgInfo.pid;
	cMsgInfo.prio = msgInfo.prio;
	msgsnd(cMsgQueueID, &cMsgInfo, (sizeof(struct Msg) - sizeof(long)), 1);
	//printf("USER SND RESPONCE: %s\n", strerror(errno));
	cMsgInfo.burstTime = 100;
	msgsnd(cMsgQueueID, &cMsgInfo, (sizeof(struct Msg) - sizeof(long)), 1);
	//printf("USER SND RESPONCE: %s\n", strerror(errno));
	
	return cMsgInfo.index;
}

void getMsgQueue(){
	msgQueueKey = ftok("./oss.c", 0);
	if(msgQueueKey == -1){
		perror("ERROR IN USER.C: FAILED TO GENERATE KEY FOR MESSAGE QUEUE");
		exit(EXIT_FAILURE);
	}
	msgQueueID = msgget(msgQueueKey, IPC_CREAT | 0600);
	if(msgQueueID == -1){
		perror("ERROR IN USER.C: FAILED TO GET KEY FOR MESSAGE QUEUE");
		exit(EXIT_FAILURE);
	}

	cMsgQueueKey = ftok("./oss.c", 4);
	if(cMsgQueueKey == -1){
		perror("ERROR IN USER.C: FAILED TO GENERATE KEY FOR CHILD MESSAGE QUEUE");
		exit(EXIT_FAILURE);
	}
	cMsgQueueID = msgget(cMsgQueueKey, IPC_CREAT | 0600);
	if(cMsgQueueID == -1){
		perror("ERROR IN USER.C: FAILED TO GET KEY FOR CHILD MESSAGE QUEUE");
		exit(EXIT_FAILURE);
	}
}

void getClock(){
	clockKey = ftok("./oss.c", 1);
	if(clockKey == -1){
		perror("ERROR IN USER.C: FAILED TO GENERATE KEY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	clockID = shmget(clockKey, sizeof(struct Clock*), 0666 | IPC_CREAT);
	if(clockID == -1){
		perror("ERROR IN USER.C: FAILER TO GET KEY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	timer = shmat(clockID, (void*) 0, 0);
	if(timer == (void*)-1){
		perror("ERROR IN USER.C: FAILED TO ATTACH MEMEORY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	startTimeSec = timer->sec;
	startTimeNSec = timer->nsec;
}

void getSema(){
	semaKey = ftok("./oss.c", 2);
	if(semaKey == -1){
		perror("ERROR IN USER.C: FAILED TO GENERATE KEY FOR SEMAPHORE");
		exit(EXIT_FAILURE);
	}
	semaID = semget(semaKey, 1, 0666 | IPC_CREAT);
	if(semaID == -1){
		perror("ERROR IN USER.C: FAILED TO GET KEY FOR SEMAHPORE");
		exit(EXIT_FAILURE);
	}	
}

void getPCB(){
	pcbKey = ftok("./oss.c", 3);
	if(pcbKey == -1){
		perror("ERROR IN USER.C: FAILED TO GENERATE KEY FOR PCB");
		exit(EXIT_FAILURE);
	}
	size_t procTableSize = sizeof(struct ProcessControlBlock) * MAX_PROC;
	pcbID = shmget(pcbKey, procTableSize, 0666 | IPC_CREAT);
	if(pcbID == -1){
		perror("ERROR IN USER.C: FAILED TO GET KEY FOR PCB");
		exit(EXIT_FAILURE);
	}
	pcb = shmat(pcbID, (void*)0, 0);
	if(pcb == (struct ProcessControlBlock*)-1){
		perror("ERROR IN USER.C: FAILED TO ATTACH MEMEORY FOR PCB");
		exit(EXIT_FAILURE);
	}
}

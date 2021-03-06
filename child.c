#include "shared.h"

// Functions used
void getMsgQue();
void getClock();
void getSema();
void getPCB();
void semLock();
void semRelease();

// Global varaiables
// Message queue variables
key_t msgQueKey = -1;
int msgQueID = -1;
struct Msg msgInfo;
// Clock variables
key_t clockKey = -1;
int clockID = -1;
struct Clock *timer;
// Semaphore variables
key_t semaKey = -1;
int semaID = -1;
struct sembuf semOp;
// PCB Variables
key_t pcbKey = -1;
int pcbID = -1;
struct ProcBlock *pcb;

int main(int argc, int argv[]){
	// Get all shared memeory stuff
	getMsgQue();
	getClock();
	getSema();
	getPCB();

	return EXIT_SUCCESS;
}

void getMsgQue(){
	msgQueKey = ftok("./oss.c", 0);
	msgQueID = msgget(msgQueKey, 0600);
	if(msgQueID < 0){
		perror("ERROR IN child.c: FAILED TO GET MSG QUEUE FROM SHARED MEM");
		exit(EXIT_FAILURE);
	}
}

void getClock(){
	clockKey = ftok("./oss.c", 1);
	if(clockKey == -1){
		perror("ERROR IN child.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	clockID = shmget(clockKey, sizeof(struct Clock*), 0666 | IPC_CREAT);
	if(clockID == -1){
		perror("ERROR IN child.c: FAILED TO GET KEY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	timer = (struct Clock*)shmat(clockID, (void*) 0, 0);
	if(timer == (void*)-1){
		perror("ERROR IN child.c: FAILED TO ATTACH MEMEORY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
}

void getSema(){
	semaKey = ftok("./oss.c", 2);
	if(semaKey == -1){
		perror("ERROR IN child.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR SEMAPHORE");
		exit(EXIT_FAILURE);
	}
	semaID = semget(semaKey, 1, 0666 | IPC_CREAT);
	if(semaID == -1){
		perror("ERROR IN child.c: FAILED TO GET KEY FOR SEMAPHORE");
		exit(EXIT_FAILURE);
	}
}

void getPCB(){
	pcbKey = ftok("./oss.c", 3);
	if(pcbKey == -1){
		perror("ERROR IN child.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR PCB");
		exit(EXIT_FAILURE);
	}
	size_t procTableSize = sizeof(struct ProcBlock) * MAX_PROC;
	pcbID = shmget(pcbKey, procTableSize, 0666 | IPC_CREAT);
	if(pcbID == -1){
		perror("ERROR IN child.c: FAILED TO GET KEY FOR PCB");
		exit(EXIT_FAILURE);
	}
	pcb = (struct ProcBlock*)shmat(pcbID, (void*) 0, 0);
	if(pcb == (void*)-1){
		perror("ERROR IN child.c: FAILED TO ATTACH MEMEORY FOR PCB");
		exit(EXIT_FAILURE);
	}
}

void semLock(){
	semOp.sem_num = 0;
	semOp.sem_op = -1;
	semOp.sem_flg = 0;
	semop(semaID, &semOp, 1);
}

void semRelease(){
	semOp.sem_num = 0;
	semOp.sem_op = 1;
	semOp.sem_flg = 0;
	semop(semaID, &semOp, 1);
}

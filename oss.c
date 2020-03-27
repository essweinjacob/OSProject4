#include "shared.h"

#define MAX_PROCESS 18

// Functions used
void god(int signal);
void freeMem();
void getMsgQue();
void getClock();
void getSema();
void getPCB();
void incTimer();
bool procReady(int index);
bool makeNewProc(int prevProcSec, int prevProcNSec, int betweenProcSec, int betweenProcNSec);
void generateProc(int index);
void semLock();
void semRelease();

// Global Variables
// Pid list info
int *listOfPIDS;
int numOfPIDS = 0;
// Message Queue Variables
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
// Queues
struct Queue *highPrio;
struct Queue *midPrio;
struct Queue *lowPrio;

int main(int argc, int argv[]){
	// Set up all shared memory
	getMsgQue();
	getClock();
	getSema();
	getPCB();

	// Variables for Forking
	int index = 0;
	int childDone = 0;
	int status = 0;
	int testCount = 0;
	int activeChildren = 0;
	bool exitStatus = false;
	pid_t pid;
	
	// Its forking time
	while(exitStatus == false){
		if(activeChildren <= 18 && procReady(index)){
			// Generate new process
			pid = fork();
			generateProc(childDone);
			if(pid < 0){
				perror("ERROR IN oss.c: Failed to fork");
				return EXIT_FAILURE;
			}
			else if(pid == 0){
				char * args[] = {"./child", NULL};
				execvp(args[0], args);
			}
			activeChildren++;
		}
		if((pid = waitpid((pid_t)-1, &status, WNOHANG)) > 0){
			if(WIFEXITED(status)){
				childDone++;
				break;
			}
		}
		if(activeChildren >= 20){
			god(1);
		}
	}
	
	freeMem();

	printf("Program finished?\n");
}

void freeMem(){
	shmctl(clockID, IPC_RMID, NULL);
	shmctl(pcbID, IPC_RMID, NULL);
	shmctl(semaID, IPC_RMID, NULL);
	msgctl(msgQueID, IPC_RMID, NULL);
	free(listOfPIDS);
}

void god(int signal){
	int i;
	for(i = 0; i < numOfPIDS; i++){
		kill(listOfPIDS[i], SIGTERM);
	}
	printf("GOD HAS BEEN CALLED AND THE RAPTURE HAS BEGUN. SOON THERE WILL BE NOTHING\n");
	freeMem();
	kill(getpid(), SIGTERM);
}

void getMsgQue(){
	msgQueKey = ftok("./oss.c", 0);
	msgQueID = msgget(msgQueKey, IPC_CREAT | 0600);
	if(msgQueID < 0){
		perror("ERROR IN oss.c: FAILED TO GET MSG QUEUE FROM SHARED MEM");
		exit(EXIT_FAILURE);
	}
}

void getClock(){
	clockKey = ftok("./oss.c", 1);
	if(clockKey == -1){
		perror("ERROR IN oss.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	clockID = shmget(clockKey, sizeof(struct Clock*), 0666 | IPC_CREAT);
	if(clockID == -1){
		perror("ERROR IN oss.c: FAILED TO GET KEY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	timer = (struct Clock*)shmat(clockID, (void*) 0, 0);
	if(timer == (void*)-1){
		perror("ERROR IN oss.c: FAILED TO ATTACH MEMEORY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
}

void getSema(){
	semaKey = ftok("./oss.c", 2);
	if(semaKey == -1){
		perror("ERROR IN oss.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR SEMAPHORE");
		exit(EXIT_FAILURE);
	}
	semaID = semget(semaKey, 1, 0666 | IPC_CREAT);
	if(semaID == -1){
		perror("ERROR IN oss.c: FAILED TO GET KEY FOR SEMAPHORE");
		exit(EXIT_FAILURE);
	}
	semctl(semaID, 0, SETVAL, 1);
}

void getPCB(){
	pcbKey = ftok("./oss.c", 3);
	if(pcbKey == -1){
		perror("ERROR IN child.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR PCB");
		exit(EXIT_FAILURE);
	}
	size_t procTableSize = sizeof(struct ProcBlock) * MAX_PROCESS;
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

void incTimer(){
	timer->nsec += 10000;
	while(timer->nsec >= 1000000000){
		timer->sec++;
		timer->nsec -= 1000000000;
	}
}

bool makeNewProc(int prevProcSec, int prevProcNSec, int betweenProcSec, int betweenProcNSec){
	if((((timer->sec * 1000000000) + timer->nsec) - ((prevProcSec * 1000000000) + prevProcNSec)) >= ((betweenProcSec * 1000000000) + betweenProcNSec)){
		return true;
	}else{
		return false;
	}
}

bool procReady(int index){
	if(pcb[index].inProg == true){
		return false;
	}else{
		return true;
	}
}

void generateProc(int index){
	pcb[index].simPID = index;
	pcb[index].pid = getpid();
	pcb[index].startTimeSec = timer->sec;
	pcb[index].startTimeNSec = timer->nsec;
	pcb[index].inProg = true;
	srand(index);
	pcb[index].prio = (rand() % (2 - 0 + 1));
	
	printf("Generating Process with PID %d and putting it in queue %d at time %d:%d\n", pcb[index].simPID, 0, pcb[index].startTimeSec, pcb[index].startTimeNSec);
}

void dispatchProc(int index){
	int dispatchTimeSec = timer->sec;
	int dispatchTimeNSec = timer->nsec;
	printf("Dispatching process with PID %d from queue at time %d:%d\n", pcb[index].simPID, dispatchTimeSec, dispatchTimeNSec);
	/*
	 * Stuff Happens
	 */
	printf("Total time this dispatch took was %d seconds and %d nanoseconds\n", timer->sec - dispatchTimeSec, timer->nsec - dispatchTimeNSec);
}

void recevingProc(int index){
	int receiveTimeSec;
	int receiveTimeNSec;
	printf("Receving that process with PID %d ran for %d seconds and %d nanoseconds\n", pcb[index].simPID, timer->sec, timer->nsec);
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

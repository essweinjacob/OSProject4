#include "shared.h"

// Functions used
void god(int signal);
void freeMem();
void getMsgQue();
void getClock();
void getSema();
void getPCB();
void incTimer();
struct QNode *newNode(int index);
void enQueue(struct Queue* q, int index);
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

// Stuff for time
int prevProcSec = 0;
int prevProcNSec = 0;

// Bit map
unsigned char bitmap[MAX_PROC];

int main(int argc, int argv[]){
	// Set up 3 seconds timer
	struct itimerval time1;
	time1.it_value.tv_sec = 3;
	time1.it_value.tv_usec = 0;
	time1.it_interval = time1.it_value;
	signal(SIGALRM, god);
	setitimer(ITIMER_REAL, &time1, NULL);

	// Catch control c signal handleing
	signal(SIGINT, god);
	signal(SIGPROF, god);

	// Set up all shared memory
	getMsgQue();
	getClock();
	getSema();
	getPCB();

	freeMem();

	printf("Program finished?\n");
}

void freeMem(){
	shmctl(clockID, IPC_RMID, NULL);
	shmctl(pcbID, IPC_RMID, NULL);
	semctl(semaID, 0, IPC_RMID);
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
		freeMem();
		exit(EXIT_FAILURE);
	}
}

void getClock(){
	clockKey = ftok("./oss.c", 1);
	if(clockKey == -1){
		perror("ERROR IN oss.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR CLOCK");
		freeMem();
		exit(EXIT_FAILURE);
	}
	clockID = shmget(clockKey, sizeof(struct Clock*), 0666 | IPC_CREAT);
	if(clockID == -1){
		perror("ERROR IN oss.c: FAILED TO GET KEY FOR CLOCK");
		freeMem();
		exit(EXIT_FAILURE);
	}
	timer = (struct Clock*)shmat(clockID, (void*) 0, 0);
	if(timer == (void*)-1){
		perror("ERROR IN oss.c: FAILED TO ATTACH MEMEORY FOR CLOCK");
		freeMem();
		exit(EXIT_FAILURE);
	}
}

void getSema(){
	semaKey = ftok("./oss.c", 2);
	if(semaKey == -1){
		perror("ERROR IN oss.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR SEMAPHORE");
		freeMem();
		exit(EXIT_FAILURE);
	}
	semaID = semget(semaKey, 1, 0666 | IPC_CREAT);
	if(semaID == -1){
		perror("ERROR IN oss.c: FAILED TO GET KEY FOR SEMAPHORE");
		freeMem();
		exit(EXIT_FAILURE);
	}
	semctl(semaID, 0, SETVAL, 1);
}

void getPCB(){
	pcbKey = ftok("./oss.c", 3);
	if(pcbKey == -1){
		perror("ERROR IN child.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR PCB");
		freeMem();
		exit(EXIT_FAILURE);
	}
	size_t procTableSize = sizeof(struct ProcBlock) * MAX_PROC;
	pcbID = shmget(pcbKey, procTableSize, 0666 | IPC_CREAT);
	if(pcbID == -1){
		perror("ERROR IN child.c: FAILED TO GET KEY FOR PCB");
		freeMem();
		exit(EXIT_FAILURE);
	}
	pcb = (struct ProcBlock*)shmat(pcbID, (void*) 0, 0);
	if(pcb == (void*)-1){
		perror("ERROR IN child.c: FAILED TO ATTACH MEMEORY FOR PCB");
		freeMem();
		exit(EXIT_FAILURE);
	}
}

struct QNode *newNode(int index){
	struct QNode *temp = (struct QNode*)malloc(sizeof(struct QNode));
	temp->index = index;
	temp->next = NULL;
	return temp;
}

void enQueue(struct Queue* q, int index){
	// Create new node
	struct QNode * temp = newNode(index);

	// Check if queue is empty
	if(q->rear == NULL){
		// If it is then the new node the first and last node
		q->front = q->rear = temp;
	}else{
		// Otherwise add new node to the end of the queue and mvoe the rear
		q->rear->next = temp;
		q->rear = temp;
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

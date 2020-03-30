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
void moveData(struct ProcBlock *pcb, struct Process *c);
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

// Other
int forkLaunched = 0;

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

	// Setup bitmap
	memset(bitmap, '\0', sizeof(bitmap));

	// Variables for forking
	int index = -1;
	bool isFree = false;
	int currentQueue = 0;
	pid_t pid;

	// Initial timer/clock setup
	timer->sec = 0;
	timer->nsec = 0;
	
	// Forking
	while(1){
		// Check if position in bitmap is still open
		isFree = false;
		int procCount = 0;
		while(1){
			index = (index + 1) % MAX_PROC;
			// Get place in bit map
			uint32_t bit = bitmap[index / 8] & (1 << (index % 8));
			if(bit == 0){
				isFree = true;
				break;
			}else{
				isFree = false;
			}

			if(procCount >= MAX_PROC - 1){
				perror("BIT MAP IS FULL\n");
				break;
			}
			procCount++;
		}

		// Fork if there is space available in the bitmap(Generation)
		if(isFree = true){
			pid = fork();
			// Forking error
			if(pid < 0){
				perror("ERROR IN oss.c: FORKING FAILED");
				god(1);
				return EXIT_FAILURE;
			}
			// Create Child
			if(pid == 0){
				int execStatus = execl("./child","./child", NULL);
				if(execStatus == -1){
					perror("ERROR IN oss.c: EXECL FAILED TO EXECUTE\n");
					god(1);
					return EXIT_FAILURE;
					
				}
			}
			// In parent
			else{
				// Increment amount of times fork has launched
				forkLaunched++;

				// Set index to taken in bitmap so its not reused
				bitmap[index / 8] |= (1 << (index % 8));

				// Create child process and send its formation to the PCB
				struct Process *child = createChildProc(index, pid);
				// Send to PCB
				moveData(&pcb[index], child);
				// Put process in queue
				enQueue(highPrio, index);

				// Display generation information
				printf("Generating process with PID %d and putting it in queue %d at time %d:%d\n", pcb[index].simPID, pcb[index].prio, timer->sec, timer->nsec);

			}
		}

		// Schedule Processes
		struct QNode nextNode;
		// Determine which queue/prio the process gets
		if(currentQueue == 0){
			nextNode.next = highPrio->front;
		}else if(currentQueue == 1){
			nextNode.next = midPrio->front;
		}else if(currentQueue == 2){
			nextNode.next = lowPrio->front;
		}

		int totProcs = 0;
		float totWaitTime = 0.0;
		struct Queue *tempoQueue = createQueue();
		// Where there are still more nodes
		while(nextNode.next != NULL){
			totProcs++;
			// Increment the timer
			incTimer();

			// Send Msg to child to signify it is their turn
			int childIndex = nextNode.index;
			msgInfo.type = pcb[childIndex].pid;
			msgInfo.index = childIndex;
			msgInfo.childPID = pcb[childIndex].pid;
			msgInfo.prio = pcb[childIndex].prio = currentQueue;
			// Send the message to child
			msgsnd(msgQueID, &msgInfo, (sizeof(struct Msg) - sizeof(long)), 0);

			// Increment timer because we did something
			incTimer();

			// Receive information from child process
			msgrcv(msgQueID, &msgInfo, (sizeof(struct Msg) - sizeof(long)), 1, 0);

			// Print dispatching info
			printf("Dispatching process with PID %d from queue %d at time %d:%d\n", msgInfo.index, msgInfo.childPID, currentQueue, msgInfo.sec, msgInfo.nsec);
			
			incTimer();

			// Second part of dispatch message
			int diffNSec = timer->nsec - msgInfo.nsec;
			printf("total time this dispatch was %d nanoseconds\n", diffNSec);

			// Find out how long the process is being run for
			while(1){
				incTimer();

				// Receuve time from child message by seeing if child has sent message back, will be not -1
				int checkIfDone = msgrcv(msgQueID, &msgInfo, (sizeof(struct Msg) - sizeof(long)), 1, IPC_NOWAIT);
				if(checkIfDone != -1){
					printf("Receving that process with PID %d ran for %d nanoseconds\n", msgInfo.index, msgInfo.burstTime);
					break;
				}
			}

			incTimer();

			// See if child is finished
			
		}
	}

	// Free up shared memeory and list of PIDs
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
	pcb = shmat(pcbID, (void*) 0, 0);
	if(pcb == (void*)-1){
		perror("ERROR IN child.c: FAILED TO ATTACH MEMEORY FOR PCB");
		freeMem();
		exit(EXIT_FAILURE);
	}
}

void incTimer(){
	srand(time(NULL));
	int randNum = (rand() % (1000 + 1));
	timer->nsec += randNum;
	while(timer->nsec >= 1000000000){
		timer->sec++;
		timer->nsec -= 1000000000;
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

void moveData(struct ProcBlock *pcb, struct Process *c){
	pcb->simPID = c->index;
	pcb->pid = c->pid;
	pcb->prio = c->prio;
	pcb->lastBurst = 0;
	pcb->totalBurst = 0;
	pcb->totSysSec = 0;
	pcb->totSysNSec = 0;
	pcb->totWaitSec = 0;
	pcb->totWaitNSec = 0;
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

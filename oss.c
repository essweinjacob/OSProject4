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

	// Generate Bitmap and initialize all values to NULL
	memset(bitmap, '\0', sizeof(bitmap));

	// Variables for Forking
	int index = -1;
	int childDone = 0;
	int status = 0;
	int activeChildren = 0;
	bool exitStatus = false;
	int childLaunched = 0;
	pid_t pid;

	// Initial timer values
	timer->sec = 0;
	timer->nsec = 0;

	// Set up queues
	highPrio = createQueue();
	midPrio = createQueue();
	lowPrio = createQueue();

	// Multi Process Handeling
	bool isOpen = false;		// Checking if place in bit map is open
	int curQue = 0;
	
	// Its forking time
	while(exitStatus == false){
		// Check if place in bit map is open
		isOpen = false;
		int procCount = 0;
		while(1){
			index = (index + 1) % MAX_PROC;
			// Bitmap operations
			uint32_t bit = bitmap[index / 8] & (1 << (index % 8));
			if(bit == 0){
				isOpen = true;
				break;
			}else{
				isOpen = false;
			}

			if(procCount >= MAX_PROC - 1){
				printf("BIT MAP IS FULL\n");
				break;
			}
			procCount++;
		}

		// If bit map is open
		if(isOpen == true){
			// Generate new process
			pid = fork();
			// Forking error
			if(pid < 0){
				perror("ERROR IN oss.c: Failed to fork");
				freeMem();
				return EXIT_FAILURE;
			}
			// Launch child
			else if(pid == 0){
				char * args[] = {"./child", NULL};
				execvp(args[0], args);
			}else{
				// Increment amount of children launched
				childLaunched++;

				// Set index in bitmap to taken
				bitmap[index / 8] |= (1 << (index % 8));

				// Intialize proccess

				// Put process in queue
				enQueue(highPrio, index);

				// Generate Process
				generateProc(index);
			}
		}
		// Scheduleing
		struct QNode next;
		if(curQue == 0){
			next.next = highPrio->front;
		}else if(curQue == 1){
			next.next = midPrio->front;
		}else if(curQue == 2){
			next.next = lowPrio->front;
		}
		// Dispatching
		if((pid = waitpid((pid_t)-1, &status, WNOHANG)) > 0){
			if(WIFEXITED(status)){
				childDone++;
				activeChildren--;
			}
		}
		// Fail safe for fork bombing
		if(activeChildren >= 20){
			god(1);
		}
		if(childDone >= 100){
			exitStatus = true;
		}
	}
	
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

void incTimer(){
	srand(time(NULL));
	timer->nsec += (rand() % (10000 - 100 + 1)) + 100;
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
	prevProcSec = timer->sec;
	prevProcNSec = timer->nsec;
	pcb[index].startTimeSec = prevProcSec;
	pcb[index].startTimeNSec = prevProcNSec;
	pcb[index].inProg = true;
	srand(index);
	
	printf("Generating Process with PID %d and putting it in queue %d at time %d:%d\n", pcb[index].simPID, 0, pcb[index].startTimeSec, pcb[index].startTimeNSec);
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

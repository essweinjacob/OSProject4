#include "shared.h"

// Functions used
void god(int signal);
void timeoutClock();
void freeMem();
void getMsgQueue();
void getClock();
void getSema();
void getPCB();
void addProcess();
void printProcess();
void addTime();

// For list of PIDs
int *listOfPIDS;
int numOfPIDS = 0;
pid_t pid;

// Message Queue Variables
key_t msgQueueKey = -1;
int msgQueueID = -1;
struct Msg msgInfo;
// Child to Parent message queue
key_t cMsgQueueKey = -1;
int cMsgQueueID = -1;
struct Msg cMsgInfo;

// Clock variables
key_t clockKey = -1;
int clockID = -1;
struct Clock *timer;

// Semaphore Variables
key_t semaKey = -1;
int semaID = -1;
struct sembuf semOp;

// PCB Variables
key_t pcbKey = -1;
int pcbID = -1;
struct ProcessControlBlock *pcb;

// Queues
struct Queue *highPrio;
struct Queue *midPrio;
struct Queue *lowPrio;

int main(int argc, int argv[]){
	// Seed random based off of pid
	srand(getpid());
	// Set up all shared memeory
	getMsgQueue();
	getClock();
	getSema();
	getPCB();
	
	// Set bitmap
	int bitmap[MAX_PROC];
	int i;
	for(i = 0; i < MAX_PROC; i++){
		bitmap[i] = 0;
	}

	// Set up signal handeling
	timeoutClock();
	signal(SIGINT, god);
	signal(SIGPROF, god);

	// Initialize Clock
	timer->sec = 0;
	timer->nsec = 0;

	// Variables
	unsigned int index = 0;
	bool timeToLaunch = true;
	int childrenFinished = 0;
	int lastLaunchSec = 0;
	int lastLaunchNSec = 0;
	int childActive = 0;
	listOfPIDS = calloc(MAX_PROC, (sizeof(int)));

	// Create process
	while(1){
		if(bitmap[index] != 1){
			lastLaunchSec = timer->sec;
			lastLaunchNSec = timer->nsec;
			timeToLaunch = false;
			pid = fork();
			// Forking error
			if(pid == -1){
				perror("ERROR IN OSS.C: FORKING FAILURE");	
				god(1);
			}
			if(pid == 0){
				char stringIndex[15];
				sprintf(stringIndex, "%d", index);
				char *args[] = {"./user", stringIndex, NULL};
				execvp(args[0], args);
			}else{
				bitmap[index] = 1;
				childActive++;
				listOfPIDS[numOfPIDS] = pid;
				numOfPIDS++;
				addProcess(index);
				msgInfo.index = index;
				msgInfo.pid = pid;
				msgInfo.prio = 1;

				printf("Generated process with pid: %d [%d] at time %d:%d\n", msgInfo.index, msgInfo.pid, timer->sec, timer->nsec);
			}
		}
		addTime();
		// Send message
		msgsnd(msgQueueID, &msgInfo, (sizeof(struct Msg) - sizeof(long)), 1);
		printf("OSS: Signaling process with PID %d [%d] from queue %d to dispatch at %d:%d\n", msgInfo.index, msgInfo.pid, cMsgInfo.prio, timer->sec, timer->nsec);
		printf("OSS SND ERROR: %s\n", strerror(errno));

		addTime();

		msgrcv(cMsgQueueID, &cMsgInfo, (sizeof(struct Msg) - sizeof(long)), 0, 0);
		//printf("OSS: MSG RCV RESPONCE: %s\n", strerror(errno));
		
		printf("OSS: Dispatching process with PID %d [%d] from queue %d at time %d:%d\n", cMsgInfo.index, cMsgInfo.pid, cMsgInfo.prio, timer->sec, timer->nsec);
		int dispatchSec = timer->sec;
		int dispatchNSec = timer->nsec;

		addTime();
		
		int dispatchTime = ((timer->sec * 1000000000) + timer->nsec) - ((dispatchSec * 1000000000) + dispatchNSec);
		printf("OSS: total time of this dispatch was %d nanoseconds\n", dispatchTime);

		addTime();
		
		msgrcv(cMsgQueueID, &cMsgInfo, (sizeof(struct Msg) - sizeof(long)), 0, 0);
		printf("OSS: receving that process with PID %d [%d] ran for %d nanoseconds\n", cMsgInfo.index, cMsgInfo.pid, cMsgInfo.burstTime);
			
		if(childActive >= 20){
			god(1);
		}
		int childStatus = 0;
		pid_t childPID = waitpid(-1, &childStatus, WNOHANG);
		if(childPID > 0){
			int childIndex = WEXITSTATUS(childStatus);
			bitmap[childIndex] = 0;
			childrenFinished++;
			childActive--;
		}
		
		if(childrenFinished >= 100){
			break;
		}
		addTime();
		if(((timer->sec * 1000000000) + timer->nsec) - ((lastLaunchSec * 1000000000) + lastLaunchNSec)){
			timeToLaunch = true;
		}
	}
	// Destroy all the vidence
	freeMem();

	return EXIT_SUCCESS;
}

void freeMem(){
	shmctl(clockID, IPC_RMID, NULL);
	shmctl(pcbID, IPC_RMID, NULL);
	semctl(semaID, 0, IPC_RMID);
	msgctl(msgQueueID, IPC_RMID, NULL);
	msgctl(cMsgQueueID, IPC_RMID, NULL);
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

void timeoutClock(){
	struct itimerval time1;
	time1.it_value.tv_sec = 3;
	time1.it_value.tv_usec = 0;
	time1.it_interval = time1.it_value;
	signal(SIGALRM, god);
	setitimer(ITIMER_REAL, &time1, NULL);
}

void getMsgQueue(){
	msgQueueKey = ftok("./oss.c", 0);
	if(msgQueueKey == -1){
		perror("ERROR IN OSS.C: FAILED TO GENERATE KEY FOR MESSAGE QUEUE");
		god(1);
	}
	msgQueueID = msgget(msgQueueKey, IPC_CREAT | 0600);
	if(msgQueueID == -1){
		perror("ERROR IN OSS.C: FAILED TO GET KEY FOR MESSAGE QUEUE");
		god(1);
	}

	cMsgQueueKey = ftok("./oss.c", 4);
	if(msgQueueKey == -1){
		perror("ERROR IN OSS.C: FAILED TO GENERATE KEY FOR MESSAGE QUEUE");
		god(1);
	}
	cMsgQueueID = msgget(cMsgQueueKey, IPC_CREAT | 0600);
	if(cMsgQueueKey == -1){
		god(1);
	}
}

void getClock(){
	clockKey = ftok("./oss.c", 1);
	if(clockKey == -1){
		perror("ERROR IN OSS.C: FAILED TO GENERATE KEY FOR MESSAGE QUEUE");
		god(1);
	}
	clockID = shmget(clockKey, sizeof(struct Clock*), 0666 | IPC_CREAT);
	if(clockID == -1){
		perror("ERROR IN OSS.C: FAILED TO GET KEY FOR CLOCK");
		god(1);
	}
	timer = (struct Clock*)shmat(clockID, (void*) 0, 0);
	if(timer == (void*)-1){
		perror("ERROR IN OSS.C: FAILED TO ATTACH CLOCK TO SHAREED MEMEORY");
		god(1);
	}
}

void getSema(){
	semaKey = ftok ("./oss.c", 2);
	if(semaKey == -1){
		perror("ERROR IN OSS.C: FAILED TO GENERATE KEY FOR SEMAPOHRE");
		god(1);
	}
	semaID = semget(semaKey, 1, 0666 | IPC_CREAT);
	if(semaID == -1){
		perror("ERROR IN OSS.C: FAILED TO GET KEY FOR SEMAPHORE");
		god(1);
	}
	semctl(semaID, 0, SETVAL, 1);
}

void getPCB(){
	pcbKey = ftok("./oss.c", 3);
	if(pcbKey == -1){
		perror("ERROR IN OSS.C: FAILED TO GENERATE KEY FOR PCB");
		god(1);
	}
	size_t procTableSize = sizeof(struct ProcessControlBlock) * MAX_PROC;
	pcbID = shmget(pcbKey, procTableSize, 0666 | IPC_CREAT);
	if(pcbID == -1){
		perror("ERROR IN OSS.C: FAILED TO GET KEY FOR PCB");
		god(1);
	}
	pcb = shmat(pcbID, (void*) 0, 0);
	if(pcb == (void*)-1){
		perror("ERROR IN OSS.C: FAILED TO ATTACH MEMEORY FOR PCB");
		god(1);
	}
}

void addProcess(int index){
	pcb[index].index = index;
	pcb[index].pid = pid;
	pcb[index].startSec = timer->sec;
	pcb[index].startNSec = timer->nsec;
	pcb[index].hasStarted = false;
	pcb[index].isFinished = false;
}
 
void printProcess(int index){
	printf("Index: %d\nPID: %d\nStart Time %d:%d\n", pcb[index].index, pcb[index].pid, pcb[index].startSec, pcb[index].startNSec);
}

void addTime(){
	timer->nsec += (rand() % (1000 + 1));
	while(timer->nsec >= 1000000000){
		timer->sec++;
		timer->nsec -= 1000000000;
	}
}

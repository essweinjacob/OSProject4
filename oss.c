#include "shared.h"

#define MAX_PROCESS 18

void god(int signal);
void incTimer();
void generateProc(int index);

// Pid list info
int *listOfPIDS;
int numOfPIDS = 0;
// PCB
struct ProcBlock *procs;

// Fake time stuff
struct Clock timer;

int main(int argc, int argv[]){
	// Creation of process control table
	key_t tableKey = ftok("./oss.c", 1);
	if(tableKey == -1){
		perror("ERROR IN oss.c: FAILED TO MAKE KEY FOR PROCESS CONTROL TABLE");
		return EXIT_FAILURE;
	}
	size_t procTableSize = sizeof(struct ProcBlock) * MAX_PROCESS;
	int tableID = shmget(tableKey, procTableSize, 0666 | IPC_CREAT);
	if(tableID == -1){
		perror("ERROR IN oss.c: FAILED TO GET SHARED MEMORY ID FOR PROCESS CONTROL TABLE");
		return EXIT_FAILURE;
	}
	procs = (struct ProcBlock*)shmat(tableID, (void*) 0, 0);
	if(procs == (void*)-1){
		perror("ERROR IN oss.c: FAILED TO ATTACH MEMORY FOR PCB");
		return EXIT_FAILURE;
	}

	// Variables for forking
	int exitStatus = 0;
	int activeProcesses = 1;	// Starts at 1 since parent is an active process
	int childDone = 0;		// Max processes to be done
	int exitCount = 0;
	pid_t pid;
	int status;
	listOfPIDS = calloc(101, (sizeof(int)));
	// Set fake clock/timer initial
	timer.sec = 0;
	timer.nsec = 0;
	
	while(exitStatus == 0){
		if(activeProcesses < MAX_PROCESS && childDone < 100 && exitStatus == 0 && exitCount < 100){
			pid = fork();
			// Fork error
			if(pid < 0){
				perror("Forking error");
				return EXIT_FAILURE;
			// Launch child
			}else if(pid == 0){
				char *args[] = {"./child", NULL};
				execvp(args[0], args);
			}
			generateProc(childDone);
			listOfPIDS[numOfPIDS] = pid;
			numOfPIDS++;
			childDone++;
			activeProcesses++;
		}
		// Check if child has ended
		if((pid = waitpid((pid_t)-1, &status, WNOHANG)) > 0){
			if(WIFEXITED(status)){
				// Process Exits child
				exitCount++;
				activeProcesses--;
				if(exitCount == 100){
					exitStatus = 1;
				}
			}
		}
		incTimer();
		//printf("activeChildren: %d\n", activeChildren);
		// Absolute fail safe for child processes
	}
	printf("Left forking\n");

	// Free up memory and shared segs and semaphores
	shmctl(tableID, IPC_RMID, NULL);
	free(listOfPIDS);
	
}

void god(int signal){
	int i;
	for(i = 0; i < numOfPIDS; i++){
		kill(listOfPIDS[i], SIGTERM);
	}
	printf("GOD HAS BEEN CALLED AND THE RAPTURE HAS BEGUN. SOON THERE WILL BE NOTHING\n");
	free(listOfPIDS);
	kill(getpid(), SIGTERM);
}

void incTimer(){
	timer.nsec += 10000;
	while(timer.nsec >= 1000000000){
		timer.sec++;
		timer.nsec -= 1000000000;
	}
}

void generateProc(int index){
	procs[index].simPID = index;
	procs[index].pid = getpid();
	procs[index].startTimeSec = timer.sec;
	procs[index].startTimeNSec = timer.nsec;
	printf("Generating Process with PID %d and putting it in queue %d at time %d:%d\n", procs[index].simPID, 0, procs[index].startTimeSec, procs[index].startTimeNSec);
}

void dispatchProc(int index){
	int dispatchTimeSec = timer.sec;
	int dispatchTimeNSec = timer.nsec;
	printf("Dispatching process with PID %d from queue at time %d:%d\n", procs[index].simPID, dispatchTimeSec, dispatchTimeNSec);
	/*
	 * Stuff Happens
	 */
	printf("Total time this dispatch took was %d seconds and %d nanoseconds\n", timer.sec - dispatchTimeSec, timer.nsec - dispatchTimeNSec);
}

void recevingProc(int index){
	int receiveTimeSec;
	int receiveTimeNSec;
	printf("Receving that process with PID %d ran for %d seconds and %d nanoseconds\n", procs[index].simPID, timer.sec, timer.nsec);
}

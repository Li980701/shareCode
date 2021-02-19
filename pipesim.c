#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*  CITS2002 Project 1 2020
    Name:                Brayden(LI, BO)， Elon, Jeffery,
    Student number(s):   Team leader 22649867
 */

//  MAXIMUM NUMBER OF PROCESSES OUR SYSTEM SUPPORTS (PID=1..20)
#define MAX_PROCESSES 20

//  MAXIMUM NUMBER OF SYSTEM-CALLS EVER MADE BY ANY PROCESS
#define MAX_SYSCALLS_PER_PROCESS 50

//  MAXIMUM NUMBER OF PIPES THAT ANY SINGLE PROCESS CAN HAVE OPEN (0..9)
#define MAX_PIPE_DESCRIPTORS_PER_PROCESS 10

//  TIME TAKEN TO SWITCH ANY PROCESS FROM ONE STATE TO ANOTHER
#define USECS_TO_change_process_status 5

//  TIME TAKEN TO TRANSFER ONE BYTE TO/FROM A PIPE
#define USECS_PER_BYTE_TRANSFERED 1

//  ---------------------------------------------------------------------

//  YOUR DATA STRUCTURES, VARIABLES, AND FUNCTIONS SHOULD BE ADDED HERE:

int timetaken = 0;
enum SYS_CALL
{
	COMPUTE,
	SLEEP,
	EXIT,
	FORK,
	WAIT,
	PIPE,
	WRITEPIPE,
	READPIPE
};
enum PROC_STATE
{
	NOT_EXIST,
	RUNNING,
	READY,
	SLEEPING,
	WAITING,
	READBLOCKED,
	WRITEBLOCKED
};

struct syscall_s
{
	enum SYS_CALL cmd;
	int arg1;
	int arg2;
} syscall_t;

struct proc_s
{
	enum PROC_STATE state; // init is NOT_EXIST
	int curr_sys_call;	   // init is zero
	int wakeup_time;	   // for SLEEPING state
	int wait_pid;		   // for WAITING state
	int total_work_left;   // FOR ALL state, init is zero
	int counter;		   // FOR RUNNING state
	int total_sys_call;
	struct syscall_s sys_call[MAX_SYSCALLS_PER_PROCESS];
	int is_pipe; // for PIPE state, 1: reader, 2: writer
	int pipe_id; // for WRITEPIPE and READPIPE state
} proc_t;

struct proc_s proc[MAX_PROCESSES + 1]; // don't use proc[0]
int timequantum;
int pipesize;
int queue[MAX_PROCESSES + 2];
int curr_proc = 0; // current proc, 0 means does not exist

/*
Initialize process array with each value at zero
Each process state starts with NOT_EXIST
*/

void initial_proc()
{
	for (int i = 1; i <= MAX_PROCESSES; i++)
	{
		proc[i].state = NOT_EXIST;
		proc[i].curr_sys_call = 0;
		proc[i].total_sys_call = 0;
		proc[i].total_work_left = 0;
		proc[i].is_pipe = 0;
	}
}

/*
Define QUEUE head and QUEUE tail
Ready QUEUE is a block queue
Four functions provided with ready QUEUE, isEmpty(), push(), pop(), initial() Because this queue is not dynamic and has static length with MAX_PROCESSES + 2 
I designated slightly different algorithm to store value in it
In the push function of this ready queue, if queue is at initial stage, the first varable will be store in the varaibles called QUEUE_HEAD and QUEUE_TAIL. After that, next variable will be add at the position after first variable
In the pop function, each time remove a varaible will cause the pointer(QUEUE_HEAD) to move to next position then remove the variable at that position and make variable at last position be zero.
Each position in the QUEUE will be initialized with 0
*/
#define QUEUE_HEAD queue[0] // queue[0] point to the start

#define QUEUE_TAIL queue[MAX_PROCESSES + 1] //queue[MAX_PROCESSES+1] points to the end

#define PIPE_READER 1

#define PIPE_WRITER 2

int pipe_owner[MAX_PIPE_DESCRIPTORS_PER_PROCESS]; // pipe owner, 0: none, 1: reader, 2: writer
int pipe_left[MAX_PIPE_DESCRIPTORS_PER_PROCESS];

int queue_is_empty()
{
	return QUEUE_HEAD == 0;
}

void queue_push(int pid)
{
	if (!QUEUE_HEAD)
	{
		QUEUE_HEAD = pid;
		QUEUE_TAIL = pid;
	}
	else
	{
		queue[QUEUE_TAIL] = pid;
		QUEUE_TAIL = pid;
	}
}

int queue_pop()
{
	int result = QUEUE_HEAD;
	QUEUE_HEAD = queue[QUEUE_HEAD];
	queue[result] = 0;
	return result;
}

void initial_queue()
{ // Intialized with 0
	for (int i = 1; i <= MAX_PROCESSES; i++)
	{
		queue[i] = 0;
	}
}

/*
Function change_process_status is aimming to change current status of a PID into a new assignated one(parameter)
enum function can return an interge value to help me track down the current process of a PID
If we need to change the status of a PID into ready, algorithm will push this PID into ready QUEUE
If we need to change the status of a PID into running, we need to store that PID and let algorithm know what process we are working on
If the state will need to change is waiting or sleeping...... we need to change curr_proc to initial stage and work on next one
Each time algorithm run this function, the timetaken will add USECS_TO_change_process_status
*/

void change_process_status(int pid, enum PROC_STATE state)
{
	char name[7][32] = {"NOT_EXIST", "RUNNING", "READY", "SLEEPING", "WAITING", "READBLOCKED", "WRITEBLOCKED"};
	printf("[%d]  %s -> %s\n", pid, name[proc[pid].state], name[state]);
	proc[pid].state = state;
	if (state == READY)
		queue_push(pid);

	if (state == RUNNING)
	{
		curr_proc = pid; //To record which process needs to run
	}
	else
	{
		curr_proc = 0;
	}

	timetaken += USECS_TO_change_process_status;
}

/*
Function check_sleep_proc is aimming to check if a PID's sleeping status is finished
wakeup_time is aimming to record how long dose a process need to sleep
If a process has status is sleeping but if its wake_time is samller than timetaken which means its sleeping time is over
In each simulation we need to run this function, in case any process is ready to run
*/

void check_sleep_proc()
{
	for (int i = 1; i <= MAX_PROCESSES; i++)
	{
		if (proc[i].state == SLEEPING &&
			proc[i].wakeup_time <= timetaken)
		{ //finish sleeping
			change_process_status(i, READY);
			timetaken = timetaken + USECS_TO_change_process_status;
		}
	}
}

/*
Function check_wait_proc is aimming to check if a PID's waiting time is over
Similar to Function check_sleep_proc
A wait_pid is recording which porcess is waiting 
*/

void check_wait_proc()
{
	for (int i = 1; i <= MAX_PROCESSES; i++)
	{
		if (proc[i].state == WAITING &&
			proc[proc[i].wait_pid].state == NOT_EXIST)
		{ //finish waiting
			change_process_status(i, READY);
		}
	}
}

/*
Function initial_first_proc is aimming to create the first process
The first process has PID 1
State of this process is running
*/

void initial_first_proc()
{
	curr_proc = 1;
	proc[1].state = RUNNING;
}

/*
Function simulate_end is aimming to check if simulation is over
If all process in array proc have state NOT_EXIST which means simulation is over, otherwise we need to keep simulate
*/
int simulate_end()
{
	for (int i = 1; i <= MAX_PROCESSES; i++)
	{
		if (proc[i].state != NOT_EXIST)
		{
			return 0;
		}
	}
	return 1;
}

/*
Function Check_PIPE is aimming to change the state of current pipe owner
Checking how many work left to do for both pipe writer and pipe reader
To store current pipe onwer into ready queue
*/

void check_pipe()
{
	for (int i = 1; i <= MAX_PROCESSES; i++)
	{
		if (proc[i].state == READBLOCKED &&
			pipe_owner[proc[i].pipe_id] == PIPE_READER)
		{ //finish waiting
			proc[i].counter = pipe_left[proc[i].pipe_id];
			change_process_status(i, READY);
		}
		if (proc[i].state == WRITEBLOCKED &&
			pipe_owner[proc[i].pipe_id] == PIPE_WRITER)
		{ //finish waiting
			proc[i].counter = pipesize - pipe_left[proc[i].pipe_id];
			change_process_status(i, READY);
		}
	}
}

/*
Most important function simulate how process runs in CPU
By using switch to check which system call is used by a process
*/

void simulate()
{
	initial_first_proc();
	initial_queue();
	while (!simulate_end())
	{
		check_sleep_proc();
		check_wait_proc();
		check_pipe();
		if (!curr_proc)
		{
			if (!queue_is_empty())
			{										 // ready queue is not empty
				int pid = queue_pop();				 // find a process
				change_process_status(pid, RUNNING); // run it
			}
		}
		if (curr_proc)
		{ // run process
			if (proc[curr_proc].total_work_left == 0)
			{
				if (proc[curr_proc].is_pipe == PIPE_READER)
				{ // Current process is pipe reader
					proc[curr_proc].is_pipe = 0;
					if (pipe_left[proc[curr_proc].pipe_id] == 0)
					{
						pipe_owner[proc[curr_proc].pipe_id] = PIPE_WRITER; // Change onwer
					}
					change_process_status(curr_proc, READY);
				}
				else if (proc[curr_proc].is_pipe == PIPE_WRITER)
				{ // Current process is pipe writer
					proc[curr_proc].is_pipe = 0;
					pipe_owner[proc[curr_proc].pipe_id] = PIPE_READER; // Change owner
					change_process_status(curr_proc, READY);
				}
				else
				{																					 // need another system call
					struct syscall_s sc = proc[curr_proc].sys_call[proc[curr_proc].curr_sys_call++]; // variable sc is recording the current system call after recording it moving to next position to keep tracking next system call
					switch (sc.cmd)
					{
					case COMPUTE: // System call COMPUTE
						printf("[%d] compute %d\n", curr_proc, sc.arg1);
						proc[curr_proc].total_work_left = sc.arg1; // How many work need to finish
						proc[curr_proc].counter = timequantum;	   // How many can cpu do once a time
						change_process_status(curr_proc, READY);   // Change state to ready and prepare to run
						break;
					case FORK: // System call FORK
						printf("[%d] fock %d\n", curr_proc, sc.arg1);
						int pid = curr_proc;				   // Record parent PID
						change_process_status(sc.arg1, READY); // Parent waits for Child process to run first(Ready)
						change_process_status(pid, READY);	   // Parent Run(ready) after Child . Parent process from Running to Ready
						break;
					case WAIT: // System call WAIT
						printf("[%d] wait %d\n", curr_proc, sc.arg1);
						proc[curr_proc].wait_pid = sc.arg1;		   // How long this process need to wait until its children process finished
						change_process_status(curr_proc, WAITING); // Change process into WAITING
						break;
					case SLEEP: // System call SLEEP
						printf("[%d] sleep %d\n", curr_proc, sc.arg1);
						proc[curr_proc].wakeup_time = timetaken + sc.arg1;
						printf("[%d] sleeping times is %d\n", curr_proc, proc[curr_proc].wakeup_time); // Record how long this process can wake. We need to put timetaken in, becasue the first system call may not be sleep
						change_process_status(curr_proc, SLEEPING);									   // Change process into SLEEPING
						break;
					case EXIT: // System call Exit
						printf("[%d] exit\n", curr_proc);
						change_process_status(curr_proc, NOT_EXIST); // Change process into EXIT
						break;
					case PIPE: // System call PIPE
						printf("[%d] pipe pipedescriptor %i\n", curr_proc, sc.arg1);
						pipe_owner[sc.arg1] = PIPE_WRITER;
						pipe_left[sc.arg1] = 0; // reset pipe
						change_process_status(curr_proc, READY);
						break;
					case WRITEPIPE: // System call write pipe
						printf("[%d] writepipe in %d %d\n", curr_proc, sc.arg1, sc.arg2);
						proc[curr_proc].is_pipe = PIPE_WRITER;					 // Current process id is pipe writer
						proc[curr_proc].pipe_id = sc.arg1;						 // Assgin current pipe descriptor to current pipe descriptor
						proc[curr_proc].total_work_left = sc.arg2;				 // Store how many work left to do of this process id
						proc[curr_proc].counter = pipesize - pipe_left[sc.arg1]; // In case, pipe writer writes several times in this pipe
						break;
					case READPIPE: // System call read pipe
						printf("[%d] readpipe from %d %d\n", curr_proc, sc.arg1, sc.arg2);
						proc[curr_proc].is_pipe = PIPE_READER;	   // Current porcess id is pipe reader
						proc[curr_proc].pipe_id = sc.arg1;		   // Assgin current pipe descriptor to current pipe id
						proc[curr_proc].total_work_left = sc.arg2; // How many work left to do
						proc[curr_proc].counter = pipe_left[sc.arg1];
						if (pipe_left[proc[curr_proc].pipe_id] == 0)
						{ // Current pipe does have not anything can be read
							change_process_status(curr_proc, READBLOCKED);
							break;
						}
					}
				}
			}
			else
			{ // One process has not done yet

				if (proc[curr_proc].counter == 0)
				{ // Assign work to cpu how long can a process run on cpu
					printf("[%d] give up cpu %d %d \n", curr_proc, proc[curr_proc].total_work_left, proc[curr_proc].counter);
					switch (proc[curr_proc].is_pipe)
					{
					case PIPE_READER:									   // Finish writing
						pipe_owner[proc[curr_proc].pipe_id] = PIPE_WRITER; // Change pipe owner to pipe reader
						change_process_status(curr_proc, READBLOCKED);
						break;
					case PIPE_WRITER:									   // Finish reading
						pipe_owner[proc[curr_proc].pipe_id] = PIPE_READER; // Change pipe owner to pipe writer
						change_process_status(curr_proc, WRITEBLOCKED);
						break;
					default:
						proc[curr_proc].counter = timequantum; // Syscall without pipe call
						printf("normal change\n");
						change_process_status(curr_proc, READY); // Change process to ready and push this process into Ready QUEUE
					}
				}
				else
				{
					proc[curr_proc].total_work_left--; // Each time minus one
					proc[curr_proc].counter--;		   // Each time minus one until to zero then in upper if sentence change it into ready
					timetaken++;					   // count time
					switch (proc[curr_proc].is_pipe)
					{
					case PIPE_READER:
						pipe_left[proc[curr_proc].pipe_id]--; // Reading
						break;
					case PIPE_WRITER:
						pipe_left[proc[curr_proc].pipe_id]++; // Writing
						break;
					}
				}
			}
		}
		else
		{
			// There is no process which is ready
			timetaken++; // But there could be a waiting or sleeping process
		}
	}
}

//  ---------------------------------------------------------------------
//  FUNCTIONS TO VALIDATE FIELDS IN EACH eventfile - NO NEED TO MODIFY
int check_PID(char word[], int lc)
{
	int PID = atoi(word);

	if (PID <= 0 || PID > MAX_PROCESSES)
	{
		printf("invalid PID '%s', line %i\n", word, lc);
		exit(EXIT_FAILURE);
	}
	return PID;
}

int check_microseconds(char word[], int lc)
{
	int usecs = atoi(word);

	if (usecs <= 0)
	{
		printf("invalid microseconds '%s', line %i\n", word, lc);
		exit(EXIT_FAILURE);
	}
	return usecs;
}

int check_descriptor(char word[], int lc)
{
	int pd = atoi(word);

	if (pd < 0 || pd >= MAX_PIPE_DESCRIPTORS_PER_PROCESS)
	{
		printf("invalid pipe descriptor '%s', line %i\n", word, lc);
		exit(EXIT_FAILURE);
	}
	return pd;
}

int check_bytes(char word[], int lc)
{
	int nbytes = atoi(word);

	if (nbytes <= 0)
	{
		printf("invalid number of bytes '%s', line %i\n", word, lc);
		exit(EXIT_FAILURE);
	}
	return nbytes;
}

//  parse_eventfile() READS AND VALIDATES THE FILE'S CONTENTS
//  YOU NEED TO STORE ITS VALUES INTO YOUR OWN DATA-STRUCTURES AND VARIABLES
void parse_eventfile(char program[], char eventfile[])
{
#define LINELEN 100
#define WORDLEN 20
#define CHAR_COMMENT '#'

	//  ATTEMPT TO OPEN OUR EVENTFILE, REPORTING AN ERROR IF WE CAN'T
	FILE *fp = fopen(eventfile, "r");

	if (fp == NULL)
	{
		printf("%s: unable to open '%s'\n", program, eventfile);
		exit(EXIT_FAILURE);
	}

	char line[LINELEN], words[4][WORDLEN];
	int lc = 0;

	//  READ EACH LINE FROM THE EVENTFILE, UNTIL WE REACH THE END-OF-FILE
	while (fgets(line, sizeof line, fp) != NULL)
	{
		++lc;

		//  COMMENT LINES ARE SIMPLY SKIPPED
		if (line[0] == CHAR_COMMENT)
		{
			continue;
		}

		//  ATTEMPT TO BREAK EACH LINE INTO A NUMBER OF WORDS, USING sscanf()
		int nwords = sscanf(line, "%19s %19s %19s %19s",
							words[0], words[1], words[2], words[3]);

		//  WE WILL SIMPLY IGNORE ANY LINE WITHOUT ANY WORDS
		if (nwords <= 0)
		{
			continue;
		}

		//  ENSURE THAT THIS LINE'S PID IS VALID
		int thisPID = check_PID(words[0], lc);

		//  OTHER VALUES ON (SOME) LINES
		int otherPID, nbytes, usecs, pipedesc;

		//  IDENTIFY LINES RECORDING SYSTEM-CALLS AND THEIR OTHER VALUES
		//  THIS FUNCTION ONLY CHECKS INPUT;  YOU WILL NEED TO STORE THE VALUES
		/*
Each time just store the values into array proc according to the specific system call
*/
		if (nwords == 3 && strcmp(words[1], "compute") == 0)
		{
			usecs = check_microseconds(words[2], lc);
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].cmd = COMPUTE;
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].arg1 = usecs;
			proc[thisPID].total_sys_call++;
		}
		else if (nwords == 3 && strcmp(words[1], "sleep") == 0)
		{
			usecs = check_microseconds(words[2], lc);
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].cmd = SLEEP;
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].arg1 = usecs;
			proc[thisPID].total_sys_call++;
		}
		else if (nwords == 2 && strcmp(words[1], "exit") == 0)
		{
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].cmd = EXIT;
			proc[thisPID].total_sys_call++;
		}
		else if (nwords == 3 && strcmp(words[1], "fork") == 0)
		{
			otherPID = check_PID(words[2], lc);
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].cmd = FORK;
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].arg1 = otherPID;
			proc[thisPID].total_sys_call++;
		}
		else if (nwords == 3 && strcmp(words[1], "wait") == 0)
		{
			otherPID = check_PID(words[2], lc);
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].cmd = WAIT;
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].arg1 = otherPID;
			proc[thisPID].total_sys_call++;
		}
		else if (nwords == 3 && strcmp(words[1], "pipe") == 0)
		{
			pipedesc = check_descriptor(words[2], lc);
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].cmd = PIPE;
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].arg1 = pipedesc;
			proc[thisPID].total_sys_call++;
		}
		else if (nwords == 4 && strcmp(words[1], "writepipe") == 0)
		{
			pipedesc = check_descriptor(words[2], lc);
			nbytes = check_bytes(words[3], lc);
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].cmd = WRITEPIPE;
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].arg1 = pipedesc;
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].arg2 = nbytes;
			proc[thisPID].total_sys_call++;
		}
		else if (nwords == 4 && strcmp(words[1], "readpipe") == 0)
		{
			pipedesc = check_descriptor(words[2], lc);
			nbytes = check_bytes(words[3], lc);
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].cmd = READPIPE;
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].arg1 = pipedesc;
			proc[thisPID].sys_call[proc[thisPID].total_sys_call].arg2 = nbytes;
			proc[thisPID].total_sys_call++;
		}
		//  UNRECOGNISED LINE
		else
		{
			printf("%s: line %i of '%s' is unrecognized\n", program, lc, eventfile);
			exit(EXIT_FAILURE);
		}
	}
	fclose(fp);

#undef LINELEN
#undef WORDLEN
#undef CHAR_COMMENT
}

//  ---------------------------------------------------------------------

//  CHECK THE COMMAND-LINE ARGUMENTS, CALL parse_eventfile(), RUN SIMULATION
int main(int argc, char *argv[])
{
	if (argc != 4)
	{ // If argument is legal
		printf("main: invalid arguments\n");
		exit(EXIT_FAILURE);
	}
	initial_proc();
	parse_eventfile("parse_eventfile", argv[1]);
	timequantum = atoi(argv[2]);
	pipesize = atoi(argv[3]);
	simulate();
	printf("timetaken %i\n", timetaken);
	return 0;
}

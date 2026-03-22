#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <semaphore.h>
#include<mqueue.h>
#include<string.h>


void handle_sigusr1(int sig) 
{
      printf("Received SIGUSR1 signal\n");
      printf("Received overload warning from controller\n");
}

void handle_sigint(int sig) 
{
      printf("\nSIGINT received (Ctrl+C pressed)\n");
}

void handler(int sig) 
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("Child %d finished\n", pid);
    }
}

//Structure for shared memory
typedef struct 
{
	int available_slots;
	int power_usage;
	int vehicles_charging;
} SharedData;
	
int main() 
{
	
	//signal for overload warning , when command kill -SIGUSR1 parent pid , it gets called
	
	signal(SIGUSR1, handle_sigusr1);
	
	//when ctrl+c pressed it gets called
	
	signal(SIGINT, handle_sigint);
	signal(SIGCHLD, handler);
	
	printf("Parent Process ID: %d\n", getpid());

	//while(1)
	{
	printf("Waiting for signal...\n");
	sleep(1);
	}
	
	
	//Creating 3 child processes with execv
	pid_t pid1 , pid2 , pid3 ;	
	
	
	pid1 = fork();
	char *args1[] = {"Generator",NULL};
	if(pid1==0)
	{
		execv("./Generator", args1);
		perror("execv for child1 failed\n");
		exit(1);
	}
	
	pid2 = fork();
	char *args2[] = { "Controller",NULL};
	if(pid2==0)
	{
		execv("./Controller", args2);
		perror("execv for child2 failed\n");
		exit(1);
	}
	
	pid3 = fork();
	char *args3[] = { "Billing",NULL };
	if(pid3==0)
	{
		execv("./Billing",args3);
		perror("execv for child3 failed\n");
		exit(1);
	}
	
	
	//CREATING SHARED MEMORY ---------------------------------------------------------------------------------------------------------------------------------------------

	int fd;                 //Shared memory file descriptor
	SharedData *ptr;       //POinter to shared memory variable
	
	// Create shared memory object
	fd = shm_open("/ev_charging", O_CREAT | O_RDWR, 0666);

	// Set size
	ftruncate(fd, sizeof(SharedData));
	
	// Map into memory
	ptr = mmap(NULL, sizeof(SharedData),
	PROT_READ | PROT_WRITE,
	MAP_SHARED, fd, 0);

	if (ptr == MAP_FAILED) {
	perror("mmap failed");
	exit(1);
	}

	// Initialize values
	ptr->available_slots = 15;
	ptr->power_usage = 0;
	ptr->vehicles_charging = 0;

	printf("Shared memory initialized\n");
	
	//Semaphore for shared memory creation--------------------------------------------------------------------------------------------------------------------------------------------------------------
      
        sem_t *sem = sem_open("/mysem", O_CREAT, 0666, 1);
        if (sem == SEM_FAILED) 
        {
        perror("sem_open failed");
        exit(1);
        }else
        {
        printf("Semaphore Created\n");
        }
        
        //Message queue creation----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        mqd_t mq;
        struct mq_attr attr;
        
        attr.mq_flags = 0;
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = 50;
        attr.mq_curmsgs = 0;

        mq = mq_open("/myqueue", O_CREAT | O_RDWR, 0777, &attr);

        if (mq == (mqd_t)-1) {
        perror("mq_open failed");
        exit(1);
        }

        printf("Message queue created successfully\n");
        mq_close(mq);
	        

        return 0;
}

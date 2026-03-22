#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<semaphore.h>
#include<mqueue.h>

//Generates New Vehicle ID and Power requested
//Sends this data to Controller process via message queue


typedef struct {
    int vid;
    int power;
    int time;
} VehicleMsg;

void* Varrival(void* arg)
{
    VehicleMsg msg;
    for(int i=0; i<4;i++)
    {
    msg.vid   = (rand() % 1001) + 1000;  // 1000–2000
    msg.power = rand() % 51;             // 0–50
    msg.time  = rand() % 101;            // 0–100

    printf("Vehicle ID: %d\n", msg.vid);
    printf("Requested Power: %d kW\n", msg.power);
    printf("Charging Time: %d min\n", msg.time);

    mqd_t mq = mq_open("/myqueue", O_WRONLY);

    if (mq == (mqd_t)-1) {
        perror("mq_open failed");
        exit(1);
    }

    mq_send(mq, (char*)&msg, sizeof(msg), 1);
    

    printf("Message sent successfully\n");

    mq_close(mq);
    sleep(50);
    }
    return NULL;
}

//Structure to access shared memory

typedef struct
{
	int available_slots;
	int power_usage;
	int vehicles_charging;
} SharedData;

//Display Status of Charging System

void* Vstatus(void* arg)
{
	int fd;
	SharedData *ptr;
	sem_t *sem;
        sem = sem_open("/mysem", 0);

	fd = shm_open("/ev_charging", O_RDWR, 0666);
        
        sem_wait(sem);    //Semaphore unlock
	ptr = mmap(NULL, sizeof(SharedData),PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
        sem_post(sem);    //Semaphore lock
        
        sleep(1);
	while (1) {
	//system("clear");  // clears terminal

	printf("=====================================\n");
	printf("   ⚡ EV CHARGING STATION MONITOR ⚡\n");
	printf("=====================================\n\n");

	printf("🚗 Vehicles Charging : %d\n", ptr->vehicles_charging);
	printf("🅿️  Available Slots  : %d\n", ptr->available_slots);
	printf("🔌 Power Usage       : %d kW\n\n", ptr->power_usage);

	printf("=====================================\n");

	usleep(500000); // refresh every 0.5 sec
	}
	
	sem_close(sem);
	return NULL;
}

int main()
{
    pthread_t tid1 , tid2;
    srand(time(NULL));

    pthread_create(&tid1, NULL, Varrival, NULL);
    pthread_create(&tid2, NULL, Vstatus, NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    printf("Main thread finished\n");

    return 0;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

// Shared memory structure
typedef struct {
    int available_slots;
    int power_usage;
    int vehicles_charging;
} SharedData;

// Message queue structure
typedef struct {
    int vid;
    int power;
    int time;
} VehicleMsg;

// FIFO structure (for billing)  ← FIX 1: was missing "typedef struct {"
typedef struct {
    int vid;
    int power;
    int time;
} BillingMsg;

// Global variables
SharedData *ptr;
sem_t      *sem;

#define BILLING_FIFO "/tmp/billing_pipe"
#define MQ_NAME      "/myqueue"
#define SHM_NAME     "/ev_charging"
#define SEM_NAME     "/mysem"
#define MQ_MSG_SIZE  50


// Thread to simulate charging, then send billing info via FIFO
void* charge_vehicle(void* arg)
{
    VehicleMsg *msg = (VehicleMsg*)arg;

    printf("[Controller] Vehicle %d is CHARGING  (power=%d kW, time=%d min)\n",
           msg->vid, msg->power, msg->time);

    sleep(msg->time);   // simulate charging duration

    // Update shared memory
    sem_wait(sem);
    ptr->available_slots++;
    ptr->vehicles_charging--;
    ptr->power_usage -= msg->power;
    sem_post(sem);

    printf("[Controller] Vehicle %d finished charging\n", msg->vid);

    // Send billing info to C3 via named pipe
    BillingMsg bill;
    bill.vid   = msg->vid;
    bill.power = msg->power;
    bill.time  = msg->time;

    int fd = open(BILLING_FIFO, O_WRONLY);
    if (fd == -1)
    {
        perror("[Controller] billing pipe open failed");
    }
    else
    {
        // FIX 2: added braces so close(fd) always runs, not just on write error
        if (write(fd, &bill, sizeof(bill)) == -1)
        {
            perror("[Controller] billing pipe write failed");
        }
        close(fd);
        printf("[Controller] Billing info sent for Vehicle %d\n", msg->vid);
    }

    free(msg);
    return NULL;
}


int main()
{
    // Create billing FIFO
    if (mkfifo(BILLING_FIFO, 0666) == -1) {
        perror("[Controller] mkfifo note");
    } else {
        printf("[Controller] Billing FIFO created: %s\n", BILLING_FIFO);
    }

    // Open message queue
    mqd_t mq = mq_open(MQ_NAME, O_RDONLY);
    if (mq == (mqd_t)-1) {
        perror("[Controller] mq_open failed");
        exit(1);
    }
    printf("[Controller] Message queue opened\n");

    // Open shared memory
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        perror("[Controller] shm_open failed");
        exit(1);
    }

    ptr = mmap(NULL, sizeof(SharedData),
               PROT_READ | PROT_WRITE,
               MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("[Controller] mmap failed");
        exit(1);
    }
    close(fd);  // fd no longer needed after mmap
    printf("[Controller] Shared memory mapped\n");

    // Open semaphore
    sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("[Controller] sem_open failed");
        exit(1);
    }
    printf("[Controller] Semaphore opened\n");

    // Main receive loop
    printf("[Controller] Waiting for vehicles...\n");

    while (1)
    {
        char buf[MQ_MSG_SIZE];
        unsigned int priority;

        // Receive raw bytes (buffer must be >= mq_msgsize)
        ssize_t bytes = mq_receive(mq, buf, MQ_MSG_SIZE, &priority);
        if (bytes == -1) {
            perror("[Controller] mq_receive failed");
            continue;
        }

        // Copy into struct
        VehicleMsg msg;
        memcpy(&msg, buf, sizeof(VehicleMsg));

        printf("\n[Controller] Vehicle %d arrived | power=%d kW | time=%d min\n",
               msg.vid, msg.power, msg.time);

        sem_wait(sem);

        if (ptr->available_slots > 0)
        {
            ptr->available_slots--;
            ptr->vehicles_charging++;
            ptr->power_usage += msg.power;
            sem_post(sem);

            // Create a thread to handle the vehicle
            pthread_t tid;
            VehicleMsg *new_msg = malloc(sizeof(VehicleMsg));
            if (!new_msg) {
                perror("[Controller] malloc failed");
                continue;
            }
            *new_msg = msg;

            pthread_create(&tid, NULL, charge_vehicle, new_msg);
            pthread_detach(tid);
        }
        else
        {
            printf("[Controller] No slots available for Vehicle %d — rejected\n", msg.vid);
            sem_post(sem);
        }
    }

    // Cleanup
    mq_close(mq);
    sem_close(sem);
    munmap(ptr, sizeof(SharedData));

    return 0;
}

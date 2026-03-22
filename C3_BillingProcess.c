#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

// Billing message structure ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct {
    int vid;
    int power;
    int time;
} BillingMsg;

#define RATE_PER_KWH  0.05f       // cost per kW per minute
#define LOG_FILE      "billing_log.txt"

void log_billing(BillingMsg *bill, float cost)
{
    FILE *f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        perror("fopen failed");
        return;
    }

    // Timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    fprintf(f, "[%s] Vehicle ID: %d | Power: %d kW | Time: %d min | Cost: Rs.%.2f\n",
            timestamp, bill->vid, bill->power, bill->time, cost);

    fclose(f);
}

int main()
{
    printf("[C3 Billing] Process started. PID: %d\n", getpid());

    printf("[C3 Billing] Waiting for billing data...\n");

    BillingMsg bill;
    int total_vehicles = 0;
    float total_revenue = 0.0f;

    while (1)
    {
        // Open FIFO , blocks until C2 opens write end
        int fd = open("/tmp/billing_pipe", O_RDONLY);
        if (fd == -1) {
            perror("billing pipe open failed");
            exit(1);
        }

        ssize_t bytes = read(fd, &bill, sizeof(BillingMsg));
        close(fd);

        if (bytes <= 0)
            continue;

        // Calculate cost
        float cost = bill.power * bill.time * RATE_PER_KWH;

        // Log to file
        log_billing(&bill, cost);

        // Update totals
        total_vehicles++;
        total_revenue += cost;

        printf("[C3 Billing] Logged Vehicle %d | Cost: Rs.%.2f | Total billed: %d | Revenue: Rs.%.2f\n",
               bill.vid, cost, total_vehicles, total_revenue);
    }

    return 0;
}

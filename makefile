CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -lrt -lpthread

# All executables
ALL = Myexe Generator Controller Billing

.PHONY: all run clean clean-ipc clean-all

# Build everything
all: $(ALL)

# parent.c → Myexe (launches the other 3)
Myexe: parent.c
	$(CC) $(CFLAGS) -o Myexe parent.c $(LDFLAGS)

# C1 → Generator
Generator: C1_VehicleGenerator.c
	$(CC) $(CFLAGS) -o Generator C1_VehicleGenerator.c $(LDFLAGS)

# C2 → Controller
Controller: C2_ChargingController.c
	$(CC) $(CFLAGS) -o Controller C2_ChargingController.c $(LDFLAGS)

# C3 → Billing
Billing: C3_BillingProcess.c
	$(CC) $(CFLAGS) -o Billing C3_BillingProcess.c $(LDFLAGS)

# Run the parent process (which launches all 3 children)
run: all
	./Myexe

# Remove compiled binaries
clean:
	rm -f $(ALL)
	@echo "Cleaned binaries"

# Remove leftover IPC resources (run after a crash)
clean-ipc:
	-rm -f /tmp/billing_pipe
	-rm -f /dev/mqueue/myqueue
	-rm -f /dev/shm/ev_charging
	-rm -f /dev/shm/sem.mysem
	@echo "IPC resources cleaned"

# Full clean
clean-all: clean clean-ipc

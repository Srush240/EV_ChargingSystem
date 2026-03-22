# EV Charging Station Simulator

This is a mini project that simulates a real-world EV charging station using C on Linux.
It uses multiple processes and threads communicating with each other through Linux IPC —
just like how real systems pass data between independent services.

---

#Project Structure
```
miniproject/
├── parent.c                # Starts everything — sets up IPC and launches the 3 processes
├── C1_VehicleGenerator.c   # Simulates vehicles arriving at the station
├── C2_ChargingController.c # Decides who gets to charge and manages the slots
├── C3_BillingProcess.c     # Calculates cost and saves the billing record
├── billing_log.txt         # Gets created automatically when vehicles finish charging
└── Makefile                # Run 'make' to build the whole project
```

---

#The Idea

The project is split into 3 independent child processes, each doing its own job:

- **C1 (Vehicle Generator)** keeps generating vehicles with random IDs, power needs,
  and charging durations — and sends them to the controller through a message queue.

- **C2 (Charging Controller)** picks up those vehicles, checks if a slot is free,
  and if yes, starts a new thread to handle that vehicle's charging session.
  Once charging is done, it passes the billing details to C3 through a named pipe.

- **C3 (Billing Process)** sits and waits for billing data to arrive,
  calculates the cost, and quietly logs everything to `billing_log.txt`.

The parent process ties it all together — it creates the shared memory, semaphore,
and message queue before launching all three children.

---

# IPC Mechanisms Used

| Mechanism       | Used For                                          |
|-----------------|---------------------------------------------------|
| Message Queue   | Vehicles arriving from C1 → C2                   |
| Shared Memory   | Live tracking of slots, power usage, vehicle count|
| Semaphore       | Making sure only one process touches shared memory at a time |
| Named Pipe FIFO | Sending billing info from C2 → C3                |
| Threads         | Each vehicle gets its own charging thread in C2   |
| Signals         | SIGUSR1 for overload alerts, SIGINT for Ctrl+C    |

---

#How to Build and Run

First, make sure you have the tools installed:
```bash
sudo apt install make gcc
```

Then just run:
```bash
make        # builds everything
make run    # runs the full simulation
```

Other useful commands:
```bash
make clean      # removes the compiled binaries
make clean-ipc  # cleans up message queue, shared memory, semaphore and FIFO
make clean-all  # does both of the above
```

> 💡 If the program crashes midway, always run `make clean-ipc` before trying again.
> Otherwise the old IPC resources are still sitting there and the next run will fail.

---

#What It Looks Like When Running
```
Parent Process ID: 22399
Shared memory initialized
Semaphore Created
Message queue created successfully

[C3 Billing] Waiting for billing data...

Vehicle ID: 1248 | Power: 40 kW | Time: 40 min
[Controller] Vehicle 1248 is CHARGING (power=40 kW, time=40 min)
[Controller] Vehicle 1248 finished charging
[C3 Billing] Logged Vehicle 1248 | Cost: Rs.80.00 | Total billed: 1 | Revenue: Rs.80.00

===================================
   ⚡ EV CHARGING STATION MONITOR ⚡
===================================
🚗 Vehicles Charging : 1
🅿️  Available Slots  : 14
🔌 Power Usage       : 40 kW
```

---

#Billing Log

Every time a vehicle finishes charging, a line gets added to `billing_log.txt`:
```
[2026-03-22 11:40:52] Vehicle ID: 1068 | Power: 4 kW | Time: 11 min | Cost: Rs.2.20
[2026-03-22 11:41:03] Vehicle ID: 1342 | Power: 30 kW | Time: 22 min | Cost: Rs.33.00
```

The cost is calculated as:
```
Cost = Power (kW) × Time (min) × Rs. 0.05
```

---

#A Few Things to Keep in Mind

- The station starts with **15 available slots** — you can change this in `parent.c`
- Vehicles that arrive when all slots are full get rejected with a message
- You can send an overload warning manually by running:
```bash
  kill -SIGUSR1 <parent_pid>
```
- The parent PID is printed when you run `make run`

---

#Author

Made by Srushti as part of a systems programming mini project.

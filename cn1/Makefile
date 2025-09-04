# Compiler and Flags
CC = gcc
CFLAGS = -Wall -pthread

# Executable names
SERVER_EXEC = server
UDP_EXEC = udp_tester
TCP_EXEC = tcp_bulk_tester
SCHED_EXEC = test_client
EXECS = $(SERVER_EXEC) $(UDP_EXEC) $(TCP_EXEC) $(SCHED_EXEC)

# Server and Test Parameters
SERVER_PORT = 8080
POLICY = FCFS
TOTAL_MB = 10

# Default target: compile all programs
.PHONY: all
all: $(EXECS)

# --- Compilation Rules ---
$(SERVER_EXEC): server.c
	$(CC) $(CFLAGS) server.c -o $(SERVER_EXEC)

$(UDP_EXEC): udp_tester.c
	$(CC) $(CFLAGS) udp_tester.c -o $(UDP_EXEC)

$(TCP_EXEC): tcp_bulk_tester.c
	$(CC) $(CFLAGS) tcp_bulk_tester.c -o $(TCP_EXEC)

$(SCHED_EXEC): test_scheduler_client.c
	$(CC) $(CFLAGS) test_scheduler_client.c -o $(SCHED_EXEC)


# --- Execution Rules ---
.PHONY: run-server-fcfs
run-server-fcfs: $(SERVER_EXEC)
	@echo "Starting server with FCFS policy..."
	./$(SERVER_EXEC) $(SERVER_PORT) FCFS

.PHONY: run-server-rr
run-server-rr: $(SERVER_EXEC)
	@echo "Starting server with Round-Robin (RR) policy..."
	./$(SERVER_EXEC) $(SERVER_PORT) RR

.PHONY: run-udp-test
run-udp-test: $(UDP_EXEC)
	@echo "Running UDP throughput test..."
	./$(UDP_EXEC) $(SERVER_PORT) test > udp_results.csv
	@echo "UDP test complete. Results saved to udp_results.csv"

.PHONY: run-tcp-test
run-tcp-test: $(TCP_EXEC)
	@echo "Running TCP bulk test for various chunk sizes..."
	./$(TCP_EXEC) $(SERVER_PORT) $(TOTAL_MB) 1 > tcp_results.csv
	./$(TCP_EXEC) $(SERVER_PORT) $(TOTAL_MB) 2 >> tcp_results.csv
	./$(TCP_EXEC) $(SERVER_PORT) $(TOTAL_MB) 4 >> tcp_results.csv
	./$(TCP_EXEC) $(SERVER_PORT) $(TOTAL_MB) 8 >> tcp_results.csv
	./$(TCP_EXEC) $(SERVER_PORT) $(TOTAL_MB) 16 >> tcp_results.csv
	@echo "TCP test complete. Results saved to tcp_results.csv"

.PHONY: run-scheduler-test
run-scheduler-test: $(SCHED_EXEC)
	@echo "Running scheduler demonstration with long and short tasks..."
	./$(SCHED_EXEC) $(SERVER_PORT)
	@echo "Scheduler test client finished."


# --- Process Management & Cleanup ---
.PHONY: kill-server
kill-server:
	@echo "Forcefully stopping any running server process..."
	@-pgrep -f "./$(SERVER_EXEC) $(SERVER_PORT)" | xargs --no-run-if-empty kill -9
	@echo "Kill command sent."

.PHONY: clean
clean:
	rm -f $(EXECS) *.csv
	rm -f $(EXECS)

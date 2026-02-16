CC=g++
CFLAGS= -Wall

all: sysmonExec intfMonitor

sysmonExec: sysmonExec.cpp
	$(CC) $(CFLAGS) sysmonExec.cpp -o sysmonExec

intfMonitor: intfMonitor.cpp
	$(CC) $(CFLAGS) intfMonitor.cpp -o intfMonitor

clean:
	rm -rf sysmonExec intfMonitor

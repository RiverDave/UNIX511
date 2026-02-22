//intfMonitor_solution.cpp - An interface monitor
//
// 13-Jul-20  M. Watler         Created.

#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

const int MAXBUF=128;
static volatile sig_atomic_t isRunning=false;

void signalHandler(int signum);

int main(int argc, char *argv[])
{
    struct sigaction sa;
    //      For sigaction, see http://man7.org/linux/man-pages/man2/sigaction.2.html

    char interface[MAXBUF];
    char statPath[MAXBUF];
    const char logfile[]="Network.log";//store network data in Network.log
    int retVal=0;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);

    if(sigaction(SIGUSR1, &sa, NULL) == -1 ||
       sigaction(SIGUSR2, &sa, NULL) == -1 ||
       sigaction(SIGINT, &sa, NULL) == -1 ||
       sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    strncpy(interface, argv[1], MAXBUF);//The interface has been passed as an argument to intfMonitor
    int fd=open(logfile, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    cout<<"intfMonitor:main: interface:"<<interface<<":  pid:"<<getpid()<<endl;

    while(!isRunning) {
        pause();
    }

    while(isRunning) {
        //gather some stats
        int tx_bytes=0;
        int rx_bytes=0;
        int tx_packets=0;
        int rx_packets=0;
	    ifstream infile;
            sprintf(statPath, "/sys/class/net/%s/statistics/tx_bytes", interface);
	    infile.open(statPath);
	    if(infile.is_open()) {
	        infile>>tx_bytes;
	        infile.close();
	    }
            sprintf(statPath, "/sys/class/net/%s/statistics/rx_bytes", interface);
	    infile.open(statPath);
	    if(infile.is_open()) {
	        infile>>rx_bytes;
	        infile.close();
	    }
            sprintf(statPath, "/sys/class/net/%s/statistics/tx_packets", interface);
	    infile.open(statPath);
	    if(infile.is_open()) {
	        infile>>tx_packets;
	        infile.close();
	    }
            sprintf(statPath, "/sys/class/net/%s/statistics/rx_packets", interface);
	    infile.open(statPath);
	    if(infile.is_open()) {
	        infile>>rx_packets;
	        infile.close();
	    }
	    char data[MAXBUF];
	    //write the stats into Network.log
	    int len=sprintf(data, "%s: tx_bytes:%d rx_bytes:%d tx_packets:%d rx_packets: %d\n", interface, tx_bytes, rx_bytes, tx_packets, rx_packets);
	    write(fd, data, len);
	    sleep(1);
    }
    close(fd);

    return 0;
}

void signalHandler(int signum)
{
    switch(signum) {
        case SIGUSR1:
            write(STDOUT_FILENO, "intfMonitor: starting up\n", 25);
            isRunning=true;
            break;
        case SIGUSR2:
            write(STDOUT_FILENO, "intfMonitor: shutting down\n", 27);
            isRunning=false;
            break;
        case SIGINT:
            write(STDOUT_FILENO, "intfMonitor: ctrl-C discarded\n", 30);
            break;
        case SIGTSTP:
            write(STDOUT_FILENO, "intfMonitor: ctrl-Z discarded\n", 30);
            break;
        default:
            write(STDOUT_FILENO, "intfMonitor: undefined signal\n", 30);
            break;
    }
}

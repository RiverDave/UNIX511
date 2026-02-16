/*****************************************************************************
 * UNX511-Lab5
 * I declare that this lab is my own work in accordance with Seneca Academic
 * Policy. No part of this assignment has been copied manually or
 * electronically from any other source (including web sites) or distributed
 * to other students.
 *
 * Name: David Rivera    Student ID: 137648226    Date: 15/02/26
 *
 *****************************************************************************/

#include <iostream>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

static const int NUM_CHILDREN = 2;
static pid_t childPids[NUM_CHILDREN];
static volatile sig_atomic_t childrenReady = 0;

// parent catches SIGUSR1 from children to know they're alive
void parentSignalHandler(int signum) {
    if (signum == SIGUSR1) {
        childrenReady++;
    }
}

int main() {
    // interfaces to monitor
    const char* interfaces[NUM_CHILDREN] = {"lo", "enp0s1"};

    // set up handler so we know when children are ready
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = parentSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    cout << "sysmonExec: starting" << endl;

    // spawn children via fork + exec
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            // child - exec intfMonitor
            execl("./intfMonitor", "intfMonitor", interfaces[i], (char*)NULL);
            // if exec fails
            perror("execl");
            exit(1);
        } else {
            // parent
            childPids[i] = pid;
            cout << "sysmonExec: spawned intfMonitor for "
                 << interfaces[i] << " (pid " << pid << ")" << endl;
        }
    }

    // wait for all children to signal they're ready
    cout << "sysmonExec: waiting for children to be ready..." << endl;
    while (childrenReady < NUM_CHILDREN) {
        sleep(1);
    }
    cout << "sysmonExec: all children ready" << endl;

    // send start signal (SIGUSR1) to all children
    for (int i = 0; i < NUM_CHILDREN; i++) {
        cout << "sysmonExec: sending SIGUSR1 to " << childPids[i] << endl;
        kill(childPids[i], SIGUSR1);
    }

    // let them monitor for 30 seconds
    cout << "sysmonExec: monitoring for 30 seconds..." << endl;
    sleep(30);

    // send stop signal (SIGUSR2) to all children
    for (int i = 0; i < NUM_CHILDREN; i++) {
        cout << "sysmonExec: sending SIGUSR2 to " << childPids[i] << endl;
        kill(childPids[i], SIGUSR2);
    }

    // wait for all children to exit
    cout << "sysmonExec: waiting for children to exit..." << endl;
    for (int i = 0; i < NUM_CHILDREN; i++) {
        int status;
        waitpid(childPids[i], &status, 0);
        cout << "sysmonExec: child " << childPids[i] << " exited with status "
             << WEXITSTATUS(status) << endl;
    }

    cout << "sysmonExec: exiting" << endl;
    return 0;
}

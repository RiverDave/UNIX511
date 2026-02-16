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
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <unistd.h>

using namespace std;

// flags set by signal handlers
static volatile sig_atomic_t running = 0;
static volatile sig_atomic_t shutdown_requested = 0;

// signal handler for all signals we care about
void signalHandler(int signum) {
    switch (signum) {
        case SIGUSR1:
            write(STDOUT_FILENO, "intfMonitor: starting up\n", 25);
            running = 1;
            break;
        case SIGUSR2:
            write(STDOUT_FILENO, "intfMonitor: shutting down\n", 27);
            shutdown_requested = 1;
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

// reads /proc/net/dev and prints stats for the given interface
void readInterfaceStats(const string& iface) {
    ifstream procNetDev("/proc/net/dev");
    if (!procNetDev.is_open()) {
        cerr << "intfMonitor: failed to open /proc/net/dev" << endl;
        return;
    }

    string line;
    // skip header lines
    getline(procNetDev, line);
    getline(procNetDev, line);

    while (getline(procNetDev, line)) {
        // each line looks like "  iface: rx_bytes rx_packets ..."
        size_t colonPos = line.find(':');
        if (colonPos == string::npos) continue;

        string name = line.substr(0, colonPos);
        // trim whitespace
        size_t start = name.find_first_not_of(" \t");
        if (start != string::npos) name = name.substr(start);

        if (name == iface) {
            string stats = line.substr(colonPos + 1);
            istringstream iss(stats);
            long long rxBytes, rxPackets, rxErrs, rxDrop;
            long long rxFifo, rxFrame, rxCompressed, rxMulticast;
            long long txBytes, txPackets, txErrs, txDrop;
            long long txFifo, txColls, txCarrier, txCompressed;

            iss >> rxBytes >> rxPackets >> rxErrs >> rxDrop
                >> rxFifo >> rxFrame >> rxCompressed >> rxMulticast
                >> txBytes >> txPackets >> txErrs >> txDrop
                >> txFifo >> txColls >> txCarrier >> txCompressed;

            cout << "intfMonitor [" << iface << "]: "
                 << "rx_bytes=" << rxBytes
                 << " rx_packets=" << rxPackets
                 << " tx_bytes=" << txBytes
                 << " tx_packets=" << txPackets
                 << endl;
            break;
        }
    }
    procNetDev.close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "usage: " << argv[0] << " <interface>" << endl;
        return 1;
    }

    string iface = argv[1];

    // register signal handlers
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // no SA_RESTART so pause() gets interrupted

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);

    // send SIGUSR1 to parent to say "I'm ready"
    kill(getppid(), SIGUSR1);

    // wait until we get the start signal (SIGUSR1)
    while (!running) {
        pause();
    }

    // main monitoring loop - keep going until shutdown
    while (!shutdown_requested) {
        readInterfaceStats(iface);
        // check between sleeps so we don't hang around
        for (int i = 0; i < 5 && !shutdown_requested; i++) {
            sleep(1);
        }
    }

    return 0;
}

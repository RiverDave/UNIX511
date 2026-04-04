/******************************************************************************
 * UNX511-Assignment 3
 * I declare that this assignment is my own work in accordance with Seneca
 * Academic Policy. No part of this assignment has been copied manually or
 * electronically from any other source (including web sites) or distributed to
 * other students.
 *
 * Name: David Rivera - Student ID: 137648226 Date: 04/04/2025
 *
 *
 *****************************************************************************/

#include <csignal>
#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <queue>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef UNX_LOGGER_H
#define UNX_LOGGER_H

struct sockaddr_in;

namespace UNXLogger {

#define LOG_BUFF_LEN 4096
#define SERVER_PORT 1234
#define LOGGER_PORT 4321

enum LOG_SEVERITY {
  DEBUG,
  WARNING,
  ERROR,
  CRITICAL,
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static char buf[LOG_BUFF_LEN];
static struct sockaddr_in servaddr, loggeraddr;
static int sockfd;
static socklen_t socklen;
static pthread_t rectid;
static LOG_SEVERITY loglevel;
static volatile sig_atomic_t is_running;

static char levelstr[][16] = {"DEBUG", "WARNING", "ERROR", "CRITICAL"};

/* sets up UDP socket, binds to LOGGER_PORT, and spins up the recv thread */
void InitializeLog();

/* sets the minimum severity threshold; messages below this are dropped */
void SetLogLevel(LOG_SEVERITY);

/* formats a timestamped log entry and sends it to the server over UDP */
void Log(LOG_SEVERITY, const char *, const char *, int, const char *);

/* signals the recv thread to stop, joins it, and closes the socket */
void ExitLog();

} // namespace UNXLogger

#endif

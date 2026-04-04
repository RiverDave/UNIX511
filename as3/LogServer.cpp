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

#include "Logger.h"
#include <arpa/inet.h>
#include <assert.h>
#include <cerrno>
#include <cinttypes>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <inttypes.h>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 1234
#define LOGGER_PORT 4321
#define LOG_FILE_NAME "server.log"

volatile sig_atomic_t is_running;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in servaddr, loggeraddr;

FILE *logfile;

static void shut_down_handler(int sig);
void *recv_func(void *arg);

/* TUI loop — lets you change log level or dump the log file; relays commands to
 * Logger clients */
int main(void) {

  // ========== Intercept ctrl-C - sig handler boilerplate =============
  struct sigaction action;
  action.sa_handler = shut_down_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);

  // ========== IPC specific stuff ==========
  // Handle socket initialization

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd < 0) {
    perror("Couldn't establish a socket");
    return EXIT_FAILURE;
  }

  // handle logger conn - we'll write to this.
  memset(&loggeraddr, 0, sizeof(loggeraddr));

  loggeraddr.sin_family = AF_INET;
  loggeraddr.sin_port = htons(LOGGER_PORT);
  loggeraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  // handle server conn - we need to bind this socket as we'll listen here
  memset(&servaddr, 0, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERVER_PORT);
  servaddr.sin_addr.s_addr = INADDR_ANY;
  if (bind(sockfd, reinterpret_cast<struct sockaddr *>(&servaddr),
           sizeof(servaddr)) < 0) {

    std::cerr << "ERROR: bind() failed for " << SERVER_PORT << "\n";
    close(sockfd);
    return EXIT_FAILURE;
  }
  std::cout << "Listening on: " << SERVER_PORT << " \n";

  fcntl(sockfd, F_SETFL, O_NONBLOCK); // set our udp sock to non-block.

  // ========== Receive thread init ==========
  pthread_t rectid;
  int ret = pthread_create(&rectid, nullptr, recv_func, &sockfd);
  if (ret != 0) {
    perror("Unable to initialize receiver thread");
    return EXIT_FAILURE;
  }

  is_running = 1;
  char in_buf[1024];
  while (is_running) {

    std::cout << "1. Set the log level \n";
    std::cout << "2. Dump log file here \n";
    std::cout << "0. shutdown \n";
    std::cout << "CMD: ";

    if (fgets(in_buf, 1024, stdin) == NULL)
      break;
    std::cout << std::endl;

    int opt = atoi(in_buf);

    switch (opt) {

    case 1: {
      char levelbuf[10];
      // Let the user handle the log level:
      std::cout << "Set the log level: ";
      if (fgets(levelbuf, 10, stdin) == NULL)
        break;
      int level = atoi(levelbuf);

      // send the buffer
      char buf[128];
      memset(buf, 0, 128);
      ssize_t len = sprintf(buf, "Set Log Level=%d", level) + 1;
      ssize_t sent = sendto(sockfd, buf, len, 0, (struct sockaddr *)&loggeraddr,
                            sizeof(loggeraddr));
      std::cout << "[SERVER] Sent " << sent << " Bytes" << "\n";
      std::cout << buf << "\n";
    } break;

    case 2: {

      pthread_mutex_lock(&mutex);

      // Given the lab lack of specifics reading the file to buffer,
      // I'm using a mix of C/C++ throghout this and more - not the prettiest
      // code I've written...

      std::ifstream fin(LOG_FILE_NAME);
      std::string contents((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
      std::cout << contents << "\n";
      pthread_mutex_unlock(&mutex);

      std::cout << "Press any key to continue: ";
      fflush(stdout);
      int c;
      while ((c = getchar()) != '\n' && c != EOF)
        ;        // flush leftover input
      getchar(); // now actually wait
    } break;

    case 0: {
      // Its almost 6 am in the morning, pardon this shorcut lmao
      goto endloop;
    }
    }
  }

  std::cout << "\n";

endloop:

  // assert(false && "NYI");
  // convert buf to num, and switch statement with options we know.

  is_running = 0;
  pthread_join(rectid, NULL);
  close(sockfd);
  return EXIT_SUCCESS;
}

/* receives log entries from Logger clients and appends them to disk */
void *recv_func(void *arg) {
  int fd = *reinterpret_cast<int *>(arg);
  int logfd = open(LOG_FILE_NAME, O_WRONLY | O_CREAT | O_APPEND, 0666);
#define RECV_BUFF_LEN 512
  char recv_buf[RECV_BUFF_LEN];

  while (is_running) {
    socklen_t addrlen = sizeof(servaddr);
    ssize_t received =
        recvfrom(fd, recv_buf, RECV_BUFF_LEN, 0,
                 reinterpret_cast<struct sockaddr *>(&servaddr), &addrlen);
    if (received < 0) {
      if (errno == EAGAIN) {
        sleep(1);
        continue;
      }

      std::cout
          << "[RECEIVE] log unsuccessful on recv logging will keep on goin'\n";
      continue;
    }

    // write to log file
    pthread_mutex_lock(&mutex);
    write(logfd, recv_buf, received);
    pthread_mutex_unlock(&mutex);
    memset(recv_buf, 0, RECV_BUFF_LEN);
  }

  std::cout << "[RECEIVE] Ending receiver\n";
  close(logfd);
  pthread_exit(NULL);
}

/* catches SIGINT to end the main loop gracefully */
static void shut_down_handler(int sig) {
  switch (sig) {

  case SIGINT:
    is_running = 0;
    break;
  }
}
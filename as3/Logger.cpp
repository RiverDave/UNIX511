/******************************************************************************
* UNX511-Assignment 3
* I declare that this assignment is my own work in accordance with Seneca Academic Policy.
* No part of this assignment has been copied manually or electronically from any other source
* (including web sites) or distributed to other students.
*
* Name: David Rivera - Student ID: 137648226 Date: 04/04/2025
*
*
*****************************************************************************/

#include "Logger.h"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <optional>
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

namespace UNXLogger {


void *recv_func(void *arg) {
  int fd = *reinterpret_cast<int *>(arg);
#define RECV_BUFF_LEN 512
  char recv_buf[RECV_BUFF_LEN];
  while (is_running) {

    socklen_t addrlen = sizeof(loggeraddr);
    ssize_t received =
        recvfrom(sockfd, recv_buf, RECV_BUFF_LEN, 0,
                 reinterpret_cast<struct sockaddr *>(&loggeraddr), &addrlen);
    if (received < 0) {
      if (errno == EAGAIN) {
        sleep(1);
        continue;
      }

      std::cout << "[RECEIVE] Got:" << strerror(errno) << "\n";
      std::cout
          << "[RECEIVE] log unsuccessful on recv logging will keep on goin'\n";
      continue;
    }

    int lev = -1;
    sscanf(recv_buf, "Set Log Level=%d", &lev);
    std::cout << "Changing severity to " << lev << "\n";

    pthread_mutex_lock(&mutex);
    loglevel = static_cast<LOG_SEVERITY>(lev);
    pthread_mutex_unlock(&mutex);

    // (Not entirely sure if this is necessary)
    memset(recv_buf, 0, RECV_BUFF_LEN);
  }

  std::cout << "[RECEIVE] Ending receiver\n";
  pthread_exit(NULL);
}

void InitializeLog() {

  // Handle socket initialization
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("Couldn't establish a socket");
    return;
  }

  // == handle server conn

  memset(&servaddr, 0, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERVER_PORT);
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  // == handle logger conn
  memset(&loggeraddr, 0, sizeof(loggeraddr));

  loggeraddr.sin_family = AF_INET;
  loggeraddr.sin_port = htons(LOGGER_PORT);
  loggeraddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd, reinterpret_cast<struct sockaddr *>(&loggeraddr),
           sizeof(loggeraddr)) < 0) {
    std::cerr << "ERROR: bind() failed for " << LOGGER_PORT << "\n";
    close(sockfd);
    return;
  }

  fcntl(sockfd, F_SETFL, O_NONBLOCK); // set our udp sock to non-block.

  // initialize receive thread
  is_running = 1;
  int ret = pthread_create(&rectid, nullptr, recv_func, &sockfd);
  if(ret != 0){
    perror("Unable to initialize receiver thread");
    return;
  }


  std::cout << "[SUCCESS] Log was initialized\n";
}

void SetLogLevel(LOG_SEVERITY newlevel){
    loglevel = newlevel;
}

void Log(LOG_SEVERITY severity, const char *filename, const char *funcname,
         int line, const char *msg) {
  // if severity level is beyond the filtered threshold, early return. It's
  // important to mutex here to avoid race conditions with the receive thread.
  pthread_mutex_lock(&mutex);
  if (severity < loglevel) {
    pthread_mutex_unlock(&mutex);
    return;
  }
  time_t now = time(0);
  char *dt = ctime(&now);
  dt[strcspn(dt, "\n")] = '\0'; // this already contains a new line - replace it

  // other threads within this process may mutate our communication buffer at
  // the same time thus, we need to guard it with mutexes
  memset(buf, 0, LOG_BUFF_LEN);
  ssize_t len = sprintf(buf, "%s %s %s:%s:%d %s\n", dt, levelstr[severity],
                        filename, funcname, line, msg) +
                1;
  buf[len - 1] = '\0';
  ssize_t sent =
      sendto(sockfd, buf, len, 0,
             reinterpret_cast<struct sockaddr *>(&servaddr), sizeof(servaddr));
  pthread_mutex_unlock(&mutex);
}

void ExitLog(){
    std::cout << "[LOG] Quitting Log... farewell...\n";
    is_running = 0;
    pthread_join(rectid, NULL);
    close(sockfd);
}


}

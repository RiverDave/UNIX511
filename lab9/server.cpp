/******************************************************************************
* UNX511-Lab9
* I declare that this lab is my own work in accordance with Seneca Academic Policy.
* No part of this assignment has been copied manually or electronically from any other source
* (including web sites) or distributed to other students.
*
* Name: David Rivera Student ID: 137648226 Date: 01/04/2025
*
*
*****************************************************************************/


#include <arpa/inet.h>
#include <array>
#include <asm-generic/socket.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <net/if.h>
#include <netinet/in.h>
#include <ostream>
#include <pthread.h>
#include <queue>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define CLIENT_COUNT 3 // 3 clients in this lab
bool is_running = false;
std::queue<std::string> messageQueue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *recv_func(void *arg);

static void shut_down_handler(int sig) {
  switch (sig) {

  case SIGINT:
    is_running = false;
    break;
  }
}

int main(int argc, char *argv[]) {

  using namespace std;

  if (argc < 2) {
    cout << "usage: server <port number>" << endl;
    return -1;
  }

  // Intercept ctrl-C - sig handler boilerplate
  struct sigaction action;
  action.sa_handler = shut_down_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);

  const int port_number = atoi(argv[1]);

  int serverFd;
  if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    cout << "client(" << getpid() << "): " << strerror(errno) << endl;
    exit(-1);
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_number);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (bind(serverFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) <
      0) {
    std::cerr << "ERROR: bind() failed for " << port_number << std::endl;
    close(serverFd);
    return 1;
  }

  if (listen(serverFd, CLIENT_COUNT) < 0) {
    std::cerr << "ERROR: listen() failed\n";
    close(serverFd);
    return 1;
  } else {
    cout << "listening on port" <<  port_number << "\n";
  }

  size_t clientCount = 0;
  std::array<int, 3> persistedCons;
  std::array<pthread_t, 3> tids;
  is_running = true;
  fcntl(serverFd, F_SETFL, O_NONBLOCK);
  while (is_running) {

    int connFd = accept(serverFd, nullptr, nullptr);

    if (connFd == -1 && errno == EAGAIN) {
      // No incoming request - skip - perhaps sleep 5 seconds?

    } else {
      if (clientCount < 3) {
        persistedCons[clientCount] = connFd;
        int ret = pthread_create(&tids[clientCount], nullptr, recv_func,
                                 &persistedCons[clientCount]);
        if (ret != 0) {
          cout << "Cannot create receive thread" << endl;
          cout << strerror(errno) << endl;
          close(persistedCons[clientCount]);
          return -1;
        }

        cout << "Creating ptrhead " << tids[clientCount] << std::endl;
        clientCount++;
      }
    }

    // Queue check
    pthread_mutex_lock(&mutex);
    if (!messageQueue.empty()) {
      // recover msg from receiver
      
      string msg = messageQueue.front();
      messageQueue.pop();
      pthread_mutex_unlock(&mutex);
      cout << msg << "\n";

    } else {

      pthread_mutex_unlock(&mutex);
      sleep(1);
    }
  }

  // cleanup: 
  // - run through all tid's and join them
  // - then close all redirected connection fd's
  // - write quit from the server to connections to instruct shutdown


  for (auto &conn : persistedCons) {
    write(conn, "Quit", 4);
  }

  for (auto &tid : tids) {
    pthread_join(tid, nullptr);
    cout << "Joined tid " << tid << "\n";
  }

  for (auto &conn : persistedCons) {
    close(conn);
  }

  // cleanup master socket
  close(serverFd);
  memset(&addr, 0, sizeof(addr));
}

// From here consider that each call to `accept` we did in main
// has redirected the buffer written from the client to a unique fd
void *recv_func(void *arg) {
  int *connFd = reinterpret_cast<int *>(arg);

  // set timeout to 5 sec
  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  setsockopt(*connFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

#define LEN 1024
  char buf[LEN];
  while (is_running) {

    ssize_t len = read(*connFd, buf, LEN);
    if (len < 0) {
      if (errno == EAGAIN)
        continue;

      std::cout << "Failed to read from receiver\n";
      continue;
    } else if (len == 0) {
      std::cout << "Read EOF from receiver\n";
    } else {
      std::string msg{buf, static_cast<size_t>(len)};

      pthread_mutex_lock(&mutex);
      messageQueue.push(msg);
      pthread_mutex_unlock(&mutex);
      std::cout << "Pushing " << len << " Bytes from receiver" << std::endl;
    }
  }
  pthread_exit(NULL);
}

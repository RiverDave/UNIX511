/******************************************************************************
 * UNX511-Lab8
 * I declare that this lab is my own work in accordance with Seneca Academic
 * Policy. No part of this assignment has been copied manually or electronically
 * from any other source (including web sites) or distributed to other students.
 *
 * Name: David Rivera Student ID: 137648226 Date: 26/03/26
 *
 *
 *****************************************************************************/

// server.cpp - Message queue server that dispatches messages between clients
//
#include "client.h"
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include <queue>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

using namespace std;

key_t key;
int msgid;

bool is_running;

// Internal queue and its mutex — shared between recv thread and main write loop
queue<Message> message;
pthread_mutex_t lock_x;

void *recv_func(void *arg);

static void shutdownHandler(int sig) {
  switch (sig) {
  case SIGINT:
    is_running = false;
    break;
  }
}

int main() {
  int ret;
  pthread_t tid;

  // Intercept ctrl-C
  struct sigaction action;
  action.sa_handler = shutdownHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);

  // Same key as all clients: ftok("serverclient", 65)
  key = ftok("serverclient", 65);
  msgid = msgget(key, 0666 | IPC_CREAT);

  pthread_mutex_init(&lock_x, NULL);
  is_running = true;

  ret = pthread_create(&tid, NULL, recv_func, NULL);
  if (ret != 0) {
    is_running = false;
    cout << strerror(errno) << endl;
    return -1;
  }

  // Write loop: pop from internal queue, set mtype to dest, forward via System
  // V queue
  while (is_running) {
    while (message.size() > 0) {
      pthread_mutex_lock(&lock_x);
      Message recvMsg = message.front();
      message.pop();
      pthread_mutex_unlock(&lock_x);

      // The destination client number becomes the new mtype
      recvMsg.mtype = recvMsg.msgBuf.dest;
      msgsnd(msgid, &recvMsg, sizeof(recvMsg), 0);

      cout << "server: forwarding from client " << recvMsg.msgBuf.source
           << " to client " << recvMsg.msgBuf.dest << endl;
    }
    usleep(10000); // 10 ms — avoid busy-spinning
  }

  cout << "server: quitting..." << endl;

  // Send "Quit" to each client so they shut down cleanly
  Message quitMsg;
  quitMsg.msgBuf.source = 0;
  strncpy(quitMsg.msgBuf.buf, "Quit", BUF_LEN);

  quitMsg.mtype = 1;
  quitMsg.msgBuf.dest = 1;
  msgsnd(msgid, &quitMsg, sizeof(quitMsg), 0);

  quitMsg.mtype = 2;
  quitMsg.msgBuf.dest = 2;
  msgsnd(msgid, &quitMsg, sizeof(quitMsg), 0);

  quitMsg.mtype = 3;
  quitMsg.msgBuf.dest = 3;
  msgsnd(msgid, &quitMsg, sizeof(quitMsg), 0);

  // Destroying the queue also unblocks the recv thread's msgrcv()
  msgctl(msgid, IPC_RMID, NULL);

  pthread_join(tid, NULL);
  pthread_mutex_destroy(&lock_x);

  return 0;
}

void *recv_func(void *arg) {
  while (is_running) {
    Message msg;
    // Block waiting for mtype=4 — the type all clients use to reach the server
    int ret = msgrcv(msgid, &msg, sizeof(msg), 4, 0);
    if (ret == -1)
      break; // queue was destroyed (IPC_RMID) — time to exit

    pthread_mutex_lock(&lock_x);
    message.push(msg);
    pthread_mutex_unlock(&lock_x);
  }
  pthread_exit(NULL);
}

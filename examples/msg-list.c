#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>

int main(int argc, char *argv[]) {
  struct msginfo info;
  int s, ind;

  s = msgctl(0, MSG_INFO, (struct msqid_ds *)&info);
  if (s == -1) {
    perror("msgctl");
    exit(1);
  }
  printf("Maximum ID index = %d\n", s);
  printf("queues in_use    = %ld\n", (long)info.msgpool);
  printf("msg_hdrs         = %ld\n", (long)info.msgmap);
  exit(EXIT_SUCCESS);
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>

int main(int argc, char *argv[])
{
    struct msginfo info;
    int s,ind;

    s= msgctl(0, MSG_INFO, (struct msqid_ds *) &info);
    if(s == -1){
      perror("msgctl");
      exit(1);
    }
    ind=msgctl(s,IPC_RMID, NULL);
    if(ind == -1) {
      perror("Couldn't remove queue");
      exit(1);
    }
    printf("Queue is removed successfully\n");

    exit(EXIT_SUCCESS);

}
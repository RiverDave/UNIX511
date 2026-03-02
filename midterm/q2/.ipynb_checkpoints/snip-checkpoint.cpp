#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
 
int main() {
    int sockfd;
    struct ifreq ifr;
    char iface[] = "eth0";  
 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }
 
    strcpy(ifr.ifr_name, iface);
 
    // Retrieve the IP address using ioctl
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1) {
        perror("ioctl");
        close(sockfd);
        return EXIT_FAILURE;
    }
 
    struct sockaddr_in *ip = (struct sockaddr_in *)&ifr.ifr_addr;
    printf("IP Address of %s: %s\n", iface, inet_ntoa(ip->sin_addr));
 
    close(sockfd);
    return 0;
}
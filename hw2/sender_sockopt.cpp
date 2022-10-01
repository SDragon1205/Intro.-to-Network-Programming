#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

using namespace std;

#define MAXLINE 1024
#define MAXbuff 1022

int main(int argc, char **argv){
    int sockfd;
    struct sockaddr_in servaddr;
    int n, nready;
    char sendline[MAXLINE], recvline[MAXLINE];
    char ack;
    ssize_t ndata;
    socklen_t len;
    struct timeval timeout;

	if( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		return 0;
	}

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[3]));
    if( inet_pton(AF_INET, argv[2], &servaddr.sin_addr) <= 0){
		printf("inet_pton error for %s\n",argv[1]);
		return 0;
	}

    ack = 0;
    //FILE *fp;
    int maxfd = sockfd;
    int fdesc = open(argv[1], O_RDONLY);
    //cout << argv[1];
    if(fdesc < 0)
    {
        perror("open");
    }

    sendline[0] = 0;
    sendline[1] = 0;
    if((ndata = read(fdesc, &sendline[2], MAXbuff)) < 0)
    {
        perror("read");
    }
    //cout << "len: " << strlen(sendline);
    // for(int i = 2; i < 10; i++)
    // {
    //     cout << i << ": " << sendline[i] << "\n";
    // }
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; /* 0.1 秒*/
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    int leave = 0;//

    printf("s: %d%d\n", sendline[0], sendline[1]);
    sendto(sockfd, sendline, ndata+2, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
    while (1) {
        memset(recvline, 0, sizeof(recvline));
        len = sizeof(servaddr);
        if((n = recvfrom(sockfd, recvline, MAXLINE, 0, (struct sockaddr *)&servaddr, &len)) > 0)
        {
            printf("r : %d%d\n", recvline[0], recvline[1]);
            //cout << "r: " << recvline << "\n";
            // cout << "s: " << sendline << "\n";

            ack += 1;
            sendline[0] = ack / 128;
            sendline[1] = ack % 128;
            ndata = read(fdesc, &sendline[2], MAXbuff);
            printf("s: %d%d\n", sendline[0], sendline[1]);
            sendto(sockfd, sendline, ndata+2, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));            
            if(ndata <= 0)
            {
                break;
            }
            leave = 0;//
        }
        else
        {
            printf("s: %d%d\n", sendline[0], sendline[1]);
            sendto(sockfd, sendline, ndata+2, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
            if(ndata != 1022)
            {
                leave++;//
            }
            if(leave >= 10)//
            {//
                break;//
            }//
        }

    }


    close(fdesc);
}
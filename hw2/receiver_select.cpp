#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

using namespace std;

#define MAXLINE 1024
   
// Driver code
int main(int argc, char *argv[]){
    int lconnfd, udpfd, nready;
    char sendline[MAXLINE], recvline[MAXLINE];
    pid_t childpid;
    fd_set rset;
    ssize_t n, ndata;
    socklen_t len;
    struct sockaddr_in cliaddr, servaddr;
    struct timeval timeout;

    /* 4create UDP socket */
    if((udpfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		return 0;
	}

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[2]));

    if( bind(udpfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) == -1){
		printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
		return 0;
	}

    int fdesc = open(argv[1], O_CREAT|O_WRONLY|O_TRUNC);
    char ack = 0;

    memset(sendline, 0, sizeof(sendline));
    memset(recvline, 0, sizeof(recvline));
    sendline[2] = 0;
    bool test = true;
    while(1) {
        FD_ZERO(&rset);
        FD_SET(udpfd, &rset);
        memset(recvline, 0, sizeof(recvline));
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        if ( (nready = select(udpfd + 1, &rset, NULL, NULL, &timeout)) <= 0) {
            if(nready == 0 && n == 2)
            {
                break;
            }
            else if (errno == EINTR)
                continue;               /* back to for() */
            else
                perror("select error");
        }
        if (FD_ISSET(udpfd, &rset)) {
            len = sizeof(cliaddr);
            if((n = recvfrom(udpfd, recvline, MAXLINE, 0, (struct sockaddr *) &cliaddr, &len)) > 0)
            {
                //cout << "n: " << n << "\n";
                if(recvline[0] != ack / 128 || recvline[1] != ack % 128)
                {
                    printf("s: %d%d\n", sendline[0], sendline[1]);
                    sendto(udpfd, sendline, 2, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                    continue;
                }
                // if(test && recvline[0] == 0 && recvline[1] == 1)
                // {
                //     test = false;
                //     continue;
                // }
                if((ndata = write(fdesc, &recvline[2], n-2)) < 0)
                {
                    printf("write");
                    return 0;
                }
                // if(n < 1024)//
                // {
                //     printf("last\n");
                //     break;
                // }
                sendline[0] = recvline[0];
                sendline[1] = recvline[1];
                sendto(udpfd, sendline, 2, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                printf("r: %d%d\n", recvline[0], recvline[1]);
                printf("s: %d%d\n", sendline[0], sendline[1]);
                //cout << "r: " << recvline[2] << "\n";
                if(n < 1024)
                {
                    printf("last\n");
                    break;
                }

                ack++;
            }
        }
    }
    close(fdesc);
    
    return 0;
}
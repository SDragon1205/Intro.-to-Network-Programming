#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <map>
#include <limits.h>              /* for OPEN_MAX */
#include <sys/poll.h>

#define OPEN_MAX 20

using namespace std;

int main(int argc, char **argv){
    int i, maxi, listenfd, connfd, sockfd;
    int nready;
    ssize_t n;
    char line[4096];
    socklen_t clilen;
    struct pollfd client[10];
    struct sockaddr_in cliaddr, servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(atoi(argv[1]));
	bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr));

    listen(listenfd, 10);

    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
    for (i = 1; i < OPEN_MAX; i++)
            client[i].fd = -1;              /* -1 indicates available entry */
    maxi = 0;                                /* max index into client[] array */
	for ( ; ; ) {
        nready = poll(client, maxi+1, -1);

        if (client[0].revents & POLLRDNORM) {   /* new client connection */
            clilen = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);
      		//ifdef  NOTDEF
            printf("new client: %s:%s\n", inet_ntoa(cliaddr.sin_addr), to_string(ntohs(cliaddr.sin_port)).c_str());
      		//endif
            for (i = 1; i < OPEN_MAX; i++)
            if (client[i].fd < 0) {
                client[i].fd = connfd;  /* save descriptor */
                break;
            }
            if (i == OPEN_MAX)
                perror("too many clients");
            client[i].events = POLLRDNORM;
            if (i > maxi)
          		maxi = i;                               /* max index in client[] array */
            if (--nready <= 0)
                continue;                               /* no more readable descriptors */
        }
		for (i = 1; i <= maxi; i++) {   /* check all clients for data */
            if ( (sockfd = client[i].fd) < 0)
                continue;
            if (client[i].revents & (POLLRDNORM | POLLERR)) {
                if ( (n = read(sockfd, line, sizeof(line))) < 0) {
                    if (errno == ECONNRESET) {
                        /* connection reset by client */
						//ifdef  NOTDEF
                        printf("client[%d] aborted connection\n", i);
						//endif
                        close(sockfd);
                        client[i].fd = -1;
                    }
					else
                        perror("readline error");
                }
				else if (n == 0) {
                    /*4connection closed by client */
					//ifdef  NOTDEF
                    printf("client[%d] closed connection\n", i);
					//endif
                    close(sockfd);
                    client[i].fd = -1;
                }
				else
                    write(sockfd, line, n);
            if (--nready <= 0)
                    break;                    /* no more readable descriptors */
			}
		}
	}
}

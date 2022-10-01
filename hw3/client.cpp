#include <iostream>
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
#include <signal.h>
#include <fcntl.h>

using namespace std;

#define MAXLINE 4096
#define name_len 64

int main(int argc, char *argv[]) {
    //int ch2;
	int   sockfd, n;
	char  readbuf[MAXLINE], writebuf[MAXLINE];
	struct sockaddr_in  servaddr;
	string cmd;

	if( argc != 4){
		printf("./client <ip> <port> <username>\n");
		return 0;
	}

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		return 0;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	if( inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0){
		printf("inet_pton error for %s\n",argv[1]);
		return 0;
	}

	if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
		printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
		return 0;
	}

    memset(writebuf, 0, sizeof(writebuf));
    strcat(writebuf, "n ");
    strcat(writebuf, argv[3]);
    if (send(sockfd, writebuf, strlen(writebuf), 0) == -1) {
        perror("send");
    }
    printf("Welcome to the dropbox-like server: %s\n", argv[3]);

    fd_set rset;
    int nready, recn, len;
    int maxfd = sockfd;
    int fd_stdin = fileno(stdin);
    if(fd_stdin > maxfd)
    {
        maxfd = fd_stdin;
    }

	while(1)
    {
        FD_ZERO(&rset);
        FD_SET(fd_stdin, &rset);
        FD_SET(sockfd, &rset);
        nready = select(maxfd+1, &rset, NULL, NULL, NULL); // 同时监听标准输入和服务器。
        if(nready == -1)
        {
            perror("select");
            exit(0);
        }
        else if(nready == 0)
        {
            continue;    
        }

        if(FD_ISSET(sockfd, &rset)) // 服务器来了数据
        {
			memset(readbuf, 0, sizeof(readbuf));
            //recn = read(sockfd, readbuf, sizeof(readbuf));
            recn = recv(sockfd, readbuf, sizeof(readbuf), 0);
            if(recn == 0)
            {
                printf("selectserver: server hung up\n");
                break;
            }
            else if(-1 == recn)
            {
                perror("recv wrong");
                //break;
            }    
            else
            {
                if(readbuf[0] == 'd')
                {
                    char file_name[MAXLINE];
                    memset(file_name, 0, MAXLINE);
                    strcat(file_name, readbuf + 2);
                    FILE *fff = fopen(file_name, "a");
                    fwrite(readbuf + name_len, 1, recn - name_len, fff);
                    fclose(fff);
                    // int fdesc = open(file_name, O_CREAT | O_APPEND | O_RDWR);
                    // int ndata;
                    // if((ndata = write(fdesc, readbuf + name_len, recn - name_len)) < 0)
                    // {
                    //     perror("write");
                    // }
                    // close(fdesc);
                    
                    //cout << "ch2: " << ch2++ << "\n";
                    if(recn != MAXLINE)
                    {
                        printf("Progress : [######################]\n");
                        printf("[Download] %s Finish!\n", file_name);                    
                    }
                }
                else if(readbuf[0] == 'r')
                {
                    char file_name[MAXLINE];
                    memset(file_name, 0, MAXLINE);
                    strcpy(file_name, readbuf + 2);
                    FILE *fff = fopen(file_name, "w");
                    fclose(fff);
                    //printf("remove %s\n", file_name);
                }
                else if(readbuf[0] == '[')
                {
                    printf("%s", readbuf);
                }
            }
            //fputs(readbuf, stdout);
            memset(readbuf, 0, sizeof(readbuf));
        }
        
        if(FD_ISSET(fd_stdin, &rset)) // 标准输入来了数据就发送给server
        {
			char cmd[MAXLINE];
			memset(cmd, 0, MAXLINE); // 確保 struct 為空
			read(fileno(stdin), cmd, MAXLINE);

			if(cmd[1] == 'e') //exit
			{
				break;
			}
            else if(cmd[1] == 's') //sleep
            {
                memset(writebuf, 0, MAXLINE);
				strcpy(writebuf, "s");
				send(sockfd, writebuf, MAXLINE, 0);

                int sec = atoi(cmd + 7);
                //sscanf(cmd + 7, "%d", &sec);
                printf("The client starts to sleep.\n");
                for(int sl = 1; sl <= sec; sl++)
                {
                    sleep(1);
                    printf("sleep %d\n", sl);
                }
                printf("Client wakes up.\n");

				memset(writebuf, 0, MAXLINE);
				strcpy(writebuf, "w");
				send(sockfd, writebuf, MAXLINE, 0);
            }
            else if(cmd[1] == 'p') //put
            {
                char file_name[MAXLINE];
				memset(file_name, 0, MAXLINE);
				strcat(file_name, cmd + 5);
				file_name[strlen(file_name) - 1] = '\0';
                int fdesc = open(file_name,  O_RDWR);

                memset(writebuf, 0, sizeof(writebuf));
                writebuf[0] = 'r';
                strcat(writebuf + 1, file_name);
                send(sockfd, writebuf, MAXLINE, 0);
                //printf("file%d: %s\n", strlen(file_name), file_name);

                printf("[Upload] %s Start!\n", file_name);
                
                int ndata;
                //int name_len = 2 + strlen(file_name);
                memset(writebuf, 0, sizeof(writebuf));

                //int ch = 0;
                while(true)//((ndata = read(fdesc, writebuf + name_len, MAXLINE - name_len)) > 0)
                {
                	usleep(10);
                    ndata = read(fdesc, writebuf + name_len, MAXLINE - name_len);
                    //cout << "ch: " << ch++ << "\n";
                    writebuf[0] = 'p';
                    strcat(writebuf + 2, file_name);
                    send(sockfd, writebuf, ndata + name_len, 0);
                    memset(writebuf, 0, sizeof(writebuf));

                    if(ndata < MAXLINE - name_len)
                    {
                        //ch = 0;
                        break;
                    }
                }
                close(fdesc);
                printf("Progress : [######################]\n");
                printf("[Upload] %s Finish!\n", file_name);
            }
            // len = strlen(writebuf);
            // ret = write(sockfd, writebuf, len);
            // if(ret == 0)
            // {
            //     printf("server close3\n");
            //     break;
            // }
            // else if(-1 == ret)
            // {
            //     perror("write");
            //     break;
            // }

            // memset(writebuf, 0, sizeof(writebuf));    
        }
    }

	// while(1)
	// {
	// 	printf("send msg to server: \n");
	// 	cin >> cmd;
	// 	if(cmd == "exit")
	// 	{
	// 		break;
	// 	}

	// 	strcpy(sendline, cmd.c_str());
	// 	if( send(sockfd, sendline, strlen(sendline), 0) < 0){
	// 		printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
	// 		return 0;
	// 	}
	// 	n = recv(sockfd, recvline, 4096, 0);
	// 	recvline[n] = '\0';
	// 	printf("recv msg from server: %s\n", recvline);
	// }

	close(sockfd);
	return 0;
}

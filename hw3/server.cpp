#include <iostream>
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <arpa/inet.h>
#include <fcntl.h> /* Added for the nonblocking socket */
#include <map>
#include <string>
#include <sys/stat.h>
#include <netdb.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <set>
#include <utility>

using namespace std;

#define MAXLINE 4096
#define name_len 64
#define BACKLOG 128     /* how many pending connections queue will hold */
map<int, string> namap;
map<int, int> isleep;
map<string, pair<set<int>, map<string, map<int, int> > > > dropbox;

int main(int argc, char *argv[]){
    //int ch;
    int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
    struct 	sockaddr_in ser_addr;    /* my address information */
    struct 	sockaddr_in cli_addr; /* connector's address information */
    socklen_t addr_size;
	char  readbuf[MAXLINE], writebuf[MAXLINE];
	int n, i;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;         /* host byte order */
    ser_addr.sin_port = htons(atoi(argv[1]));     /* short, network byte order */
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* auto-fill with my IP */

    if (bind(sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    int flag = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flag|O_NONBLOCK); /* Change the socket into non-blocking state	*/

    addr_size = sizeof(cli_addr);
    vector<int> leave_cli;
	while(1){
		//usleep(10000);
		map<int, string>::iterator fds_iter;
		map<string, pair<set<int>, map<string, map<int, int> > > >::iterator drop_iter;
		// printf("----------------------print stat-------------------\n");
		// for(fds_iter = namap.begin(); fds_iter != namap.end(); fds_iter++) {
		// 	printf("%d\t%s\tif sleep :%d\n", fds_iter->first, fds_iter->second.c_str(), isleep[fds_iter->first]);
		// }
		// for(drop_iter = dropbox.begin(); drop_iter != dropbox.end(); drop_iter++) {
		// 	printf("user :%s\n", drop_iter->first.c_str());
		// 	printf("\tat fds :");
		// 	set<int>::iterator set_iter;
		// 	for(set_iter = drop_iter->second.first.begin(); set_iter != drop_iter->second.first.end(); set_iter++) {
		// 		printf("%d, ", *set_iter);
		// 	}
		// 	printf("\n");
		// 	printf("\thas files :\n");
		// 	map<string, map<int, int> >::iterator file_iter;
		// 	for(file_iter = drop_iter->second.second.begin(); file_iter != drop_iter->second.second.end(); file_iter++) {
		// 		printf("\t\t%s :\n", file_iter->first.c_str());
		// 		map<int, int>::iterator files_stat_iter;
		// 		for(files_stat_iter = file_iter->second.begin(); files_stat_iter != file_iter->second.end(); files_stat_iter++) {
		// 			printf("\t\t\t%d, %d\n", files_stat_iter->first, files_stat_iter->second);
		// 		}
		// 	}
		// }
		// printf("---------------------------------------------------\n");

		for(drop_iter = dropbox.begin(); drop_iter != dropbox.end(); drop_iter++) {
			map<string, map<int, int> >::iterator file_iter;
			for(file_iter = drop_iter->second.second.begin(); file_iter != drop_iter->second.second.end(); file_iter++) {
                map<int, int>::iterator upload_iter;
				bool isuploading = true;
				for(upload_iter = file_iter->second.begin(); upload_iter != file_iter->second.end(); upload_iter++) {
					if(upload_iter->second == 0)
                    {
						isuploading = false;
					}
				}
                if(isuploading)
                {
                    set<int>::iterator set_iter;
                    for(set_iter = drop_iter->second.first.begin(); set_iter != drop_iter->second.first.end(); set_iter++) {
                        if(file_iter->second.find(*set_iter) == file_iter->second.end()) {
                            //printf("need to send file %s to fd %d\n", file_iter->first.c_str(), *set_iter);
                            if(isleep[*set_iter] == 1) {
                                //printf("but it is sleeping\n");
                            } else {
                                char file_name[MAXLINE];
                                memset(file_name, 0, MAXLINE);
                                strcat(file_name, file_iter->first.c_str());
                                memset(writebuf, 0, MAXLINE);
                                strcat(writebuf, "[Download] ");
                                strcat(writebuf, file_name);
                                strcat(writebuf, " Start!\n");
                                send(*set_iter, writebuf, MAXLINE, 0);

                                memset(writebuf, 0, sizeof(writebuf));
                                writebuf[0] = 'r';
                                strcat(writebuf + 2, file_name);
                                send(*set_iter, writebuf, MAXLINE, 0);

                                char user_name[MAXLINE];
                                char path[MAXLINE];
                                memset(user_name, 0, MAXLINE);
                                memset(path, 0, MAXLINE);
                                strcat(user_name, drop_iter->first.c_str());
                                sprintf(path, "./%s/%s", user_name, file_name);
                                int fdesc = open(path,  O_RDWR);
                                //cout << "path: " << path << "\n";
                                //printf("path: %s\n", path);
                                
                                int ndata;
                                //int name_len = 2 + strlen(file_name);
                                memset(writebuf, 0, sizeof(writebuf));
                                //int ch2 = 0;
                                while(true)//((ndata = read(fdesc, writebuf + name_len, MAXLINE - name_len)) > 0)
                                {
                                	usleep(10);
                                    ndata = read(fdesc, writebuf + name_len, MAXLINE - name_len);
                                    //cout << "read: " << writebuf[name_len] << "\n";
                                    //cout << "ch2: " << ch2++ << "\n";
                                    writebuf[0] = 'd';
                                    strcat(writebuf + 2, file_name);
                                    send(*set_iter, writebuf, ndata + name_len, 0);
                                    memset(writebuf, 0, sizeof(writebuf));

                                    if(ndata < MAXLINE - name_len)
                                    {
                                        break;
                                    }
                                }
                                close(fdesc);

                                file_iter->second[*set_iter] = 1;
                            }
                        }
                    }
                }
			}
		}



		// printf("----------------------print stat-------------------\n");
		map<int, string>::iterator iter;
		// for(iter = namap.begin(); iter != namap.end(); iter++) {
		// 	printf("%d\t%s\n", iter->first, iter->second.c_str());
		// }
		// printf("---------------------------------------------------\n");
		new_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &addr_size);
		// printf("%d\n", new_fd);
		if(new_fd != -1) {
			namap[new_fd] = "anonymous";
			fcntl(new_fd, F_SETFL, O_NONBLOCK);
		}
		for(iter = namap.begin(); iter != namap.end(); iter++) {
			int fd = iter->first;
			int recn;
			memset(readbuf, 0, sizeof(readbuf));
			recn = recv(fd, readbuf, MAXLINE, 0);
            //printf("rec: %c\n", readbuf[0]);
			if(recn != -1) {
				if(recn == 0) {
                    leave_cli.push_back(fd);
                    dropbox[namap[fd]].first.erase(fd);
					isleep.erase(fd);
					//namap.erase(fd);
				}
                else if(readbuf[0] == 'n') {
					string name = readbuf + 2;
                    //printf("name: %s\n", name);

                    int status;
                    if((status = mkdir(name.c_str() , S_IRWXU | S_IRWXG | S_IRWXO)) < 0)
                    {
                        //printf("user existed\n");
                    }

                    namap[fd] = name;
                    isleep[fd] = 0;
                    dropbox[namap[fd]].first.insert(fd);
                    //cout << "name: " << name << "\n";
				}
                else if(readbuf[0] == 'r')
                {
                    char file_name[MAXLINE];
                    char user_name[MAXLINE];
                    char path[MAXLINE];
                    memset(file_name, 0, MAXLINE);
                    memset(user_name, 0, MAXLINE);
                    memset(path, 0, MAXLINE);
                    strcpy(file_name, readbuf + 1);
                    strcpy(user_name, namap[fd].c_str());
                    sprintf(path, "./%s/%s", user_name, file_name);
                    FILE *fff = fopen(path, "w");
                    fclose(fff);
                    dropbox[namap[fd]].second[file_name].clear();
                    dropbox[namap[fd]].second[file_name][fd] = 0;
                    //printf("remove %s\n", file_name);
				}
                else if(readbuf[0] == 'p')
                {
                    //cout << "ch: " << ch++ << "\n";
                    char file_name[MAXLINE];
                    char user_name[MAXLINE];
                    char path[MAXLINE];
                    memset(file_name, 0, MAXLINE);
                    memset(user_name, 0, MAXLINE);
                    memset(path, 0, MAXLINE);
                    strcpy(file_name, readbuf + 2);
                    strcpy(user_name, namap[fd].c_str());
                    sprintf(path, "./%s/%s", user_name, file_name);
                    //cout << "path: " << path << "\n";

                    //int name_len = 2 + file_name.length();
                    FILE *fff = fopen(path, "a");
                    fwrite(readbuf + name_len, 1, recn - name_len, fff);
                    fclose(fff);
                    // int fdesc = open(path, O_CREAT | O_APPEND | O_RDWR);
                    // int ndata;
                    // if((ndata = write(fdesc, readbuf + name_len, recn-name_len)) < 0)
                    // {
                    //     perror("write");
                    //     //return 0;
                    // }
                    // close(fdesc);
                    if(recn != MAXLINE) {
                        dropbox[namap[fd]].second[file_name][fd] = 1;
                    }
                }
                else if(readbuf[0] == 's')
                {
					isleep[fd] = 1;
				}
                else if(readbuf[0] == 'w')
                {
					isleep[fd] = 0;
				}
			}
		}
        for(vector<int>::iterator i=leave_cli.begin(); i!=leave_cli.end(); i++)
        {
            namap.erase(*i);
        }
        leave_cli.clear();


        //////////
		// for (i=sockfd; i<=last_fd; i++){
		// 	//printf("Round number %d\n",i);
       	// 	if (i == sockfd){
		//  		sin_size = sizeof(struct sockaddr_in);
        //         if ((new_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &sin_size)) == -1) {
        //             perror("accept");
        //             exit(1);
        //         }
        //         printf("server: got connection from %s\n", inet_ntoa(cli_addr.sin_addr)); 
        //         fcntl(new_fd, F_SETFL, O_NONBLOCK);
		// 		last_fd = new_fd;
		// 	}
		// 	else{
	    // 		n=recv(i,readbuf,sizeof(readbuf),0);
		// 		if (n < 1){ 
		// 			perror("recv - non blocking \n");
	    // 			printf("Round %d, and the data read size is: n=%d \n",i,n);
		// 		}
		// 		else{
        //             cout << "cmd: " << readbuf;
        //             char *cmd = strtok(readbuf, " ");
        //             if(strcmp(cmd, "name") == 0)
        //             {
        //                 cmd = strtok(NULL, " ");
        //                 namap[i] = cmd;
        //                 cout << cmd << "\n";

        //                 string name = cmd;
        //                 bool find = true;
        //                 for (map<int,string>::iterator it=namap.begin(); it!=namap.end(); ++it)
        //                 {
        //                     if(name == it->second)
        //                     {
        //                         find = false;
        //                         break;
        //                     }
        //                 }
        //                 if(find)
        //                 {
        //                     cout << "fffff\n";
        //                 }
        //             }
		// 		}
	    // 	}
		// }
	}
}

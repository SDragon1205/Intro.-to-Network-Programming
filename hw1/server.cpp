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
#include <map>
#include <string>

using namespace std;

struct FdInfo
{
	char ip[256];
	char name[256];
	int port;
};


map< int, string > fds_ip;
map< int, string > fds_port;
map< int, string > fds_name;

map< int, FdInfo > fds;

char* getip(int fd) {
	socklen_t len;
	struct sockaddr_storage addr;
	static char ipstr[INET6_ADDRSTRLEN];
	int port;
	len = sizeof(addr);
	getpeername(fd, (struct sockaddr*)&addr, &len);
	struct sockaddr_in *s = (struct sockaddr_in *)&addr;
	port = ntohs(s->sin_port);
	inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	return ipstr;
}

int getport(int fd) {
	socklen_t len;
	struct sockaddr_storage addr;
	char ipstr[INET6_ADDRSTRLEN];
	int port;
	len = sizeof(addr);
	getpeername(fd, (struct sockaddr*)&addr, &len);
	struct sockaddr_in *s = (struct sockaddr_in *)&addr;
	port = ntohs(s->sin_port);
	inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	return port;
}

int main(int argc, char *argv[]) {
	if( argc != 2){
		printf("usage: ./server <port>\n");
		return 0;
	}
	////////////////////////////// select set
	fd_set master; // master file descriptor 清單 
	fd_set read_fds; // 給 select() 用的暫時 file descriptor 清單
	FD_ZERO(&master); // 清除 master 與 temp sets
  	FD_ZERO(&read_fds);
	int fdmax; // 最大的 file descriptor 數目
	////////////////////////////// socket set
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo; // 將指向結果
	memset(&hints, 0, sizeof(hints) ); // 確保 struct 為空
	hints.ai_family = AF_UNSPEC; // 不用管是 IPv4 或 IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE; // 幫我填好我的 IP 
	// 準備好連線
	status = getaddrinfo(
		NULL,
		argv[1],
		&hints,
		&servinfo
	);

	int listener = socket(
		servinfo->ai_family,
		servinfo->ai_socktype,
		servinfo->ai_protocol
	);

	status = bind(
		listener,
		servinfo->ai_addr,
		servinfo->ai_addrlen
	);

	status = listen(
		listener,
		10
	);

	FD_SET(listener, &master);
  // 持續追蹤最大的 file descriptor
	fdmax = listener; // 到此為止，就是它了

	int new_fd;
	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	addr_size = sizeof(client_addr);


	while (1) {
    	read_fds = master; // 複製 master
    	if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      		perror("select");
      		exit(4);
    	}
    	for (int i = 0; i <= fdmax; i++) {
      		if (FD_ISSET(i, &read_fds)) { // 我們找到一個！！
				if (i == listener) {
					// handle new connections
					addr_size = sizeof(client_addr);
					if ( (new_fd = accept(
						listener,
						(struct sockaddr *)&client_addr,
						&addr_size
					) ) == -1) {
						perror("accept wrong");
					} else {
						FD_SET(new_fd, &master); // 新增到 master set
						if (new_fd > fdmax) { // 持續追蹤最大的 fd
							fdmax = new_fd;
						}
						printf("new socket : %d\tip : %s\tport : %d\n", new_fd, getip(new_fd), getport(new_fd));
						///-----------to new
						char sendbuf[1024];
						sprintf(sendbuf, "[Server] Hello, anonymous! From: %s:%d", getip(new_fd), getport(new_fd));
						send(new_fd, sendbuf, 1024, 0);
						///----------------------to exist
						for (int j = 0; j <= fdmax; j++) { // 送給大家！
							if (FD_ISSET(j, &master)) { // 不用送給 listener 跟我們自己
								if (j != new_fd) {
									if (send(j, "[Server] Someone is coming!", 256, 0) == -1) {
										perror("send");
									}
								}
							}
						}
						//////---------init fd info
						struct FdInfo newfdinfo;
						sprintf(newfdinfo.name, "anonymous");
						sprintf(newfdinfo.ip, "%s", getip(new_fd));
						newfdinfo.port = getport(new_fd);
						fds[new_fd] = newfdinfo;
					}
				} else {// 處理來自 client 的資料
					int nrecbytes;
					char recbuf[1024];
					if ((nrecbytes = recv(i, recbuf, sizeof(recbuf), 0)) <= 0) {
						if (nrecbytes == 0) {
							printf("selectserver: socket %d hung up\n", i);
							for (int j = 0; j <= fdmax; j++) { // 送給大家！
								if (FD_ISSET(j, &master) && j != i) { // 不用送給 listener 跟我們自己 j != i
									char ex_msg[256];
									sprintf(ex_msg, "[Server] %s is offline.", fds[i].name);
									if (send(j, ex_msg, 256, 0) == -1) {
										printf("%d  ", j);
										perror("send");
									}
								}
							}
						} else {
							perror("recv wrong");
						}
						close(i); // bye!
						FD_CLR(i, &master); // 從 master set 中移除
					} else { // 我們從 client 收到一些資料
						printf("recbuf : %s\n", recbuf);
						char rectemp[1024];
						strcpy(rectemp, recbuf);
						char *cmd = strtok(rectemp, " ");
						// printf("cmd : %s\n", cmd);
						if (! strcmp(cmd, "name")) {
							bool lenvalid = true;
							bool univalid = true;
							bool anovalid = true;
							char newname[256];
							strcpy(newname, recbuf + 5);
							newname[strlen(newname) - 1] = '\0';
							lenvalid = (
								lenvalid & 
								(strlen(newname) > 1) &
								(strlen(newname) < 13)
							);
							for(int idx = 0; idx < strlen(newname); idx++) {
								lenvalid = lenvalid & isalpha(newname[idx]);
							}
							map<int, FdInfo>::iterator iter;
							for(iter = fds.begin(); iter != fds.end(); iter++) {
								univalid = univalid & (strcmp(iter->second.name, newname) != 0);
							}
							anovalid = ( strcmp(newname, "anonymous") != 0 );
							if (anovalid) {
								if (lenvalid) {
									if (univalid) {
										char oldname[256];
										strcpy(oldname, fds[i].name);
										strcpy(fds[i].name, newname);
										char chname_msg[256];
										sprintf(chname_msg, "[Server] You're now known as %s.", newname);
										send(i, chname_msg, 256, 0);
										for (int j = 0; j <= fdmax; j++) { // 送給大家！
											if (FD_ISSET(j, &master) && j != i) { // 不用送給 listener 跟我們自己 j != i
												char chname_ann_msg[256];
												sprintf(chname_ann_msg, "[Server] %s is now known as %s.", oldname, newname);
												if (send(j, chname_ann_msg, 256, 0) == -1) {
													printf("%d  ", j);
													perror("send");
												}
											}
										}
									} else {
										char overlap_msg[256];
										sprintf(overlap_msg, "[Server] ERROR: %s has been used by others.", newname);
										send(i, overlap_msg, 256, 0);
									}
								} else {
									send(i, "[Server] ERROR: Username can only consists of 2~12 English letters.", 256, 0);
								}
							} else {
								send(i, "[Server] ERROR: Username cannot be anonymous.", 256, 0);
							}
							printf("name cmd : %s\n", newname);
						} else if (! strcmp(cmd, "yell")) {
							for (int j = 0; j <= fdmax; j++) { // 送給大家！
								if (FD_ISSET(j, &master)) { // 不用送給 listener 跟我們自己
									if (j != listener && j != i) {
										char yell_ann_msg[1024];
										sprintf(yell_ann_msg, "[Server] %s yell %s", fds[i].name, recbuf + 5);
										if (send(j, yell_ann_msg, 1024, 0) == -1) {
											perror("send");
										}
									}
								}
							}
						} else if (! strcmp(cmd, "tell")) {
							strcpy(rectemp, recbuf + 5);
							printf("----------------------%s", rectemp);
							char *sendto = strtok(rectemp, " ");
							bool isbadname = false;
							if ( ! strcmp(fds[i].name, "anonymous")) {
								isbadname = true;
								if (send(i, "[Server] ERROR: You are anonymous.", 1024, 0) == -1) {
									perror("send");
								}
							}
							if ( ! strcmp(sendto, "anonymous")) {
								isbadname = true;
								if (send(i, "[Server] ERROR: The client to which you sent is anonymous.", 1024, 0) == -1) {
									perror("send");
								}
							}
							if (!isbadname) {
								bool tellsuc = false;
								for (int j = 0; j <= fdmax; j++) {
									if (FD_ISSET(j, &master)) {
										if ( ! strcmp(fds[j].name, sendto)) {
											tellsuc = true;
											send(i, "[Server] SUCCESS: Your message has been sent.", 256, 0);
											char tell_msg[1024];
											sprintf(tell_msg, "[Server] %s tell you %s", fds[i].name, recbuf + 5 + strlen(sendto) + 1);
											
											tell_msg[strlen(tell_msg) - 1] = '\0';
											printf("-----------%s\n", tell_msg);
											if (send(j, tell_msg, 1024, 0) == -1) {
												perror("send");
											}
										}
									}
								}
								if(! tellsuc) {
									if (send(i, "[Server] ERROR: The receiver doesn't exist.", nrecbytes, 0) == -1) {
										perror("send");
									}
								}
							}
						} else if (! strcmp(cmd, "who\n")) {
							for (int j = 0; j <= fdmax; j++) {
								if (FD_ISSET(j, &master)) {
									if (fds[j].port) {
										char who_msg[1024];
										sprintf(who_msg, "[Server] %s %s:%d ->me", fds[j].name, getip(new_fd), fds[j].port);
										if (i != j) {
											who_msg[strlen(who_msg) - 5] = '\0';
										}
										if (send(i, who_msg, 1024, 0) == -1) {
											perror("send");
										}
									}
								}
							}
						} else {
							if (send(i, "[Server] ERROR: Error command.", 1024, 0) == -1) {
								perror("send");
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

using namespace std;

static void sig_handler (int sig) {
    int retval;
    if (sig == SIGCHLD) {
        wait(&retval);
        //printf("CATCH SIGNAL PID=%d\n", getpid());
    }
}

string spliter (char *raw, char sc, int b, int e) {
    string result = "";
    char line[1024];
    strcpy(line, raw);
    char s[3] = {sc, '\n', '\0'};
    char *token = strtok(line, s);
    for (int i = 0; token != NULL && i != e; i++) {
        if (i >= b && i != e) {
            if (i != b) 
                result += sc;
            result += token;
        }
        token = strtok(NULL, s);
    }
    return result;
}

void ss0(int fd, string msg, int len, string errmsg) {
    if (send(fd, msg.c_str(), len, 0) == -1) {
        perror(errmsg.c_str());
    }
    return;
}

char* getip(int fd) {
    socklen_t len;
    struct sockaddr_storage addr;
    static char ipstr[INET6_ADDRSTRLEN];
    int port;
    len = sizeof(addr);
    getpeername(fd, (struct sockaddr*)&addr, &len);
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    port = ntohs(s->sin_port);
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
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
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
    return port;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage ./servert <port>\n");
        return 0;
    }
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

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
    fcntl(listener, F_SETFL, O_NONBLOCK);
    status = bind(
        listener,
        servinfo->ai_addr,
        servinfo->ai_addrlen
    );
    status = listen(
        listener,
        15
    );

    int fdsrecbuf_shmid = shmget(0, sizeof(char) * 15 * 1024, IPC_CREAT|0666);
    char* fdsrecbuf_p = (char*)shmat(fdsrecbuf_shmid, NULL, 0);
    memset(fdsrecbuf_p, 0, sizeof(char) * 15 * 1024);

    int fds_shmid = shmget(0, sizeof(int) * 15, IPC_CREAT|0666);
    int* fds_p = (int*)shmat(fds_shmid, NULL, 0);
    memset(fds_p, -1, sizeof(int) * 15);

    int isac_shmid = shmget(0, sizeof(bool) * 1, IPC_CREAT|0666);
    bool* isac_p = (bool*)shmat(isac_shmid, NULL, 0);
    memset(isac_p, false, sizeof(bool) * 1);

    int fdsishold_shmid = shmget(0, sizeof(bool) * 15, IPC_CREAT|0666);
    bool* fdsishold_p = (bool*)shmat(fdsishold_shmid, NULL, 0);
    memset(fdsishold_p, false, sizeof(bool) * 15);

    while (1) {
        int new_fd;
        struct sockaddr_storage client_addr;
        socklen_t addr_size = sizeof(client_addr);
        if ( (new_fd = accept(listener, (struct sockaddr *)&client_addr, &addr_size)) == -1) {
            // perror("accept");
            ;
        } else {
            for (int idx = 0; idx < 15; idx++) {
                if (fds_p[idx] == -1) {
                    fds_p[idx] = new_fd;
                    strcpy(fdsrecbuf_p + idx * 1024, "hello");
                    fcntl(new_fd, F_SETFL, O_NONBLOCK);
                    break;
                }
            }
        }
        for (int idx = 0; idx < 15; idx++) {
            if (fds_p[idx] != -1) {
                char recbuf[1024];
                int nrec;
                if ( (nrec = recv(fds_p[idx], recbuf, sizeof(recbuf), 0)) <= 0) {
                    if (nrec == 0) {
                        printf("%d left", fds_p[idx]);
                        close(fds_p[idx]);
                    }
                } else {
                    printf("%d yell :%s\n", fds_p[idx], recbuf);
                    for (int j = 0; j < 15; j++) {
                        if (fds_p[j] != -1 && j != idx) {
                            ss0(fds_p[j], recbuf, 1024, "yell");
                        }
                    }
                }
            }
        }
    }
}

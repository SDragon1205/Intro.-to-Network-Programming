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

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage ./client <ipaddress> <port>\n");
        return 0;
    }
    fd_set read_fds;
    FD_ZERO(&read_fds);
    int fdmax;
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(
        argv[1],
        argv[2],
        &hints,
        &servinfo
    );
    int receiver = socket(
        servinfo->ai_family,
        servinfo->ai_socktype,
        servinfo->ai_protocol
    );
    status = connect(
        receiver,
        servinfo->ai_addr,
        servinfo->ai_addrlen
    );
    while (1) {
        FD_SET(fileno(stdin), &read_fds);
        FD_SET(receiver, &read_fds);
        fdmax = receiver;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        if (FD_ISSET(fileno(stdin), &read_fds)) {
            char buf[1024];
            memset(buf, 0, 1024);
            read(fileno(stdin), buf, 1024);
            if (! strcmp(buf, "exit\n")) {
                return 0;
            }
            send(receiver, buf, 1024, 0);
        }
        if (FD_ISSET(receiver, &read_fds)) {
            int nrecbytes;
            char recbuf[1024];
            if ((nrecbytes = recv(receiver, recbuf, sizeof(recbuf), 0)) <= 0) {
                if (nrecbytes == 0) {
                    printf("server hung up\n");
                    break;
                } else {
                    perror("recv");
                }
            } else {
                printf("%s\n", recbuf);
            }
        }
    }

    close(receiver);

    return 0;
}
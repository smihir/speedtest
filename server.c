#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "packet.h"
#include "common.h"

#define DEFAULT_PORT 9000
#define BACKLOG 50

void s_run(unsigned int port) {
    int fd;
    struct sockaddr_in my;

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("Socket open failed");
        return;
    }

    my.sin_addr.s_addr = INADDR_ANY;
    my.sin_family = AF_INET;
    my.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *)&my, sizeof(my)) != 0) {
        perror("Socket bind failed");
        return;
    }

    if (listen(fd, BACKLOG) == -1) {
        perror("Cannot listen");
        return;
    }

    struct sockaddr_storage their;
    socklen_t sz = sizeof(their);
    int confd = accept(fd, (struct sockaddr *)&their, &sz);

    if (confd == -1) {
        perror("cannot accept connection");
        return;
    }
    sleep(1);

    //run_rx_test(confd);
    run_tx_test(confd);
}

int main(int argc, char **argv) {
    int ch;
    unsigned long int port = DEFAULT_PORT;

    while ((ch = getopt(argc, argv, "p:")) != -1) {
        switch (ch) {
            case 'p':
                port = strtoul(optarg, NULL, 10);
                if (port <= 1024 || port > 65536) {
                    printf("Invalid Port\n");
                    exit(1);
                }
                break;
            case '?':
            default:
                printf("Not enough arguments\n");
        }
    }

    printf("port is %lu\n", port);
    s_run(port);

    return 0;
}

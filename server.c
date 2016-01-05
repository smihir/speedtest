#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "packet.h"

#define DEFAULT_PORT 9000
#define BACKLOG 50
#define MAXPKTS 10000

void s_run_rx_test(int confd) {
    struct timeval tv;
    char *buf = malloc(RX_BUFSIZE);

    if (buf == NULL) {
        printf("Cannot allocate memory for Rx\n");
        exit(1);
    }

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(confd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
    }

    printf("rx_test\n");
    while (1) {
        int len;
        struct packet_header *hdr;

        len = recv(confd, buf, RX_BUFSIZE, 0);
        if (len == -1) {
            if (errno == EAGAIN) {
                free(buf);
                close(confd);
                return;
            }
            perror("Error Receiving Packet");
            continue;
        } else if (len == 0) {
            printf("Connection Closed\n");
            free(buf);
            close(confd);
            return;
        }

        hdr = (struct packet_header *)buf;
        
        printf(".");
        if (hdr->type == T_END) {
            printf("Done\n");
            break;
        }
    }
    printf("\n");

    free(buf);
}

void s_run_tx_test(int confd) {
    char *buffer;
    int sz = TX_BUFSIZE;
    int numpkts = MAXPKTS;
    struct packet_header *hdr;

    printf("tx_test\n");
    buffer = (char *) malloc(sizeof(char) * sz);
    if (buffer == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    hdr = (struct packet_header *)buffer;
    hdr->type = T_DATA;

    while (numpkts--) {
        if (numpkts == 0) {
            hdr->type = T_END;
        }

        printf(".");
        if (send(confd, buffer, TX_BUFSIZE, 0) == -1) {
            if (errno == EBADF) {
                free(buffer);
                close(confd);
                return;
            }
            perror("send");
            printf("%u\n", errno);
        }
    }
    printf("\n");
}

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

    s_run_rx_test(confd);
    s_run_tx_test(confd);
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

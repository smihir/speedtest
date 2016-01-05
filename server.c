#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "packet.h"
#include "common.h"

#define DEFAULT_PORT 9000
#define BACKLOG 50

int send_ack(int confd) {
    char *buffer;
    int sz = TX_BUFSIZE;
    struct packet_header *hdr;

    printf("Acknowledge...\n");

    buffer = (char *) malloc(sizeof(char) * sz);
    if (buffer == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    hdr = (struct packet_header *)buffer;
    hdr->type = T_SERVER_ACK;

    while (1) {
        int len;
        if ((len = send(confd, buffer, TX_BUFSIZE, 0)) <= 0) {
            return 0;
        }
        return 1;
    }
}

void send_summarystats(int confd, char *s) {
    char *buffer;
    int sz = TX_BUFSIZE;
    struct packet_header *hdr;
    int retries = 5;

    printf("Send Summary Stats...\n");

    buffer = (char *) malloc(sizeof(char) * sz);
    if (buffer == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    hdr = (struct packet_header *)buffer;
    hdr->type = T_SUMMARYSTATS;
    hdr->data.stats.len = htonl((unsigned long)(strlen(s) + 1));
    memcpy(&hdr->data.stats.msg[0], s, strlen(s) + 1);

    while (retries > 0) {
        int len;
        if ((len = send(confd, buffer, TX_BUFSIZE, 0)) <= 0) {
            perror("Send Summary Stats Failed:");
            retries--;
            continue;
        }
        printf("Done...\n");
        return;
    }
}

void process_ctr_msg(int confd) {
    struct timeval tv;
    char *buf = malloc(RX_BUFSIZE);
    char *s = NULL;
    int cum_len = 0;

    if (buf == NULL) {
        printf("Cannot allocate memory for Rx\n");
        exit(1);
    }

    tv.tv_sec = 60;
    tv.tv_usec = 0;
    if (setsockopt(confd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
    }

    printf("Waiting for Client...\n");
    while (1) {
        int len;
        struct packet_header *hdr;

        len = recv(confd, &buf[cum_len], RX_BUFSIZE - cum_len, 0);
        if (len == -1) {
            if (errno == EAGAIN) {
                printf("Timed out waiting for commands from client\n");
                close(confd);
                break;
            }
            continue;
        } else if (len == 0) {
            printf("Connection Closed\n");
            close(confd);
            break;
        }

        cum_len += len;
        if (cum_len < RX_BUFSIZE)
            continue;
        cum_len = 0;

        hdr = (struct packet_header *)buf;
        
        printf("Received Control Message: %c\n", hdr->type);
        switch (hdr->type) {
            case T_UPLOAD_TEST:
                printf("Ready for Upload Test...\n");
                if (send_ack(confd))
                    s = run_rx_test(confd, 1);
                if (s != NULL) {
                    send_summarystats(confd, s);
                    free(s);
                } else {
                    printf("Error! will not send summary stats\n");
                }
            break;

            case T_DOWNLOAD_TEST:
                printf("Ready for Download Test...\n");
                if (send_ack(confd))
                    run_tx_test(confd);
            break;
        }
    }

    free(buf);
}

void s_run(unsigned int port) {
    int fd;
    struct sockaddr_in my;
    int flag = 1;

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("Socket open failed");
        return;
    }

    // So that we dont have to wait for around 1min for OS
    // to recycle the socket
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag,
                    sizeof(int)) == -1) { 
        perror("setsockopt"); 
        exit(1); 
    } 

    my.sin_addr.s_addr = INADDR_ANY;
    my.sin_family = AF_INET;
    my.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *)&my, sizeof(my)) != 0) {
        perror("Socket bind failed");
        return;
    }

    // We want the send packet size to be no more than
    // what we specify in send() call so set NODELAY flag.
    // The size requirement is because we are trying to
    // measure througput for a particular packet size
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                         sizeof(int)) == -1) {
        printf("Cannot disable Nagle! Exit\n");
        exit(1);
    }

    if (listen(fd, BACKLOG) == -1) {
        perror("Cannot listen");
        return;
    }

    while (1) {
        struct sockaddr_storage their;
        socklen_t sz = sizeof(their);
        int confd = accept(fd, (struct sockaddr *)&their, &sz);

        if (confd == -1) {
            perror("cannot accept connection");
            return;
        }

        if (setsockopt(confd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                            sizeof(int)) == -1) {
            printf("Cannot disable Nagle! Exit\n");
            exit(1);
        }

        process_ctr_msg(confd);
    }

    close(fd);
}

int main(int argc, char **argv) {
    int ch;
    unsigned long int port = DEFAULT_PORT;

    // make provision to specify the port in case
    // DEFAULT_PORT(9000) does not work
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

    printf("Operating on port %lu\n", port);
    s_run(port);

    return 0;
}

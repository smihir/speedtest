#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "packet.h"

#define MAXPKTS 10000

void run_rx_test(int confd) {
    struct timeval tv;
    int total_len = 0;
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
            continue;
            printf("Connection Closed\n");
            free(buf);
            close(confd);
            return;
        }

        total_len += len;
        hdr = (struct packet_header *)buf;
        
        printf(".");
        if (hdr->type == T_END) {
            printf("Done\n");
            break;
        }
    }
    printf("total_len %d\n", total_len);

    free(buf);
}

void run_tx_test(int confd) {
    char *buffer;
    int sz = TX_BUFSIZE;
    int numpkts = MAXPKTS;
    struct packet_header *hdr;
    int total_len = 0;

    printf("tx_test\n");
    buffer = (char *) malloc(sizeof(char) * sz);
    if (buffer == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    hdr = (struct packet_header *)buffer;
    hdr->type = T_DATA;

    while (numpkts--) {
        int len;
        if (numpkts == 0) {
            hdr->type = T_END;
        }

        //printf(".");
        if ((len = send(confd, buffer, TX_BUFSIZE, 0)) == -1) {
            if (errno == EBADF) {
                free(buffer);
                close(confd);
                return;
            }
            perror("send");
            printf("%u\n", errno);
        }
        total_len += len;
    }
    printf("total_len %d\n", total_len);
}


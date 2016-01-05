#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "packet.h"

#define MAXPKTS 1000
#define BYTESTOMB 1024 * 1024
#define MAX_PRINT_LEN 200

double elapsed_time(struct timeval *tv_first, struct timeval *tv_end)
{
    double elapsed_msec;

    elapsed_msec = (tv_end->tv_sec - tv_first->tv_sec) * 1000;
    elapsed_msec += (tv_end->tv_usec - tv_first->tv_usec) / 1000;

    return elapsed_msec;
}

char *print_summarystats(struct timeval *tv_first, struct timeval *tv_end,
        unsigned int numpkts_rx, unsigned int numbytes_rx, int retstr)
{
    double elapsed_msec;
    float MBps;
    char *s = NULL;

    if (retstr)
        s = malloc(MAX_PRINT_LEN);

    elapsed_msec = elapsed_time(tv_first, tv_end);
    MBps = numbytes_rx / ((elapsed_msec / 1000) * BYTESTOMB);

    if (s != NULL) {
        snprintf(s, MAX_PRINT_LEN, "packets received: %u \nbytes_received: %u \n"
                "average packets per second: %f \naverage Mega Bytes per second: %f (%f Mbps)\n"
                "duration (ms): %f \n",
                numpkts_rx, numbytes_rx, numpkts_rx / (elapsed_msec / 1000),
                MBps, MBps * 8, elapsed_msec);
    } else {
        printf("packets received: %u \nbytes_received: %u \n"
               "average packets per second: %f \naverage Mega Bytes per second: %f (%f Mbps)\n"
               "duration (ms): %f \n",
               numpkts_rx, numbytes_rx, numpkts_rx / (elapsed_msec / 1000),
               MBps, MBps * 8, elapsed_msec);
    }
    return s;
}

char *run_rx_test(int confd, int retstr) {
    struct timeval tv, tv1, tv_first;
    unsigned int total_len = 0, num_pkts = 0;
    char *buf = malloc(RX_BUFSIZE);
    int cum_len = 0;

    if (buf == NULL) {
        printf("Cannot allocate memory for Rx\n");
        exit(1);
    }

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(confd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
    }

    printf("Running RX Test\n");
    while (1) {
        int len;
        struct packet_header *hdr;

        len = recv(confd, &buf[cum_len], RX_BUFSIZE - cum_len, 0);
        if (len == -1) {
            if (errno == EBADF) {
                perror("Error Receiving Packet Closing socket!");
                close(confd);
                break;
            }
            perror("Error Receiving Packet");
            break;
        } else if (len == 0) {
            printf("Connection Closed\n");
            close(confd);
            break;
        }

        total_len += len;
        num_pkts++;
        gettimeofday(&tv1, NULL);
        if (num_pkts == 1) {
            tv_first = tv1;
        }

        cum_len += len;
        if (cum_len < RX_BUFSIZE)
            continue;
        cum_len = 0;


        hdr = (struct packet_header *)buf;
        
        if (hdr->type == T_END) {
            break;
        }
    }
    return print_summarystats(&tv_first, &tv1, num_pkts, total_len, retstr);

    free(buf);
}

void run_tx_test(int confd) {
    char *buffer;
    int sz = TX_BUFSIZE;
    int numpkts = MAXPKTS;
    struct packet_header *hdr;
    int total_len = 0;

    printf("Running TX Test\n");
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

        if ((len = send(confd, buffer, TX_BUFSIZE, 0)) == -1) {
            if (errno == EBADF) {
                perror("Error Sending Packet Closing socket!");
                free(buffer);
                close(confd);
                return;
            }
            perror("Error Sending Packet");
        }
        total_len += len;
    }
}

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "packet.h"

#define MAXPKTS 10000

void c_usage() {
    printf("client --server-ip <ip address of server> --server-port <port of the server>\n");
    exit(1);
}

void c_run_rx_test(int sockfd) {
    struct timeval tv;
    char *buf = malloc(RX_BUFSIZE);

    if (buf == NULL) {
        printf("Cannot allocate memory for Rx\n");
        exit(1);
    }

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
    }

    printf("rx_test\n");
    while (1) {
        int len;
        struct packet_header *hdr;

        len = recv(sockfd, buf, RX_BUFSIZE, 0);
        if (len == -1) {
            if (errno == EAGAIN) {
                free(buf);
                close(sockfd);
                return;
            }
            perror("Error Receiving Packet");
            continue;
        }

        printf(".");
        hdr = (struct packet_header *)buf;
        
        if (hdr->type == T_END) {
            printf("Done\n");
            break;
        }
    }
    printf("\n");
}

void c_run_tx_test(int sockfd) {
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
        if (send(sockfd, buffer, TX_BUFSIZE, 0) == -1) {
            perror("send");
            printf("%u\n", errno);
        }
    }
    printf("\n");
}

void c_run(char *hostname, char *port) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET_ADDRSTRLEN];
    int socketfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;

        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else { // IPv6
            continue;
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("%s\n", ipstr);
        break;
    }

    socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (socketfd < 0) {
        perror("Socket open failed: ");
    }
    if (connect(socketfd, p->ai_addr, p->ai_addrlen) == -1) {
        close(socketfd);
        perror("client: connect");
        return;
    }

    c_run_tx_test(socketfd);
    c_run_rx_test(socketfd);
}

int main(int argc, char **argv) {
    int ch, index = 0;
    char *server_ip;
    char *port;
    unsigned int ip_set = 0, port_set = 0;
    static struct option client_options[] =
        {
            {"server-ip", required_argument, NULL, 's'},
            {"server-port", required_argument, NULL, 'p'},
        };

    while ((ch = getopt_long(argc, argv, ":", client_options, &index)) != -1) {
        switch (ch) {
            case 's':
                if (optarg) {
                    server_ip = strdup(optarg);
                    ip_set = 1;
                } else {
                    c_usage();
                }
                break;
            case 'p':
                if (optarg) {
                    port = strdup(optarg);
                    port_set = 1;
                } else {
                    c_usage();
                }
                break;
            case '?':
            default:
                c_usage();
        }
    }

    if (!ip_set || !port_set) {
        c_usage();
    }

    c_run(server_ip, port);

    return 0;
}

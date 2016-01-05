#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "packet.h"
#include "common.h"

void c_usage() {
    printf("client --server-ip <ip address of server> --server-port <port of the server>\n");
    exit(1);
}

void receive_summarystats(int confd) {
    struct timeval tv;
    char *buf = malloc(RX_BUFSIZE);
    int len, i;
    struct packet_header *hdr;
    unsigned int msglen;
    int retries = 5;

    if (buf == NULL) {
        printf("Cannot allocate memory for Rx\n");
        exit(1);
    }

    printf("Waiting for Summary Stats\n");
    tv.tv_sec = 15;
    tv.tv_usec = 0;
    if (setsockopt(confd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
    }

    while (retries) {
        len = recv(confd, buf, RX_BUFSIZE, 0);
        if (len == -1) {
            //nothing received for 5 sec, just break out...
            if (errno == EAGAIN) {
                perror("Timedout Receiving Summary Stats");
                retries = 0;
                break;
            }
            perror("Error Receiving Summary Stats");
            retries--;
            continue;
        } else if (len == 0) {
            printf("Connection Closed\n");
            close(confd);
            exit(1);
        }
    }

    if (retries == 0)
        return;

    hdr = (struct packet_header *)buf;
    
    printf("Received Control Message: %c\n", hdr->type);
    switch (hdr->type) {
        case T_SUMMARYSTATS:
             msglen = ntohl(hdr->data.stats.len);
             printf("%d %u\n", len, msglen);
            if (msglen > len - sizeof(struct packet_header)) {
                printf("Summary Stats corrupted\n");
                return;
            }

            for (i = 0; i < msglen; i++) {
                printf("%d", i);
                printf("%c\n", hdr->data.stats.msg[i]);
            }
            printf("\n");
            break;
    }

    free(buf);
}

int send_ctrl_msg(int confd, int msgtype, char *data, int len) {
    char *buffer;
    int sz = TX_BUFSIZE;
    struct packet_header *hdr;
    struct timeval tv;
    int retries = 5;

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(confd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error setting socket options...Continue");
    }

    buffer = (char *) malloc(sizeof(char) * sz);
    if (buffer == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    hdr = (struct packet_header *)buffer;
    hdr->type = msgtype;

    while (retries) {
        int len;
        char *buf = malloc(RX_BUFSIZE);

        if (buf == NULL) {
            printf("Cannot allocate memory for Rx\n");
            exit(1);
        }

        printf("Send Control Message: %c\n", msgtype);
        send(confd, buffer, TX_BUFSIZE, 0);

        len = recv(confd, buf, RX_BUFSIZE, 0);
        if (len == -1) {
            perror("Error Receiving Packet");
            free(buf);
            retries--;
            continue;
        } else if (len == 0) {
            printf("Connection Closed\n");
            close(confd);
            free(buffer);
            free(buf);
            exit(1);
        }

        hdr = (struct packet_header *)buf;
        
        printf("Received Control Message: %c\n", hdr->type);
        if (hdr->type == T_SERVER_ACK) {
            free(buffer);
            free(buf);
            return 1;
        }
    }
    free(buffer);
    return 0;
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

    if (send_ctrl_msg(socketfd, T_DOWNLOAD_TEST, NULL, 0)) {
        printf("Server is Ready for Download Test...\n");
        run_rx_test(socketfd, 0);
    } else {
        printf("Internal Error! Will not run Download Test\n");
    }

    if (send_ctrl_msg(socketfd, T_UPLOAD_TEST, NULL, 0)) {
        printf("Server is Ready for Upload Test...\n");
        run_tx_test(socketfd);
        receive_summarystats(socketfd);
    } else {
        printf("Internal Error! Will not run Upload Test\n");
    }
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

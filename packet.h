#ifndef SPPEDTEST_PACKET_H
#define SPPEDTEST_PACKET_H

#define RX_BUFSIZE 1024
#define TX_BUFSIZE 1024

struct packet_header {
	char type;
        union data {
            struct stats {
                unsigned int len;
                char msg[0];
            }__attribute__((__packed__)) stats;
        } data;
}__attribute__((__packed__));

#define T_DATA 'D'
#define T_END  'E'
#define T_UPLOAD_TEST 'U'
#define T_DOWNLOAD_TEST 'O'
#define T_SERVER_ACK 'A'
#define T_UPLOAD_TEST_RESULTS 'R'
#define T_SUMMARYSTATS 'S'

#endif //SPPEDTEST_PACKET_H

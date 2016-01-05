#ifndef SPPEDTEST_PACKET_H
#define SPPEDTEST_PACKET_H

#define RX_BUFSIZE 10 * 1024
#define TX_BUFSIZE 1024

struct packet_header {
	char type;
}__attribute__((__packed__));

#define T_DATA 'D'
#define T_END  'E'

#endif //SPPEDTEST_PACKET_H

#ifndef __SENDER_H__
#define __SENDER_H__

/*
	Create the next packet
*/
pkt_t* create_next_pkt(FILE *f, uint8_t pkt_nb, uint8_t window);

/*
	Send data to receiver
*/
void send_data(FILE *f, int sfd);
#endif
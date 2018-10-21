/*
 * Author : Antoine Gennart
 * Date : 2018-10
 * Description : This file is part of the project folder for the course
 *               LINGI1341 at UCLouvain.
 */

#ifndef __SENDER_H__
#define __SENDER_H__


/*
	Create the next packet
*/
pkt_status_code create_next_pkt(pkt_t* pkt,
								FILE *f,
								uint32_t pkt_nb,
								uint8_t window,
								uint32_t nb_packet);


/*
	Send data to receiver
*/
void read_write_loop(FILE *f, int sfd);


#endif
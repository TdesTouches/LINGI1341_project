/*
 * Author : Antoine Gennart
 * Date : 2018-10
 * Description : This file is part of the project folder for the course
 *               LINGI1341 at UCLouvain.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <sys/timeb.h>


#include "pkt.h"
#include "network.h"
#include "utils.h"

#include "sender.h"


int main(int argc, char** argv){

	// -------------------------------------------------------------------------
	// ---------------------- parsing input arguments --------------------------
	// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -
	// input :
	// 	- int argc
	//	- char** argv
	// output :
	// 	- char* host
	// 	- uint16_t port
	// 	- FILE* input

	if(argc < 5){
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "sender <opts> [file] <hostname> <port>\n");
		fprintf(stderr, "   -f : input file\n");
		exit(0);
	}

	FILE *input;
	char *host;
	uint16_t port;

	int c;
	extern char* optarg;
	extern int optind;
	while((c=getopt(argc, argv, "f:")) != -1){
		switch(c){
			case 'f':
				input = fopen(optarg, "rb");
				if(input==NULL){
					fprintf(stderr, "Cannot open file %s\n", optarg);
					exit(-1);
				}
				break;
			default:
				fprintf(stderr, "Option not recognized\n");
				exit(-1);
				break;
		}
	}

	host = argv[optind++];
	port = (uint16_t) atoi(argv[optind++]);
	if(optind < argc){
		fprintf(stderr, "%d arguent(s) is (are) ignored\n", argc-optind);
	}


	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
	// ---------------------- starting connection with host  -------------------
	// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -
	// input :
	// 	- char* host
	// 	- uint16_t port;
	// output
	// 	- int sfd
	// 	- sockaddr_in6 addr;

	struct sockaddr_in6 addr;
	const char *err = real_address(host, &addr);
	if(err){
		fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
		return EXIT_FAILURE;
	}

	// int sfd = create_socket(&addr, port, NULL, -1);
	int sfd = create_socket(NULL, -1, &addr, port); /* Connected */
	if (sfd < 0) {
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}

	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
	// --------------------------- Read write loop  ----------------------------
	// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -
	// input :
	// 	- int sfd
	// 	- FILE* input OR stdin_fileno // if file==NULL
	// output :
	read_write_loop(input, sfd);

	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
	// --------------- close connection, end program properly  -----------------
	// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -
	fclose(input);
	close(sfd);

	// -------------------------------------------------------------------------

	return EXIT_SUCCESS;
}


pkt_status_code create_next_pkt(pkt_t* pkt,
								FILE *f,
								uint8_t seqnum,
								uint8_t window,
								uint32_t nb_packet){

	pkt_set_type(pkt, PTYPE_DATA);
	pkt_set_tr(pkt, 0);
	pkt_set_window(pkt, window);
	pkt_set_seqnum(pkt, seqnum);

	if(seqnum!=nb_packet){
		fseek(f, seqnum*MAX_PAYLOAD_SIZE, SEEK_SET);
		char* buf = malloc(MAX_PAYLOAD_SIZE*sizeof(char));
		size_t n_bytes = fread(buf, 1, MAX_PAYLOAD_SIZE, f);
		pkt_set_payload(pkt, buf, n_bytes);
		free(buf);
	}else{
		// last packet
		pkt_set_payload(pkt, NULL, 0);
	}

	pkt_set_timestamp(pkt, 0); // 0 so that timestamp is outdated
	pkt_set_crc1(pkt, pkt_gen_crc1(pkt));
	pkt_set_crc2(pkt, pkt_gen_crc2(pkt));
	return PKT_OK;
}


void read_write_loop(FILE* f, int sfd){
	// window information
	uint8_t window = 1;
	uint8_t seqnum = 0;
	// pkt_t* sliding_window[MAX_WINDOW_SIZE];

	uint32_t nb_packet = tot_nb_packet(f);
	int first_packet = 1;
	pkt_t *pkt_to_send = pkt_new();
	create_next_pkt(pkt_to_send, f, seqnum, window, nb_packet);

	struct pollfd fds[2]; // see man poll
	fds[0].fd = sfd; // file descriptor
	fds[0].events = POLLOUT; // writing on socket
	fds[1].fd = sfd;
	fds[1].events = POLLIN;
	int timeout = 0;

	char buf[MAX_PACKET_SIZE];

	int read_size, write_size; // read and write size on the socket
	pkt_status_code status;


	// uint32_t ct = get_time(); // time in sec
	uint32_t RTT = 1000; // rtt in millisec

	// <= because the last packet send is an PDATA with no data inside
	while(seqnum <= nb_packet){ // while sending packets
		// send packets
		int ready = poll(fds, 2, timeout);
		if(ready==-1){
			fprintf(stderr, "Error while poll : %s\n", strerror(errno));
		}

		if(fds[0].revents == POLLOUT){ // ready to write on the socket
			if(pkt_timestamp_outdated(pkt_to_send, RTT) || first_packet){
				first_packet = 0;
				pkt_update_timestamp(pkt_to_send);

				size_t len = MAX_PACKET_SIZE;
				status = pkt_encode(pkt_to_send, buf, &len);
				if(status != PKT_OK){
					LOG("Encoding error : ");
					LOG(pkt_get_error(status));
				}

				write_size = (int) write(sfd, (void*) buf, len);
				if(write_size != (int) len){
					fprintf(stderr, "Writing the socket was incomplete\n");
				}
			}
		}

		// receive packets and check acks
		if(fds[1].revents == POLLIN){ // ready to read data on socket
			read_size = (int) read(sfd, (void*) buf, sizeof(buf));
			pkt_t *pkt = pkt_new();
			status = pkt_decode(buf, read_size, pkt);
			if(status != PKT_OK){
				fprintf(stderr, "Decoding error : %s\n", pkt_get_error(status));
			}else{
				if(pkt_get_type(pkt)==PTYPE_ACK){
					LOG("Received ack");
				}
				if(pkt_compare_seqnum(pkt, pkt_to_send)){
					if(pkt_get_type(pkt) == PTYPE_ACK){
						seqnum++;
						create_next_pkt(pkt_to_send,
										f,
										seqnum,
										window,
										nb_packet);
					}else if(pkt_get_type(pkt) == PTYPE_NACK){
						fprintf(stderr, "NACK received!\nTODODODODO\n");
					}
				}
			}
			pkt_del(pkt);
		}


	}// while sending packet

	pkt_del(pkt_to_send);
}
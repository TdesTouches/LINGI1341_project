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

uint32_t start_time;

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
	uint32_t before = get_time();
	start_time = before;
	read_write_loop(input, sfd);
	uint32_t after = get_time();
	fprintf(stderr, "Performance : %d [us] for the entire transfer\n",
															(after-before));

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
								uint32_t seqnum32,
								uint8_t window,
								uint32_t nb_packet){

	pkt_set_type(pkt, PTYPE_DATA);
	pkt_set_tr(pkt, 0);
	pkt_set_window(pkt, window);
	pkt_set_seqnum(pkt, (uint8_t) seqnum32);

	if(seqnum32<nb_packet){
		fseek(f, seqnum32*MAX_PAYLOAD_SIZE, SEEK_SET);
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
	uint32_t seqnum32 = 0;

	uint32_t nb_packet = tot_nb_packet(f);
	int i;

	pkt_t *sliding_window[MAX_WINDOW_SIZE+1];
	int sliding_window_ok[MAX_WINDOW_SIZE+1];
	for(i=0;i<MAX_WINDOW_SIZE+1;i++){
		sliding_window[i] = NULL;
		sliding_window_ok[i] = 0;
	}
	sliding_window[0] = pkt_new();
	create_next_pkt(sliding_window[0], f, (uint8_t) seqnum32, window, nb_packet);

	struct pollfd fds[2]; // see man poll
	fds[0].fd = sfd; // file descriptor
	fds[0].events = POLLOUT; // writing on socket
	fds[1].fd = sfd;
	fds[1].events = POLLIN;
	int timeout = 0;

	char buf[MAX_PACKET_SIZE];

	int read_size, write_size; // read and write size on the socket
	pkt_status_code status;


	// Variable for computing RTO using jacobson algorithm
	uint32_t RTO = 5000000; // rtt in microsec
	const uint32_t MAX_RTO = 5000000;
	int RTT_busy = 0; // 1 when computing rtt value
	uint32_t RTT_estimation;
	uint32_t RTT_startTime;
	uint32_t RTT_endTime;
	uint8_t RTT_seqnum;
	const double RTT_alpha = 0.125;
	const double RTT_beta = 0.25;
	uint32_t srtt = RTO;
	uint32_t rttvar = RTO / 2;


	// <= because the last packet send is an PDATA with no data inside
	while(seqnum32 < nb_packet){ // while sending packets
		// send packets
		int ready = poll(fds, 2, timeout);
		if(ready==-1){
			fprintf(stderr, "Error while poll : %s\n", strerror(errno));
		}

		if(fds[0].revents == POLLOUT){ // ready to write on the socket
			for(i=0;i<window;i++){
				if(pkt_timestamp_outdated(sliding_window[i],RTO)){
					LOG("Waiting for ack: %d or more", (uint8_t) seqnum32+1);
					pkt_update_timestamp(sliding_window[i]);

					// Starting jacobson algorithm
					if(RTT_busy==0){
						RTT_startTime = get_time();
						RTT_seqnum = pkt_get_seqnum(sliding_window[i]);
						RTT_busy = 1;
					}

					size_t len = MAX_PACKET_SIZE;
					status = pkt_encode(sliding_window[i], buf, &len);
					if(status != PKT_OK){
						LOG("Encoding error : ");
					}

					write_size = (int) write(sfd, (void*) buf, len);
					if(write_size != (int) len){
						fprintf(stderr, "Writing the socket was incomplete\n");
					}
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
					LOG("Received ack %d at %d", pkt_get_seqnum(pkt),
													get_diff_time(start_time));
					if(pkt_get_seqnum(pkt) < (uint8_t) seqnum32){
						LOG("WTF ?");
					}
					// RTT update
					if(RTT_busy && pkt_get_seqnum(pkt)>RTT_seqnum){
						RTT_busy = 0;
						RTT_endTime = get_time();
						RTT_estimation = RTT_endTime - RTT_startTime;
						if(RTT_estimation < 10000000){
							rttvar = (1-RTT_beta)*rttvar
										+ RTT_beta*abs(srtt-RTT_estimation);
							srtt =(1-RTT_alpha)*srtt + RTT_alpha*RTT_estimation;
							RTO = MIN(srtt + 4*rttvar, MAX_RTO);
							LOG("RTO MODIFICATIONS %d", RTO);
						}else{
							LOG("RTO exeception, should be very rare");
						}
					}

					// check if packet is waited
					uint8_t diff_seqnum = pkt_get_seqnum(pkt) - (seqnum32%256);
					if(diff_seqnum<=window){ // check if packet is in sw
						for(i=0;i<diff_seqnum;i++){
							sliding_window_ok[i] = 1;
						}
					}

					// check window size
					uint8_t window_in = pkt_get_window(pkt);
					if(window_in < window){
						for(i=window_in;i<window;i++){
							sliding_window_ok[i] = 0;
							if(sliding_window[i]!=NULL){
								pkt_del(sliding_window[i]);
								sliding_window[i] = NULL;
							}
						}
						window = window_in;
					}else if(window_in > window){
						window = window_in;
					}

				}else if(pkt_get_type(pkt) == PTYPE_NACK){
					fprintf(stderr, "NACK received!\nTODODODODO\n");
				}
			}
			pkt_del(pkt);
		}

		while(sliding_window_ok[0] == 1){
			seqnum32++;
			pkt_del(sliding_window[0]);
			for(i=0;i<window;i++){
				sliding_window[i] = sliding_window[i+1];
				sliding_window_ok[i] = sliding_window_ok[i+1];
				if(sliding_window[i]==NULL){
					sliding_window[i] = pkt_new();
					create_next_pkt(sliding_window[i],
									f,
									seqnum32+(uint32_t)i,
									window,
									nb_packet);
				}else{
					create_next_pkt(sliding_window[i],
									f,
									seqnum32+(uint32_t)i,
									window,
									nb_packet);
					pkt_update_timestamp(sliding_window[i]);
				}
			}
			sliding_window_ok[window-1] = 0;
		}
	}// while sending packet

	// frees
	for(i=0;i<window;i++){
		pkt_del(sliding_window[i]);
	}
}
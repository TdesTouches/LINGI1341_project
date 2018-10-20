/*
 * Author : Antoine Gennart
 * Date : 2018-10
 * Description : This file is part of the project folder for the course 
 *               LINGI1341 at UCLouvain.
 */

#include <stdio.h>
#include <unistd.h> // getopt
#include <stdlib.h> // exit
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <sys/timeb.h>
#include <unistd.h>

#include "pkt.h"
#include "network.h"
#include "utils.h"
#include "fifo.h"

#include "receiver.h"

#define WINDOW 1

int main(int argc, char** argv){
	// -------------------------------------------------------------------------
	// ---------------------- parsing input arguments --------------------------
	// -------------------------------------------------------------------------
	if(argc < 3){
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "receiver <hostname> <port>\n");
		exit(0);
	}

	char *host;
	uint16_t port;

	extern int optind;

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
	// 	- uint16_t port
	// output
	// 	- int sfd
	// 	- sockaddr_in6 addr;
	struct sockaddr_in6 addr;
	const char *err = real_address(host, &addr);
	if(err){
		fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
		return EXIT_FAILURE;
	}

	int sfd = create_socket(&addr, port, NULL, -1); /* Connected */
	if(sfd > 0 && wait_for_client(sfd) < 0){
		fprintf(stderr, "Could not connect to client after the fist message\n");
		close(sfd);
		return EXIT_FAILURE;
	}
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
	// output : 
	// 	- stdout // if file==NULL
	read_write_loop(sfd);

	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
	// --------------- close connection, end program properly  -----------------
	// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -
	close(sfd);

	// -------------------------------------------------------------------------

	return EXIT_SUCCESS;
}


void read_write_loop(int sfd){
	uint8_t seqnum = 0; // seqnum du packet dans sliding_window[0]

	struct pollfd fds[2];
	fds[0].fd = sfd; // file descriptor
	fds[0].events = POLLIN; // writing on socket
	fds[1].fd = sfd;
	fds[1].events = POLLOUT;
	int timeout = 0;

	char buf[MAX_PACKET_SIZE];
	size_t len;

	int read_size, write_size;
	pkt_status_code status;

	fifo_t *fifo_ack = fifo_new();
	int end_transmission = 0;

	pkt_t *sliding_window[MAX_WINDOW_SIZE];
	int sliding_window_ok[MAX_WINDOW_SIZE]; // everything init to 0 (normally)
	int i;
	for(i=0;i<WINDOW;i++){
		sliding_window[i] = pkt_new();
		sliding_window_ok[i] = 0;
	}

	uint32_t now = get_time();
	uint32_t action_time = get_time();

	while(!end_transmission || !is_fifo_empty(fifo_ack)){

		int ready = poll(fds, 2, timeout);
		if(ready==-1){
			fprintf(stderr, "Error while poll : %s\n", strerror(errno));
		}

		// receive data
		if(fds[0].revents == POLLIN){ // read socket
			LOG("Receiving packet");
			read_size = (int) read(sfd, (void*) buf, sizeof(buf));
			pkt_t *pkt = pkt_new();
			status = pkt_decode(buf, read_size, pkt);
			if(status != PKT_OK || pkt_get_type(pkt) != PTYPE_DATA){
				fprintf(stderr, "Decoding error : %s\n", pkt_get_error(status));	
			}else{
				uint8_t sn_in = pkt_get_seqnum(pkt);
				int index = sn_in - seqnum;
				// pacekt received in sending window
				if(index >= 0 && index < WINDOW){
					pkt_copy(sliding_window[index], pkt);
					sliding_window_ok[index] = 1;
				}
				if(pkt_get_seqnum(pkt) < seqnum + WINDOW - 1){
					pkt_t* pkt_ack = pkt_new();
					uint8_t seq_ack = pkt_get_seqnum(pkt);
					pkt_create(pkt_ack, seq_ack, WINDOW, PTYPE_ACK);
					fifo_push(fifo_ack, pkt_ack);
					for(i=0;i<WINDOW;i++){
						if(!(pkt_get_length(sliding_window[i])>0) && sliding_window_ok[i]){ // last packet
							end_transmission = 1;
						}
					}
				}
			}
			action_time = get_time();
			pkt_del(pkt);
		}

		// send acks
		if(fds[1].revents == POLLOUT){
			while(!is_fifo_empty(fifo_ack)){
				LOG("Sending acks");
				pkt_t* pkt_ack = fifo_pop(fifo_ack);
				pkt_update_timestamp(pkt_ack);
				len = MAX_PACKET_SIZE;
				status = pkt_encode(pkt_ack, buf, &len);
				pkt_check_error("Encoding ack error", status);
				write_size = (int) write(sfd, (void*) buf, len);
				if(write_size != (int) len){
					fprintf(stderr, "Writing the socket was a failure\n");
				}
				pkt_del(pkt_ack);
				action_time = get_time();
			}
		}


		// check if packet can be printed and slide window
		while(sliding_window_ok[0] == 1){
			LOG("Printing packet on stdout");
			if(pkt_get_length(sliding_window[0])>0){
				const char *payload = pkt_get_payload(sliding_window[0]);
				fprintf(stdout, "%s", payload);

				pkt_del(sliding_window[0]);
				for(i=0;i<WINDOW-1;i++){
					sliding_window[i] = sliding_window[i+1];
					sliding_window_ok[i] = sliding_window_ok[i+1];
				}
				sliding_window[WINDOW-1] = pkt_new();
				sliding_window_ok[WINDOW-1] = 0;

			}else{
				// end_transmission = 1;
				sliding_window_ok[0] = 0;
			}
			seqnum++;
			action_time = get_time();
		}

		now = get_time();
		if(now-action_time > 5000){
			LOG("Timeout");
			end_transmission = 1;
		}

	} // while (!end_transmission)

	fflush(stdout);
	for(i=0;i<WINDOW;i++){
		pkt_del(sliding_window[i]);
	}
	fifo_del(fifo_ack);
}
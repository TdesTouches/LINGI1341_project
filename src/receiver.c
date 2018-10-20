#include <stdio.h>
#include <unistd.h> // getopt
#include <stdlib.h> // exit
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <sys/timeb.h>

#include "pkt.h"
#include "network.h"
#include "utils.h"

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

	pkt_t* pkt_ack = pkt_new();
	int send_ack = 0;
	int end_transmission = 0;

	while(!end_transmission){

		int ready = poll(fds, 2, timeout);
		if(ready==-1){
			fprintf(stderr, "Error while poll : %s\n", strerror(errno));
		}

		// receive data
		if(fds[0].revents == POLLIN){ // read socket
			read_size = (int) read(sfd, (void*) buf, sizeof(buf));
			pkt_t *pkt = pkt_new();
			status = pkt_decode(buf, read_size, pkt);
			if(status != PKT_OK){
				fprintf(stderr, "Decoding error : %s\n", pkt_get_error(status));	
			}else{
				if(pkt_get_seqnum(pkt) == seqnum){ // received waited packet
					if(pkt_get_length(pkt) != 0){
						const char* payload = pkt_get_payload(pkt);
						fprintf(stdout, "%s", payload);
					}else{
						end_transmission = 1;
					}
						seqnum++;
				}
				if(pkt_get_seqnum(pkt) < seqnum){
					send_ack = 1;
					pkt_create(pkt_ack, pkt_get_seqnum(pkt), WINDOW, PTYPE_ACK);
				}
			}
			pkt_del(pkt);
		}

		// send acks
		if(fds[1].revents == POLLOUT){
			if(send_ack==1){
				send_ack = 0;
				pkt_update_timestamp(pkt_ack);
				len = MAX_PACKET_SIZE;
				status = pkt_encode(pkt_ack, buf, &len);
				pkt_check_error("Encoding ack error", status);
				write_size = (int) write(sfd, (void*) buf, len);
				if(write_size != (int) len){
					fprintf(stderr, "Writing the socket was a failure\n");
				}
			}
		}
	} // while (!end_transmission)
	fflush(stdout);
	pkt_del(pkt_ack);
}
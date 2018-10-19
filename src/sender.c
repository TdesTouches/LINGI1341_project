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

	fprintf(stderr, "%s\n", host);
	fprintf(stderr, "%d\n", port);


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

	int sfd = create_socket(&addr, port, NULL, -1);
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
	// 	- FILE* input OR stdin_fileno // if file==NULL
	// output : 
	read_write_loop(input, sfd);

	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
	// --------------- close connection, end program properly  -----------------
	// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -
	close(sfd);

	// -------------------------------------------------------------------------

	return EXIT_SUCCESS;
}


pkt_t* create_next_pkt(FILE *f, uint8_t seqnum, uint8_t window){
	pkt_t* pkt = pkt_new();

	fseek(f, seqnum*MAX_PAYLOAD_SIZE, SEEK_SET);
	char* buf = malloc(MAX_PAYLOAD_SIZE*sizeof(char));
	size_t n_bytes = fread(buf, 1, MAX_PAYLOAD_SIZE, f);

	pkt_set_type(pkt, PTYPE_DATA);
	pkt_set_tr(pkt, 0);
	pkt_set_window(pkt, window);
	pkt_set_seqnum(pkt, seqnum);

	pkt_set_payload(pkt, buf, n_bytes);
	pkt_update_timestamp(pkt);
	pkt_set_crc2(pkt, pkt_gen_crc2(pkt));

	return pkt;
}


void read_write_loop(FILE* f, int sfd){
	// window information
	uint8_t window = 1;
	uint8_t seqnum = 0;
	pkt_t* sliding_window[MAX_WINDOW_SIZE];

	uint32_t nb_packet = tot_nb_packet(f);
	int first_packet = 1;
	sliding_window[0] = create_next_pkt(f, seqnum, window);

	struct pollfd fds[2]; // see man poll
	fds[0].fd = sfd; // file descriptor
	fds[0].events = POLLOUT; // writing on socket
	fds[1].fd = sfd;
	fds[1].events = POLLIN;
	int timeout = 0;

	char buf[MAX_PACKET_SIZE];

	int read_size, write_size; // read and write size on the socket
	pkt_status_code status;


	// uint32_t ct = get_time(); // time in millisec
	uint32_t RTT = 1000; // rtt in millisec

	while(seqnum < nb_packet){ // while sending packets

		// send packets
		int ready = poll(fds, 2, timeout);
		if(ready==-1){
			fprintf(stderr, "Error while poll : %s\n", strerror(errno));
		}

		if(fds[0].revents == POLLOUT){ // ready to write on the socket
			if(first_packet || pkt_timestamp_outdated(sliding_window[0], RTT)){
				first_packet = 0;
				pkt_update_timestamp(sliding_window[0]);

				size_t len = MAX_PACKET_SIZE;
				status = pkt_encode(sliding_window[0], buf,	&len);
				if(status != PKT_OK){
					fprintf(stderr, "Encoding error : %s\n", pkt_get_error(status));
				}

				write_size = (int) write(sfd, (void*) buf, len);
				if(write_size != (int) len){
					fprintf(stderr, "Writing the socket was incomplete\n");
				}
			}
		}

		// buf = memset(buf, 0, MAX_PACKET_SIZE);
		// receive packets and check acks, move s_window if necessary
		if(fds[1].revents == POLLIN){ // ready to read data on socket
			read_size = (int) read(sfd, (void*) buf, sizeof(buf));
			pkt_t *pkt = pkt_new();
			status = pkt_decode(buf, read_size, pkt);
			if(status != PKT_OK){
				fprintf(stderr, "Decoding error : %s\n", pkt_get_error(status));	
			}else{
				// compare timestamp with the sliding window elements
				if(pkt_compare_timestamp(pkt, sliding_window[0])){
					slide_array(sliding_window, MAX_WINDOW_SIZE);
					seqnum++;
				}
			}
		}


	}// while sending packet
}
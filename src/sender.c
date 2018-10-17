#include <stdio.h>
#include <unistd.h> // getopt
#include <stdlib.h> // exit

#include "pkt.h"
#include "network.h"

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
	// 	- uint16_t portddr_in6 addr;
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
		fprintf(stderr, "Could not connect to client after the first message\n");
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
	send_data(input, sfd);

	// -------------------------------------------------------------------------

	// -------------------------------------------------------------------------
	// --------------- close connection, end program properly  -----------------
	// --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -
	close(sfd);

	// -------------------------------------------------------------------------

	return EXIT_SUCCESS;
}


pkt_t* create_next_pkt(FILE *f, uint8_t pkt_nb, uint8_t window){
	pkt_t* pkt = pkt_new();

	fseek(f, pkt_nb*MAX_PAYLOAD_SIZE, SEEK_SET);
	char* buf = malloc(MAX_PAYLOAD_SIZE*sizeof(char));
	size_t n_bytes = fread(buf, 1, MAX_PAYLOAD_SIZE, f);

	pkt_set_type(pkt, PTYPE_DATA);
	pkt_set_tr(pkt, 0);
	pkt_set_window(pkt, window);
	pkt_set_seqnum(pkt, pkt_nb);

	pkt_set_payload(pkt, buf, n_bytes);

	return pkt;
}


void send_data(FILE* f, int sfd){
	// window information
	uint8_t window_size = 1;
	uint8_t window_pos = 0;
	pkt_t* sliding_window[MAX_WINDOW_SIZE];
	// todo
}
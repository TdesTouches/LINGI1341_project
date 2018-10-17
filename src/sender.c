#include <stdio.h>
#include <unistd.h> // getopt
#include <stdlib.h> // exit

#include "pkt.h"

#include "sender.h"

int main(int argc, char** argv){

	// -------------------------------------------------------------------------
	// ---------------------- parsing input arguments --------------------------
	// -------------------------------------------------------------------------
	if(argc < 5){
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "sender <opts> [file] <hostname> <port>\n");
		fprintf(stderr, "   -f : input file\n");
		exit(0);
	}

	FILE *input;
	char *address;
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

	address = argv[optind++];
	port = (uint16_t) atoi(argv[optind++]);

	fprintf(stderr, "%s\n", address);
	fprintf(stderr, "%d\n", port);


	// -------------------------------------------------------------------------

	return 0;
}
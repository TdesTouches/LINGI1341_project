#include <stdio.h>
#include <stdlib.h>

#include "pkt.h"

#include "receiver.h"

int main(int argc, char** argv){
	// -------------------------------------------------------------------------
	// ---------------------- parsing input arguments --------------------------
	// -------------------------------------------------------------------------
	if(argc < 3){
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "receiver <hostname> <port>\n");
		exit(0);
	}

	char *address;
	uint16_t port;

	extern int optind;

	address = argv[optind++];
	port = (uint16_t) atoi(argv[optind++]);
	if(optind < argc){
		fprintf(stderr, "%d arguent(s) is (are) ignored\n", argc-optind);
	}

	fprintf(stderr, "%s\n", address);
	fprintf(stderr, "%d\n", port);

	// -------------------------------------------------------------------------


	return 0;
}
#include <stdio.h>

#include "pkt.h"

#include "receiver.h"

int main(int argc, char** argv){
	if(argc==1){
		fprintf(stderr, "Name of the program : %s\n", argv[0]);
	}

	pkt_t *pkt = pkt_new();
	pkt_del(pkt);
	printf("Hello world from receiver\n");
	return 0;
}
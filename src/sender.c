#include <stdio.h>

#include "pkt.h"

#include "sender.h"

int main(int argc, char** argv){
	if(argc==1){
		fprintf(stderr, "Name of the program : %s\n", argv[0]);
	}

	printf("Hello world from sender\n");
}
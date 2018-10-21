/*
 * Author : Antoine Gennart
 * Date : 2018-10
 * Description : This file is part of the project folder for the course
 *               LINGI1341 at UCLouvain.
 */

#include <stdio.h>
#include <sys/timeb.h>

#include "pkt.h"

#include "utils.h"

void slide_array(pkt_t **array, int len){
	int i=0;
	for(i=0; i<len-1; i++){
		array[i] = array[i+1];
	}
}


uint32_t file_size(FILE* f){
	fseek(f, 0L, SEEK_END);
	uint32_t sz = ftell(f);
	rewind(f);
	return sz;
}


uint32_t tot_nb_packet(FILE *f){
	uint32_t nb_packet = 0;
	uint32_t nb_bytes = file_size(f);
	nb_packet += nb_bytes / 512;
	if(nb_bytes % 512 != 0){
		nb_packet += 1;
	}
	return nb_packet+1; // +1 for the last packet
}


uint32_t get_time(){
	struct timeb tp;
	ftime(&tp);
	uint64_t ct = (uint32_t) tp.time * 1000 + (uint32_t) tp.millitm;
	uint32_t ct_32 = ct % 4294967295;
	return ct_32;
}
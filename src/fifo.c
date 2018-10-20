/*
 * Author : Antoine Gennart
 * Date : 2018-10
 * Description : This file is part of the project folder for the course 
 *               LINGI1341 at UCLouvain.
 */

#include <stdlib.h>
#include <stdio.h>

#include "pkt.h"
#include "utils.h"

#include "fifo.h"

fifo_t *fifo_new(){
	fifo_t *fifo = malloc(sizeof(fifo_t));
	fifo->first = NULL;
	return fifo;
}


void fifo_del(fifo_t* fifo){
	if(fifo!=NULL){
		while(fifo->first != NULL){
			fifo_pop(fifo);
		}
		free(fifo);
	}
}


int is_fifo_empty(fifo_t* fifo){
	return fifo->first==NULL;
}


pkt_t* fifo_pop(fifo_t* fifo){
	if(fifo==NULL){
		fprintf(stderr, "NULL fifo\n");
		exit(EXIT_FAILURE);
	}

	fifo_elem* elem = fifo->first;
	pkt_t* pkt = NULL;

	if(elem != NULL){
		pkt = elem->pkt;
		fifo->first = elem->next;
		free(elem);
	}
	return pkt;
}


void fifo_push(fifo_t* fifo, pkt_t* pkt){
	if(pkt==NULL){
		fprintf(stderr, "Trying to push NULL packet on fifo\n");
		return;
	}
	if(fifo==NULL){
		fprintf(stderr, "NULL fifo while pushing\n");
	}

	uint8_t sn = pkt_get_seqnum(pkt);

	if(fifo->first==NULL){
		fifo_elem* new_elem = malloc(sizeof(fifo_elem));
		new_elem->pkt = pkt;
		new_elem->next = NULL;
		fifo->first = new_elem;
	}else{
		fifo_elem* curr = fifo->first;
		while(curr->next != NULL){
			if(sn==pkt_get_seqnum(curr->next->pkt)){
				LOG("Packet already in fifo");
				return;
			}
			curr = curr->next;
		}
		fifo_elem* new_elem = malloc(sizeof(fifo_elem));
		new_elem->pkt = pkt;
		new_elem->next = NULL;
		curr->next = new_elem;
	}
}


void fifo_print(fifo_t* fifo){
	fifo_elem* curr = fifo->first;
	fprintf(stderr, "FIFO block print\n");
	while(curr != NULL){
		fprintf(stderr, "%d\n", pkt_get_seqnum(curr->pkt));
		curr = curr->next;
	}
}
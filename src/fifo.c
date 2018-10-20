#include <stdlib.h>
#include <stdio.h>

#include "pkt.h"

#include "fifo.h"

// int main(){
// 	fifo_t* fifo = fifo_init();
// 	pkt_t* pkt1 = pkt_new();
// 	pkt_t* pkt2 = pkt_new();
// 	pkt_set_seqnum(pkt1, 0);
// 	pkt_set_seqnum(pkt2, 1);
// 	fifo_push(fifo, pkt1);
// 	fifo_push(fifo, pkt2);
// 	pkt_t* rec1 = fifo_pop(fifo);
// 	pkt_t* rec2 = fifo_pop(fifo);
// 	pkt_t* rec3 = fifo_pop(fifo);
// 	if(rec3==NULL){
// 		fprintf(stderr, "Alleluia\n");
// 	}
// 	if(rec1==NULL){
// 		fprintf(stderr, "err1\n" );
// 	}
// 	if(rec2==NULL){
// 		fprintf(stderr, "err2\n" );
// 	}
// 	fprintf(stderr, "%d %d\n", pkt_get_seqnum(pkt1), pkt_get_seqnum(pkt2));
// 	pkt_del(pkt1);
// 	pkt_del(pkt2);
// 	fifo_del(fifo);
// 	return EXIT_SUCCESS;
// }

fifo_t *fifo_init(){
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
			if(sn==pkt_get_seqnum(curr->pkt)){
				fprintf(stderr, "No need to add packet %d, already in queue\n", sn);
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
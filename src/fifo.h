#ifndef __FIFO_H__
#define __FIFO_H__

typedef struct fifo_elem fifo_elem;
struct fifo_elem{
	pkt_t *pkt;
	fifo_elem *next;
};

typedef struct fifo{
	fifo_elem* first;
}fifo_t;


fifo_t* fifo_init();
void fifo_del();

// pre : fifo != empty
// return the pkt in the fifo
pkt_t* fifo_pop(fifo_t* fifo);


void fifo_push(fifo_t* fifo, pkt_t* pkt);
// 



#endif
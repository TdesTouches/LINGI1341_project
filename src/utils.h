#ifndef __UTILS_H_
#define __UTILS_H_

/*
 * slide array to the left
 */
void slide_array(pkt_t **array, int len);

/*
 * 
 */
uint32_t file_size(FILE* f);

/*
 *
 */
uint32_t tot_nb_packet(FILE *f);

uint32_t get_time();

#endif
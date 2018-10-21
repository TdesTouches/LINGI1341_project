/*
 * Author : Antoine Gennart
 * Date : 2018-10
 * Description : This file is part of the project folder for the course
 *               LINGI1341 at UCLouvain.
 */

#ifndef __UTILS_H_
#define __UTILS_H_

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

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


void LOG(const char* msg);
#endif
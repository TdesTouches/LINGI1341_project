/*
 * Author : Antoine Gennart
 * Date : 2018-10
 * Description : This file is part of the project folder for the course
 *               LINGI1341 at UCLouvain.
 */

#ifndef __UTILS_H_
#define __UTILS_H_

#define DEBUG 1

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))


// from Youri mouton
#define _INFO(file, prefix, msg, ...)           \
    do {                                        \
        if (DEBUG)                              \
        fprintf(file, prefix ": "msg"\n",	    \
	##__VA_ARGS__);                             \
    } while(0)

#define LOG(msg, ...)   _INFO(stderr, "[LOG]", msg, ##__VA_ARGS__)
#define ERROR(msg, ...) _INFO(stderr, "[ERROR]", msg, ##__VA_ARGS__)
// not from youri mouton anymore


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

uint32_t get_diff_time(uint32_t start_time);

#endif
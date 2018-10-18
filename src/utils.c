#include "utils.h"

void slide_array(void *array, int len){
	int i=0;
	for(i=0; i<len-1; i++){
		array[i] = array[i+1];
	}
}
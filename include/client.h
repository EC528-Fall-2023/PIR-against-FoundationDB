#ifndef PORAM_CLIENT_H
#define PORAM_CLIENT_H

#include <stdint.h>

/*

*/
void put(uint8_t const *key_name, int key_name_length, uint8_t const *value, int value_length);

/*

*/
uint8_t get(uint8_t const *key_name, int key_name_length);

/*

*/
void read_range(uint8_t const *begin_key_name, int begin_key_name_length, 
		uint8_t const *end_key_name, int end_key_name_length);

/*

*/
void clear_range(uint8_t const *begin_key_name, int begin_key_name_length, 
		 uint8_t const *eng_key_name, int end_key_name_length);

#endif

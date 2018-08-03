#ifndef CIRBU_H_
#define CIRBU_H_

#include "bsp.h"

/*
	A circular buffer non thread safe
	
	To get a circular buffer of MAX_BUFF bytes:
		define:
			static union {
				S_CIRBU c ;
				uint8_t b[sizeof(S_CIRBU) - 1 + MAX_BUFF] ;
			} u ;
		initialize:
			CIRBU_begin(&u.c, MAX_BUFF)

	Now you can access the buffer
*/

typedef struct {
	// Circular buffer dimension = sizeof(S_CIRBU::buf)
	uint16_t DIM_CIRBU ;

	// First byte (start reading from here)
	uint16_t read ;

	// Number of bytes in the circular buffer
	uint16_t tot ;

	// Memory for the buffer
	uint8_t buf[1] ;
} S_CIRBU ;

// Insert bytes in the buffer
void CIRBU_ins(S_CIRBU *, const uint8_t *, uint16_t) ;

// Extract bytes from the buffer
uint16_t CIRBU_ext(S_CIRBU *, uint8_t *, uint16_t) ;

// How many bytes are in the buffer
static inline uint16_t CIRBU_dim(S_CIRBU * x)
{
	return x->tot ;
}

// Free space in the buffer
static inline uint16_t CIRBU_free(S_CIRBU * x)
{
	return x->DIM_CIRBU - x->tot ;
}

// Remove all the bytes
static inline void CIRBU_empty(S_CIRBU * x)
{
	x->read = x->tot = 0 ;
}

// The first function to call
static inline void CIRBU_begin(S_CIRBU * x, uint16_t dim)
{
	x->DIM_CIRBU = dim ;
	x->read = x->tot = 0 ;
}

#endif

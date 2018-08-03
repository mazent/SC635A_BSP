#ifndef GESTORE_H_
#define GESTORE_H_

#include "bsp.h"
#include "pbc.h"

typedef struct {
	osPoolId mp ;
	void (*conn)(const char * ip) ;
	void (*msg)(UN_BUFFER *) ;
	void (*scon)(void) ;
} S_GST_CFG ;

bool GST_beg(S_GST_CFG *) ;
void GST_end(void) ;

bool GST_tx(const void *, uint16_t) ;

#endif

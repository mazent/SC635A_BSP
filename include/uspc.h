#ifndef USPC_H_
#define USPC_H_

#include "bsp.h"
#include "pbc.h"

typedef struct {
	osPoolId mp ;
	void (*msg)(UN_BUFFER *) ;
} S_USPC_CFG ;

bool USPC_open(S_USPC_CFG *) ;
void USPC_close(void) ;

bool USPC_tx(const void *, uint16_t) ;

#endif

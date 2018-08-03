#ifndef TASTO_H_
#define TASTO_H_

#include "bsp.h"

typedef void (* PF_TST)(void) ;

bool TST_beg(PF_TST) ;
void TST_end(void) ;

#endif

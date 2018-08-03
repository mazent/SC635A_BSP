#ifndef AGGIORNA_H_
#define AGGIORNA_H_

#include "bsp.h"

void AGG_beg(uint32_t) ;
bool AGG_dat(const void *, uint32_t dim, uint32_t ofs) ;
bool AGG_end(void) ;

#endif

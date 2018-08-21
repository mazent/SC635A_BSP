#ifndef CAVO_H_
#define CAVO_H_

#include "bsp.h"

/*
 * Queste api forniscono solo l'accesso ai pin
 *
 * In realta' il segnale si muove due volte:
 *  	la prima durante il boot
 *  	la seconda poco dopo
 *
 * Inoltre l'inserimento provoca rimbalzi, per cui occorre
 * validare lo stato del cavo
 */

typedef void (* PF_CRJ)(void) ;

bool CRJ_beg(PF_CRJ) ;
void CRJ_end(void) ;

// True if a cable is inserted
bool CRJ_in(void) ;

#endif

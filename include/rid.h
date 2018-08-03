#ifndef RID_H_
#define RID_H_

#include "bsp.h"

/*
 * Rileva diagnosi
 *
 * Alla fine viene invocata la callback
 */

typedef void (* PF_RID)(void) ;

bool RID_beg(PF_RID) ;
void RID_end(void) ;

// Inizia la procedura (deve essere in stop)
bool RID_start(void) ;

// Ferma la procedura disabilitando la doip se presente
void RID_stop(void) ;

// Torna vero se la rilevazione e' terminata
// Il parametro e' vero se e' doip
bool RID_doip(bool *) ;

#endif

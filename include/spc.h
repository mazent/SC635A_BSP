#ifndef SPC_H_
#define SPC_H_

#include "bsp.h"
#include "pbc.h"

/*
 * Semplice protocollo di comunicazione su seriale
 *
 */


// Solo i 14 bit piu' bassi
typedef uint16_t SPC_CMD ;


typedef struct {
	// stato del protocollo
	bool nega ;

	// il pacchetto finisce qui
	const int DIM_RX ;
	uint8_t * rx ;
	int dimRx ;
} RX_SPC ;

typedef bool (*PF_TX)(const void *, uint16_t) ;

typedef struct {
	// il pacchetto finisce qui
	const int DIM_TX ;
	uint8_t * tx ;
	int dimTx ;

	// quanti, dei byte da inviare, sono nel pacchetto
	int scritti ;

	PF_TX ftx ;
} TX_SPC ;

bool SPC_ini_rx(RX_SPC *) ;
bool SPC_ini_tx(TX_SPC *) ;

bool SPC_esamina(RX_SPC *, UN_BUFFER *) ;

// Risposte
    // Bene
bool SPC_resp(TX_SPC *, SPC_CMD, const void *, int) ;
    // Male
bool SPC_unk(TX_SPC *, SPC_CMD) ;
bool SPC_err(TX_SPC *, SPC_CMD) ;

#endif

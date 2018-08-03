#include "spc.h"

// I due bit alti indicano:
	// Comando
#define ERR_CMD    (0 << 14)
	// Errore nell'esecuzione del comando
#define ERR_EXE    (1 << 14)
	// Errore: richiesta sconosciuta
#define ERR_SCO    (2 << 14)
	// Tutto bene / maschera
#define ERR_OK     (3 << 14)


#define INIZIO_TRAMA        0x8D
#define FINE_TRAMA          0x8E
#define CARATTERE_DI_FUGA   0x8F


extern uint16_t crc1021V(uint16_t, const uint8_t *, int) ;

static uint16_t CRC_I = 0x5635 ;

static uint16_t aggiungi(uint8_t * btx, uint16_t dim, uint8_t x)
{
    if ( (INIZIO_TRAMA == x) ||
         (FINE_TRAMA == x) ||
         (CARATTERE_DI_FUGA == x)
       ) {
        btx[dim++] = CARATTERE_DI_FUGA ;
        x = NOT(x) ;
    }

    btx[dim++] = x ;

    return dim ;
}

static void componi(TX_SPC * ptx, SPC_CMD cmd, const void * v, int d)
{
    union {
        SPC_CMD cmd ;
        uint8_t b[sizeof(SPC_CMD)] ;
        uint16_t crc ;
    } u ;
	uint8_t * btx = ptx->tx ;
	int scritti = 0 ;
	int dimTx = 0 ;

    btx[dimTx++] = INIZIO_TRAMA ;

    // comando
    u.cmd = cmd ;
    dimTx = aggiungi(btx, dimTx, u.b[0]) ;
    dimTx = aggiungi(btx, dimTx, u.b[1]) ;
    uint16_t crc = crc1021V(CRC_I, u.b, sizeof(cmd)) ;

    // Dati
    if (v) {
        const uint8_t * p = v ;
        // dim gia' inseriti + crc + fine trama
        const int DIM_TX = ptx->DIM_TX - dimTx - 2 * 2 - 1 ;

        for (scritti=0 ; (scritti < d) && (dimTx < DIM_TX) ; scritti++)
        	dimTx = aggiungi(btx, dimTx, p[scritti]) ;

        crc = crc1021V(crc, p, scritti) ;
    }

    // Checksum
    u.crc = crc ;
    dimTx = aggiungi(btx, dimTx, u.b[1]) ;
    dimTx = aggiungi(btx, dimTx, u.b[0]) ;

    btx[dimTx++] = FINE_TRAMA ;

    ptx->dimTx = dimTx ;
    ptx->scritti = scritti ;
}

static bool rispondi(TX_SPC * ptx, SPC_CMD cmd, const void * v, int d)
{
	ptx->dimTx = 0 ;
	ptx->scritti = 0 ;

	componi(ptx, cmd, v, d) ;

	return ptx->ftx(ptx->tx, ptx->dimTx) ;
}

bool SPC_ini_rx(RX_SPC * p)
{
	bool esito = false ;

	p->nega = false ;

	if (NULL == p->rx) {
		p->rx = os_malloc(p->DIM_RX) ;
		esito = p->rx != NULL ;
	}
	else
		esito = true ;

	return esito ;
}

bool SPC_ini_tx(TX_SPC * p)
{
	bool esito = false ;

	if (NULL == p->tx) {
		p->tx = os_malloc(p->DIM_TX) ;
		esito = p->tx != NULL ;
	}
	else
		esito = true ;

	return esito ;
}

#define DA_CAPO do {					\
	dimRx = 0 ;         				\
	nega = false ; } while (false)


bool SPC_esamina(RX_SPC * prx, UN_BUFFER * ub)
{
	bool nega = prx->nega ;
	int dimRx = prx->dimRx ;
	uint8_t * brx = prx->rx ;
	const int DIM_RX = prx->DIM_RX ;
	const int LETTI = ub->dim ;
	const uint8_t * dati = ub->mem ;
	bool trovato = false ;

	for (int i=0 ; i<LETTI ; i++) {
	    uint8_t rx = dati[i] ;

	    if (nega) {
	        rx = NOT(rx) ;

	        switch (rx) {
	        case INIZIO_TRAMA:
	        case FINE_TRAMA:
	        case CARATTERE_DI_FUGA:
	            // Solo questi sono ammessi
				brx[dimRx++] = rx ;
				nega = false ;
	            break ;
	        default:
	            DA_CAPO ;
	            DBG_ERR ;
	            break ;
	        }
	    }
	    else if (INIZIO_TRAMA == rx)
	        DA_CAPO ;
	    else if (CARATTERE_DI_FUGA == rx)
	        nega = true ;
	    else if (FINE_TRAMA == rx) {
	        if (dimRx < sizeof(SPC_CMD) + 2) {
	            // Ci deve essere almeno il comando e il crc!
	            DBG_ERR ;
	        }
	        else {
	            // Controllo il crc
				uint16_t crc = crc1021V(CRC_I, brx, dimRx) ;
				if (0 == crc) {
					// Tolgo il crc
					dimRx -= 2 ;

	        		trovato = true ;
	        		break ;
				}
	            else {
	                DBG_ERR ;
	            }
	        }

	        // In ogni caso ricomincio
	        DA_CAPO ;
	    }
	    else if (DIM_RX == dimRx) {
	        // Non ci stanno
	        DA_CAPO ;

	        DBG_ERR ;
	    }
	    else {
	    	brx[dimRx++] = rx ;
	    }
	}

	prx->nega = nega ;
	prx->dimRx = dimRx ;

	return trovato ;
}

bool SPC_resp(TX_SPC * ptx, SPC_CMD cmd, const void * v, int d)
{
	cmd |= ERR_OK ;

	return rispondi(ptx, cmd, v, d) ;
}

bool SPC_unk(TX_SPC * ptx, SPC_CMD cmd)
{
    cmd |= ERR_SCO ;

    return rispondi(ptx, cmd, NULL, 0) ;
}

bool SPC_err(TX_SPC * ptx, SPC_CMD cmd)
{
    cmd |= ERR_EXE ;

    return rispondi(ptx, cmd, NULL, 0) ;
}

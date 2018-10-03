#ifndef TLS_SRV_H_
#define TLS_SRV_H_

#include "bsp.h"

/*
	server tcp con tls
	
	appunti: https://gist.github.com/mazent/5fbd28aaeb0cb1a42e9ebb3b2810d77c

	Apre un socket udp (per i comandi) e un socket tcp di servizio
	sulla porta indicata
*/

#define TLS_SRV_MSG_DIM		1400

struct TLS_SRV ;
typedef struct TLS_SRV TLS_SRV ;

typedef struct {
	uint32_t id ;
	size_t dim ;
	uint8_t mem[TLS_SRV_MSG_DIM] ;
} TLS_SRV_MSG ;


typedef struct {
	uint16_t porta ;
	
	// da qui vengono presi i messaggi
	osPoolId mp ;
	
	// copiato sul messaggio
	uint32_t id ;

	// callback:
		// connessione
	void (*conn)(const char * ip) ;
		// messaggio: alla fine qualcuno deve rimetterlo in mp
	void (*msg)(TLS_SRV_MSG *) ;
		// sconnessione
	void (*scon)(void) ;

	// Nome del client autorizzto (CN)
	const char * cln_cn ;

	// Catena dei certificati
	const unsigned char * cert_chain ;
	size_t dim_cert_chain ;

	// Certificato del server
	const unsigned char * srv_cert ;
	size_t dim_srv_cert ;

	// Chiave privata del server
	const unsigned char * srv_key ;
	size_t dim_srv_key ;

} TLS_SRV_CFG ;

TLS_SRV * TLS_SRV_beg(TLS_SRV_CFG *) ;
void TLS_SRV_end(TLS_SRV **) ;

bool TLS_SRV_tx(TLS_SRV *, const void *, uint16_t) ;

#endif

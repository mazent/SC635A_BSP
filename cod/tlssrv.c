#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "tlssrv.h"

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"

#if defined(MBEDTLS_SSL_CACHE_C)
#include "mbedtls/ssl_cache.h"
#endif

#include "mbedtls/esp_debug.h"

#define STACK	5000

struct TLS_SRV {
	TLS_SRV_CFG cfg ;

	osThreadId tid ;

	// socket udp per comandi
	int srvI ;

	mbedtls_net_context listen_fd, client_fd;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt srvcert;
	mbedtls_pk_context pkey;
#if defined(MBEDTLS_SSL_CACHE_C)
	mbedtls_ssl_cache_context cache;
#endif
} ;

static const char * TAG = "tlssrv";


#define CMD_ESCI		((uint32_t)	0xFF7B7E76)
#define CMD_CH_CLN		((uint32_t)	0xFF42A85A)


static void riusabile(int sockfd)
{
	int optval = 1 ;
	/* setsockopt: Handy debugging trick that lets
	 * us rerun the server immediately after we kill it;
	 * otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 */
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
			(const void *)&optval , sizeof(int));
}

static bool invia(TLS_SRV * pS, uint32_t cmd)
{
	bool esito = false ;
	int soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

	do {
		if (soc < 0)
			break ;

		struct sockaddr_in server = { 0 } ;
		server.sin_family = AF_INET;
		server.sin_port = htons(pS->cfg.porta);
		server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		if (sendto(soc, &cmd, sizeof(cmd), 0,
		                 (struct sockaddr *)&server, sizeof(server)) < 0)
			break ;

		uint32_t rsp ;
		int n = recvfrom(soc, &rsp, sizeof(rsp), 0, NULL, 0) ;
		if (n != sizeof(rsp))
			break ;

		esito = rsp == cmd ;

	} while (false) ;

	if (soc >= 0)
	   close(soc);

	return esito ;
}

static bool load_cert_n_key(TLS_SRV * pSrv)
{
	bool esito = false ;

	do {
	    ESP_LOGI(TAG, "Loading the server cert. and key ..." );

	    int ret = mbedtls_x509_crt_parse(
	    				&pSrv->srvcert,
	    				pSrv->cfg.srv_cert,
	                    pSrv->cfg.dim_srv_cert) ;
	    if (ret != 0) {
	        ESP_LOGE(TAG, "srv: mbedtls_x509_crt_parse returned %d", ret) ;
	        break ;
	    }

	    ret = mbedtls_x509_crt_parse(
	    				&pSrv->srvcert,
	    				pSrv->cfg.cert_chain,
	                    pSrv->cfg.dim_cert_chain) ;
	    if (ret != 0) {
	        ESP_LOGE(TAG, "catena: mbedtls_x509_crt_parse returned %d", ret) ;
	        break ;
	    }

	    ret = mbedtls_pk_parse_key(
	    				&pSrv->pkey,
	    				pSrv->cfg.srv_key, pSrv->cfg.dim_srv_key,
	    				NULL, 0) ;
	    if (ret != 0) {
	        ESP_LOGE(TAG, "mbedtls_pk_parse_key returned %d", ret) ;
	        break ;
	    }

	    esito = true ;

	} while (false) ;

	return esito ;
}

static bool setup_socket(TLS_SRV * pSrv)
{
	bool esito = false ;

	do {
		char porta[10] ;

		sprintf(porta, "%d", pSrv->cfg.porta) ;

		ESP_LOGI(TAG, "Bind on %s", porta) ;

		int ret = mbedtls_net_bind(
						&pSrv->listen_fd,
						NULL, porta,
						MBEDTLS_NET_PROTO_TCP) ;

	    if (ret != 0) {
	    	ESP_LOGE(TAG, "mbedtls_net_bind returned %d", ret );
	        break ;
	    }

	    esito = true ;

	} while (false) ;

	return esito ;
}

static bool setup_rng(TLS_SRV * pSrv)
{
	ESP_LOGI(TAG, "Seeding the random number generator" );

	int ret = mbedtls_ctr_drbg_seed(
					&pSrv->ctr_drbg,
					mbedtls_entropy_func,
					&pSrv->entropy,
					NULL, 0) ;


    if (ret != 0) {
    	ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret) ;
    }

	return 0 == ret ;
}

#ifdef MBEDTLS_DEBUG_C
static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
#if 0
    ESP_LOGI(TAG, "dbg %s:%04d: %s", file, line, str) ;
#else
    // Basta solo str ma senza l'acapo
	char x[100] ;
	size_t i = strlen(str) - 1 ;
	while ((str[i] == 0x0D) || (str[i] == 0x0A))
		--i ;
	if (i >= sizeof(x))
		i = sizeof(x) - 1 ;

	memcpy(x, str, i+1) ;
	x[i+1] = 0 ;

    ESP_LOGI(TAG, "dbg %s", x) ;
#endif
}
#endif

static bool setup_tls(TLS_SRV * pSrv)
{
	bool esito = false ;

	do {
		ESP_LOGI(TAG, "Setting up the SSL data") ;

		int ret = mbedtls_ssl_config_defaults(
						&pSrv->conf,
						MBEDTLS_SSL_IS_SERVER,
						MBEDTLS_SSL_TRANSPORT_STREAM,
						MBEDTLS_SSL_PRESET_DEFAULT) ;
		if (ret != 0) {
			ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret) ;
		    break ;
		}

		mbedtls_ssl_conf_authmode(&pSrv->conf, MBEDTLS_SSL_VERIFY_REQUIRED) ;

		mbedtls_ssl_conf_rng(&pSrv->conf, mbedtls_ctr_drbg_random, &pSrv->ctr_drbg) ;
#ifdef MBEDTLS_DEBUG_C
		mbedtls_ssl_conf_dbg(&pSrv->conf, my_debug, NULL) ;
#endif
#if defined(MBEDTLS_SSL_CACHE_C)
		mbedtls_ssl_conf_session_cache(&pSrv->conf, &pSrv->cache,
		                               mbedtls_ssl_cache_get,
		                               mbedtls_ssl_cache_set) ;
#endif
		mbedtls_ssl_conf_ca_chain(&pSrv->conf, pSrv->srvcert.next, NULL) ;

		ret = mbedtls_ssl_conf_own_cert(&pSrv->conf, &pSrv->srvcert, &pSrv->pkey) ;
		if (ret != 0) {
			ESP_LOGE(TAG, "mbedtls_ssl_conf_own_cert returned %d", ret) ;
		    break ;
		}

		ret = mbedtls_ssl_setup(&pSrv->ssl, &pSrv->conf) ;
		if (ret != 0) {
			ESP_LOGE(TAG, "mbedtls_ssl_setup returned %d", ret) ;
		    break ;
		}

		ret = mbedtls_ssl_set_hostname(&pSrv->ssl, pSrv->cfg.cln_cn) ;
		if (ret != 0) {
			ESP_LOGE(TAG, "mbedtls_ssl_set_hostname = %d", ret) ;
		    break ;
		}

		esito = true ;
	} while (false) ;

	return esito ;
}

void reset(TLS_SRV * pSrv)
{
    mbedtls_net_free(&pSrv->client_fd) ;
    mbedtls_ssl_session_reset(&pSrv->ssl) ;
}

static void tlsThd(void * v)
{
	fd_set active_fd_set, read_fd_set;
	int i;
	TLS_SRV * pSrv = v ;
	TLS_SRV_MSG * msg = (TLS_SRV_MSG *) osPoolAlloc(pSrv->cfg.mp) ;

	mbedtls_net_init( &pSrv->listen_fd );
	mbedtls_net_init( &pSrv->client_fd );
	mbedtls_ssl_init( &pSrv->ssl );
	mbedtls_ssl_config_init( &pSrv->conf );
#if defined(MBEDTLS_SSL_CACHE_C)
	mbedtls_ssl_cache_init( &pSrv->cache );
#endif
	mbedtls_x509_crt_init( &pSrv->srvcert );
	mbedtls_pk_init( &pSrv->pkey );
	mbedtls_entropy_init( &pSrv->entropy );
    mbedtls_ctr_drbg_init( &pSrv->ctr_drbg );
#ifdef MBEDTLS_DEBUG_C
    // Debug level (0-4 verboso)
    mbedtls_esp_enable_debug_log(&pSrv->conf, 2);
#endif

	do {
	    /*
	     * 1. Load the certificates and private RSA key
	     */
		if ( !load_cert_n_key(pSrv) )
			break ;

	    /*
	     * 2. Setup the listening TCP socket
	     */
		if ( !setup_socket(pSrv) )
			break ;

	    /*
	     * 3. Seed the RNG
	     */
		if ( !setup_rng(pSrv) )
			break ;

		/*
		 * 4. Setup stuff
		 */
		if ( !setup_tls(pSrv) )
			break ;

		reset(pSrv) ;

		FD_ZERO(&active_fd_set) ;

		struct sockaddr_in name = { 0 } ;
		name.sin_family = AF_INET;
		name.sin_port = htons(pSrv->cfg.porta) ;

		// socket interno udp
		pSrv->srvI = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
		if (pSrv->srvI < 0)
			break ;

		name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		riusabile(pSrv->srvI) ;

		if ( bind(pSrv->srvI, (struct sockaddr *) &name, sizeof (name)) < 0)
			break ;

		FD_SET(pSrv->srvI, &active_fd_set);

		// DA FARE: registrarsi con mDNS

		FD_SET(pSrv->listen_fd.fd, &active_fd_set);

		bool continua = true ;
		while (continua) {
			/* Block until input arrives on one or more active sockets. */
			read_fd_set = active_fd_set;
			if ( select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0 )
				break ;

			/* Service all the sockets with input pending. */
			for (i = 0; i < FD_SETSIZE; ++i) {
				if ( FD_ISSET(i, &read_fd_set) ) {
					if (i == pSrv->srvI) {
						// comando
						uint32_t cmd ;
						int nbytes = read(pSrv->srvI, &cmd, sizeof(cmd));
						if (nbytes == sizeof(cmd)) {
							switch (cmd) {
							case CMD_CH_CLN:
					            FD_CLR(pSrv->client_fd.fd, &active_fd_set) ;
					            close(pSrv->client_fd.fd) ;
								break ;
							case CMD_ESCI:
								if (pSrv->client_fd.fd >= 0) {
									FD_CLR(pSrv->client_fd.fd, &active_fd_set) ;
									reset(pSrv) ;
								}

								(void) sendto(pSrv->srvI, &cmd, sizeof(cmd), 0, NULL, 0) ;

								continua = false ;
								break ;
							}
						}
					}
					else if (i == pSrv->listen_fd.fd) {
						/* Connection request on original socket. */
						struct in_addr ipa ;
						size_t dimip ;
						int ret = mbedtls_net_accept(
										&pSrv->listen_fd,
										&pSrv->client_fd,
										&ipa, sizeof(ipa), &dimip) ;
					    if (ret != 0) {
					    	ESP_LOGE(TAG, "OKKIO mbedtls_net_accept returned %d", ret) ;

					    	// Adesso non funziona piu'!!!
					        reset(pSrv) ;
					    }

					    mbedtls_ssl_set_bio(&pSrv->ssl, &pSrv->client_fd, mbedtls_net_send, mbedtls_net_recv, NULL) ;

					    /*
					     * 5. Handshake
					     */

					    bool hsOk = true ;
					    while ( ( ret = mbedtls_ssl_handshake( &pSrv->ssl ) ) != 0 ) {
					        if ( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {
					        	ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -%d(0x%04X)\n\n", -ret, -ret) ;

					        	reset(pSrv) ;
					        	hsOk = false ;
					        	break ;
					        }
					    }

					    if (hsOk) {
					    	FD_SET(pSrv->client_fd.fd, &active_fd_set) ;
					    	char * ip = inet_ntoa(ipa);
					    	pSrv->cfg.conn(ip) ;
					    }
					}
					else {
						/* Data arriving on an already-connected socket. */
						do {
							if (NULL == msg) {
								// Riprovo
								msg = (TLS_SRV_MSG *) osPoolAlloc(pSrv->cfg.mp) ;

								ESP_LOGE(TAG, "buffer esauriti") ;
								if (NULL == msg)
									break ;
							}

							msg->id = pSrv->cfg.id ;

						    do {
						        int ret = mbedtls_ssl_read(&pSrv->ssl, msg->mem, TLS_SRV_MSG_DIM) ;

						        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
						            continue ;

						        if (ret <= 0) {
						            switch (ret) {
									case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
										ESP_LOGE(TAG, "connection was closed gracefully\n") ;
										break ;

									case MBEDTLS_ERR_NET_CONN_RESET:
										ESP_LOGE(TAG, "connection was reset by peer\n") ;
										break;

									case 0:
										// Sconnesso
										break ;

									default:
										ESP_LOGE(TAG, "mbedtls_ssl_read returned -%d(0x%04X)\n", -ret, -ret) ;
										break;
						            }

						            close(i) ;
						            FD_CLR(i, &active_fd_set) ;
						            pSrv->cfg.scon() ;
						            reset(pSrv) ;
						            break;
						        }

						        //ESP_LOGI(TAG, "%d bytes read", ret) ;

						        msg->dim = ret ;

						        pSrv->cfg.msg(msg) ;

						        msg = (TLS_SRV_MSG *) osPoolAlloc(pSrv->cfg.mp) ;
						        break ;

						    } while (true) ;

						} while (false) ;
					}
				}
			}
		}

	} while (false) ;

    mbedtls_net_free(&pSrv->client_fd);
    mbedtls_net_free(&pSrv->listen_fd);

    mbedtls_x509_crt_free(&pSrv->srvcert) ;
    mbedtls_pk_free(&pSrv->pkey) ;
    mbedtls_ssl_free(&pSrv->ssl) ;
    mbedtls_ssl_config_free(&pSrv->conf) ;
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_free(&pSrv->cache) ;
#endif
    mbedtls_ctr_drbg_free(&pSrv->ctr_drbg) ;
    mbedtls_entropy_free(&pSrv->entropy) ;

	pSrv->tid = NULL ;
	CHECK_IT( osOK == osThreadTerminate(NULL) ) ;
}

TLS_SRV * TLS_SRV_beg(TLS_SRV_CFG * pCfg)
{
	TLS_SRV * srv = NULL ;

	do {
		assert(pCfg) ;
		if (NULL == pCfg)
			break ;

		assert(pCfg->mp) ;
		if (NULL == pCfg->mp)
			break ;

		assert(pCfg->conn) ;
		if (NULL == pCfg->conn)
			break ;

		assert(pCfg->scon) ;
		if (NULL == pCfg->scon)
			break ;

		assert(pCfg->msg) ;
		if (NULL == pCfg->msg)
			break ;

		srv = (TLS_SRV *) os_malloc(sizeof(TLS_SRV)) ;
		assert(srv) ;
		if (NULL == srv)
			break ;

		srv->cfg = *pCfg ;

		osThreadDef(tlsThd, osPriorityNormal, 1, STACK) ;
		srv->tid = osThreadCreate(osThread(tlsThd), srv) ;
		assert(srv->tid) ;
		if (NULL == srv->tid) {
			os_free(srv) ;
			srv = NULL ;
			break ;
		}

	} while (false) ;

	return srv ;
}

void TLS_SRV_end(TLS_SRV ** x)
{
	do {
		if (NULL == x)
			break ;

		TLS_SRV * pS = *x ;
		if (NULL == pS)
			break ;

		*x = NULL ;

		while (pS->tid) {
			(void) invia(pS, CMD_ESCI) ;

			osDelay(100) ;
		}

		os_free(pS) ;

	} while (false) ;
}

bool TLS_SRV_tx(TLS_SRV * pS, const void * v, uint16_t len)
{
	if (NULL == pS)
		return false ;
	else if (pS->client_fd.fd < 0)
		return false ;
	else {
		bool esito = false ;
		const uint8_t * buf = (const uint8_t *) v ;

		while (true) {
			int ret = mbedtls_ssl_write(&pS->ssl, buf, len) ;

			if ( ret == MBEDTLS_ERR_SSL_WANT_READ ||
				 ret == MBEDTLS_ERR_SSL_WANT_WRITE )
				continue ;

			if (ret < 0) {
	            reset(pS) ;

	            (void) invia(pS, CMD_CH_CLN) ;
	            break ;
			}

			if (ret < len) {
				buf += ret ;
				len -= ret ;
				continue ;
			}

			esito = ret == len ;
			break ;
		}

	    return esito ;
	}
}

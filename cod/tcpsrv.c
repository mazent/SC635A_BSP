#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "tcpsrv.h"


#define STACK	2000

struct TCP_SRV {
	TCPSRV_CFG cfg ;

	osThreadId tid ;

	int cln ;
} ;

static const char * TAG = "tcpsrv";

//static osThreadId tid = NULL ;

//static TCPSRV_CFG cfg = {
//	.conn = NULL,
//	.msg = NULL,
//	.scon = NULL
//} ;

//static int cln = -1 ;

#define CMD_ESCI		((uint32_t)	0x5C72EC95)

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

static bool invia(TCP_SRV * pS, uint32_t cmd)
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

static void tcpThd(void * v)
{
	int srvE, srvI ;
	fd_set active_fd_set, read_fd_set;
	int i;
	TCPSRV_MSG * msg = (TCPSRV_MSG *) osPoolAlloc(cfg.mp) ;
	TCP_SRV * pS = v ;

	do {
		FD_ZERO(&active_fd_set) ;

		struct sockaddr_in name = { 0 } ;
		name.sin_family = AF_INET;
		name.sin_port = htons(pS->cfg.porta) ;

		// socket interno udp
		srvI = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
		if (srvI < 0)
			break ;

		name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		riusabile(srvI) ;

		if ( bind(srvI, (struct sockaddr *) &name, sizeof (name)) < 0)
			break ;

		FD_SET(srvI, &active_fd_set);

		/* Create the socket and set it up to accept connections. */
		srvE = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (srvE < 0)
			break ;

		name.sin_addr.s_addr = htonl(INADDR_ANY);

		if ( bind(srvE, (struct sockaddr *) &name, sizeof (name)) < 0)
			break ;

		if ( listen(srvE, 1) < 0 )
			break ;

		// DA FARE: registrarsi con mDNS

		FD_SET(srvE, &active_fd_set);

		bool continua = true ;
		while (continua) {
			/* Block until input arrives on one or more active sockets. */
			read_fd_set = active_fd_set;
			if ( select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0 )
				break ;

			/* Service all the sockets with input pending. */
			for (i = 0; i < FD_SETSIZE; ++i) {
				if ( FD_ISSET(i, &read_fd_set) ) {
					if (i == srvI) {
						// comando
						uint32_t cmd ;
						int nbytes = read(srvI, &cmd, sizeof(cmd));
						if (nbytes == sizeof(cmd)) {
							switch (cmd) {
							case CMD_ESCI:
								if (pSrv->cln >= 0) {
									close(pSrv->cln) ;
									pSrv->cln = -1 ;
								}
								close(srvE) ;

								(void) sendto(srvI, &cmd, sizeof(cmd), 0, NULL, 0) ;

								continua = false ;
								break ;
							}
						}
					}
					else if (i == srvE) {
						/* Connection request on original socket. */
						struct sockaddr_in clientname ;
						size_t size = sizeof (clientname) ;
						pSrv->cln = accept(srvE, (struct sockaddr *) &clientname, &size) ;
						if (pSrv->cln >= 0) {
							const char * ip = inet_ntoa(clientname.sin_addr) ;

							FD_SET(pSrv->cln, &active_fd_set);

							pS->cfg.conn(ip) ;
						}
					}
					else {
						/* Data arriving on an already-connected socket. */
						do {
							if (NULL == msg) {
								// Riprovo
								msg = (TCPSRV_MSG *) osPoolAlloc(pS->cfg.mp) ;

								ESP_LOGE(TAG, "buffer esauriti") ;
								if (NULL == msg)
									break ;
							}

							msg->id = pS->cfg.id ;

							int nbytes = read(i, msg->mem, TCPSRV_MSG_DIM) ;
							if (nbytes <= 0) {
								// sconnesso!
								close(i);
								FD_CLR(i, &active_fd_set);

								pS->cln = -1 ;

								pS->cfg.scon() ;
							}
							else {
								/* Data read. */
								msg->dim = nbytes ;

								pS->cfg.msg(msg) ;

								msg = (TCPSRV_MSG *) osPoolAlloc(pS->cfg.mp) ;
							}

						} while (false) ;
					}
				}
			}
		}

	} while (false) ;

	pS->tid = NULL ;
	CHECK_IT( osOK == osThreadTerminate(NULL) ) ;
}

TCP_SRV * TCPSRV_beg(TCPSRV_CFG * pCfg)
{
	TCP_SRV * srv = NULL ;

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

		srv = (TCP_SRV *) os_malloc(sizeof(TCP_SRV)) ;
		assert(srv) ;
		if (NULL == srv)
			break ;

		srv->cfg = *pCfg ;
		srv->cln = -1 ;

		osThreadDef(tcpThd, osPriorityNormal, 1, STACK) ;
		srv->tid = osThreadCreate(osThread(tcpThd), srv) ;
		assert(srv->tid) ;
		if (NULL == srv->tid) {
			os_free(srv) ;
			srv = NULL ;
			break ;
		}

	} while (false) ;

	return srv ;
}

void TCPSRV_end(TCP_SRV ** x)
{
	do {
		if (NULL == x)
			break ;

		TCP_SRV * pS = *x ;
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

bool TCPSRV_tx(TCP_SRV * pS, const void * buf, uint16_t count)
{
	if (NULL == pS)
		return false ;
	else if (pS->cln < 0)
		return false ;
	else {
		int s = write(pS->cln, buf, count) ;
		if (s < 0)
			return false ;
		else
			return s == count ;
	}
}

#include "cirbu.h"

static uint16_t incr(uint16_t x, uint16_t y, uint16_t l)
{
	x += y ;

	return x % l ;
}

void CIRBU_ins(S_CIRBU * pC, const uint8_t * data, uint16_t dim)
{
	assert(pC) ;
	assert(data) ;
	assert(dim) ;

	DBG_PRINTF("%s(%p, %u)\n", __FUNCTION__, data, dim) ;

	do {
		if (NULL == pC)
			break ;

		if (NULL == data)
			break ;

		if (0 == dim)
			break ;

		if (0 == pC->tot)
			pC->read = 0 ;

		const uint16_t DIM_CIRBU = pC->DIM_CIRBU ;
		if (dim > DIM_CIRBU) {
			// The buffer will never store all this things
			DBG_ERR ;

			// Only the last will be saved
			pC->tot = DIM_CIRBU ;
			pC->read = 0 ;
			data += dim - DIM_CIRBU ;
			memcpy(pC->buf, data, DIM_CIRBU) ;

			// Done
			break ;
		}

		const uint16_t FREE = DIM_CIRBU - pC->tot ;
		if (dim > FREE) {
			// Overflow
			DBG_ERR ;

			// Free some space
			pC->read = incr(pC->read, dim - FREE, DIM_CIRBU) ;

			// Now we can proceed
		}

		// The free space starts here
		uint16_t write_pos = incr(pC->read, pC->tot, DIM_CIRBU) ;

		if (1 == dim) {
			pC->buf[write_pos] = *data ;
			pC->tot++ ;
		}
		else if (pC->read >= write_pos) {
			//     w      r
			// ****.......****
			memcpy(pC->buf + write_pos, data, dim) ;
			pC->tot += dim ;
		}
		else {
			//     r      w
			// ....*******....
			// The first bytes go to the end
			const uint16_t END_DIM = MIN(dim, DIM_CIRBU - write_pos) ;
			memcpy(pC->buf + write_pos, data, END_DIM) ;
			pC->tot += END_DIM ;

			// The rest at the beginning
			dim -= END_DIM ;
			if (dim) {
				memcpy(pC->buf, data + END_DIM, dim) ;
				pC->tot += dim ;
			}
		}

	} while (false) ;
}

uint16_t CIRBU_ext(S_CIRBU * pC, uint8_t * data, uint16_t dim)
{
	uint16_t risul = 0 ;

	assert(pC) ;
	assert(data) ;
	assert(dim) ;

	DBG_PRINTF("%s(%p, %u)\n", __FUNCTION__, data, dim) ;

	do {
		if (NULL == pC)
			break ;

		if (NULL == data)
			break ;

		if (0 == dim)
			break ;

		if (0 == pC->tot) {
			DBG_ERR ;
			break ;
		}

		if (dim > pC->tot)
			dim = pC->tot ;

		if (1 == pC->tot) {
			*data = pC->buf[pC->read] ;
			pC->read = incr(pC->read, 1, pC->DIM_CIRBU) ;
			pC->tot-- ;
            risul = 1 ;
			break ;
		}

		while (dim) {
			// We can read the data ...
			// 	   ... to the end
			//                   r   L
			//        ****.......****
			//     ... to the total:
			//            r      L
			//        ....*******....
			const uint16_t LAST = MIN(pC->DIM_CIRBU, pC->read + pC->tot) ;
			const uint16_t DIM = MIN(dim, LAST - pC->read) ;
			if (1 == DIM)
				*data = pC->buf[pC->read] ;
			else
				memcpy(data, pC->buf + pC->read, DIM) ;

			data += DIM ;
			risul += DIM ;
			pC->tot -= DIM ;

			pC->read = incr(pC->read, DIM, pC->DIM_CIRBU) ;
			dim -= DIM ;
		}

	} while (false) ;

	return risul ;
}

#if 0

#	define MAX_BUFF		911

	static union {
		S_CIRBU circ ;
		uint8_t b[MAX_BUFF + sizeof(S_CIRBU) - 1] ;
	} u ;

	static uint8_t ctu[2 * MAX_BUFF] ;
	static uint8_t tmp[MAX_BUFF] ;

	static const uint16_t vDim[] = {
		// Ha senso provare ad aggiungere un elemento ...
		  1,
		// ..e poi vado di numeri primi
		  2,   3,   5,   7,  11,  13,  17,  19,   23,  29,
		 31,  37,  41,  43,  47,  53,  59,  61,   67,  71,
		 73,  79,  83,  89,  97, 101, 103, 107,  109, 113,
		127, 131, 137, 139, 149, 151, 157, 163,  167, 173,
		179, 181, 191, 193, 197, 199, 211, 223,  227, 229,
		233, 239, 241, 251, 257, 263, 269, 271,  277, 281,
		283, 293, 307, 311, 313, 317, 331, 337,  347, 349,
		353, 359, 367, 373, 379, 383, 389, 397,  401, 409,
		419, 421, 431, 433, 439, 443, 449, 457,  461, 463,
		467, 479, 487, 491, 499, 503, 509, 521,  523, 541,
		547, 557, 563, 569, 571, 577, 587, 593,  599, 601,
		607, 613, 617, 619, 631, 641, 643, 647,  653, 659,
		661, 673, 677, 683, 691, 701, 709, 719,  727, 733,
		739, 743, 751, 757, 761, 769, 773, 787,  797, 809,
		811, 821, 823, 827, 829, 839, 853, 857,  859, 863,
		877, 881, 883, 887, 907, 911, 919, 929,  937, 941,
		947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013
	} ;
	static const size_t MAX_DIM = DIM_VETT(vDim) ;

	static void riempio_ed_estraggo(void)
	{
		PUTS("Riempio in un colpo ed astraggo a quinti") ;

		// Ripulisco
		CIRCO_svuota(&u.circ) ;

		// Riempio ...
		CIRBU_ins(&u.circ, ctu, MAX_BUFF) ;
		assert( MAX_BUFF == CIRCO_dim(&u.circ) ) ;

		// ... ed estraggo
		uint16_t tot = 0 ;
		while (true) {
			uint16_t letti = CIRBU_ext(&u.circ, tmp, MAX_BUFF / 5) ;
			if (0 == letti)
				break ;

			assert(0 == memcmp(ctu + tot, tmp, letti)) ;
			tot += letti ;
		}
	}

	static void inserisco_ed_estraggo(const uint16_t PARZ, const size_t CICLI)
	{
        uint16_t tot = 0 ;

        PRINTF("%u cicli da %d byte: ", CICLI, PARZ) ;

        uint32_t inizio = bsp_get_milli() ;

		// Ripulisco
		CIRCO_svuota(&u.circ) ;

		// Metto qualcosa
		CIRBU_ins(&u.circ, ctu, PARZ) ;
		tot += PARZ ;
		assert( tot == CIRCO_dim(&u.circ) ) ;

		for (size_t i=0 ; i<CICLI ; i++) {
			// Metto altre cose
			CIRBU_ins(&u.circ, ctu, PARZ) ;
			tot += PARZ ;
			assert(tot == CIRCO_dim(&u.circ)) ;

			// Tolgo
			uint16_t letti = CIRBU_ext(&u.circ, tmp, PARZ) ;
			assert(letti == PARZ) ;
			tot -= letti ;
			assert( tot == CIRCO_dim(&u.circ) ) ;

			for (uint16_t j=0 ; j<letti ; j++) {
				assert(tmp[j] == ctu[j]) ;
			}
		}

		uint16_t letti = CIRBU_ext(&u.circ, tmp, sizeof(tmp)) ;
		assert( letti == PARZ ) ;
		assert( 0 == CIRCO_dim(&u.circ) ) ;

		for (uint16_t i=0 ; i<letti ; i++) {
			assert(tmp[i] == ctu[i]) ;
		}

		uint32_t fine = bsp_get_milli() ;
		PRINTF("%u tick\n", fine - inizio) ;
	}

	void CIRCO_tu(void)
	{
		CIRCO_iniz(&u.circ, MAX_BUFF) ;

		for (size_t i=0 ; i<sizeof(ctu) ; i++)
			ctu[i] = i ;

		PUTS("Questo fallisce") ;
		CIRBU_ins(&u.circ, ctu, 2 * MAX_BUFF) ;

		(void) CIRBU_ext(&u.circ, tmp, MAX_BUFF / 2) ;

		PUTS("Anche questo") ;
		CIRBU_ins(&u.circ, ctu, MAX_BUFF) ;

		while (true) {
			riempio_ed_estraggo() ;

			for (size_t i=0 ; i<MAX_DIM ; i++) {
				uint16_t dim = vDim[i] ;
				if (dim < MAX_BUFF / 2) {
					size_t cicli = ((MAX_BUFF * 100) + dim - 1) / dim ;

					inserisco_ed_estraggo(dim, cicli) ;
				}
				else
					break ;
			}
		}
	}

#endif

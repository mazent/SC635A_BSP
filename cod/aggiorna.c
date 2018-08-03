#define STAMPA_DBG
#include "aggiorna.h"
#include "esp_ota_ops.h"

#include "mbedtls/aes.h"
#include "mbedtls/md.h"

static const char *TAG = "agg";

static uint8_t * nuovo = NULL ;
static uint32_t dimNuovo = 0 ;
static uint8_t * fw = NULL ;
static uint32_t fwOfs ;
static uint32_t fwDim ;

// Dimensione delle due chiavi e di sha256
#define DIM_BLOCCO		32

static const uint8_t kcif[DIM_BLOCCO] = {
	0x57, 0xF4, 0x0F, 0xF2, 0x91, 0xBF, 0xDC, 0x8E, 0x69, 0x12, 0x1C, 0xC4, 0xE3, 0x99, 0x05, 0x05,
	0xEA, 0xEA, 0x82, 0x3A, 0x15, 0x1A, 0x39, 0x6B, 0xA9, 0xFE, 0xE4, 0x68, 0x18, 0x75, 0xF4, 0x08
};

static const uint8_t kmac[DIM_BLOCCO] = {
	0xB9, 0x16, 0x6E, 0x5F, 0xF1, 0x61, 0xFD, 0x0B, 0x5E, 0x19, 0xB7, 0xF7, 0x0D, 0xE9, 0x9C, 0x64,
	0xBE, 0x9D, 0x60, 0x44, 0x27, 0x5F, 0xFE, 0x09, 0xC6, 0xF4, 0x9F, 0x00, 0x30, 0x4B, 0xCE, 0xCB
};

static bool valido(void)
{
	bool esito = false ;
	const uint32_t DIM = dimNuovo - DIM_BLOCCO ;
	const void * firman = nuovo + DIM ;
	uint8_t firma[DIM_BLOCCO] ;

	if (0 == mbedtls_md_hmac(
			 mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
			 kmac, sizeof(kmac),
			 nuovo, DIM,
			 firma) ) {
		esito = 0 == memcmp(firman, firma, DIM_BLOCCO) ;
	}
	else {
		DBG_ERR ;
	}

	if (esito)
		dimNuovo = DIM ;

	return esito ;
}

static const char INIZIO[] = "208 " ;
static const uint32_t DIM_INIZIO = sizeof(INIZIO) - 1 ;

static const char FINE[] = " SC635" ;
static const uint32_t DIM_FINE = sizeof(FINE) - 1 ;

static bool decifra(void)
{
	bool esito = false ;
	mbedtls_aes_context aes = { 0 } ;
	uint8_t * iv = nuovo ;
	const uint8_t * ct = nuovo + 16 ;
	const uint32_t DIM = dimNuovo - 16 ;

	do {
		fwDim = DIM ;
		fw = os_malloc(fwDim) ;
		if (NULL == fw) {
			DBG_ERR ;
			break ;
		}

		mbedtls_aes_init(&aes) ;

		(void) mbedtls_aes_setkey_dec(&aes, kcif, 256) ;

		if (0 != mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, DIM, iv, ct, fw)) {
			DBG_ERR ;
			break ;
		}

		os_free(nuovo) ;
		nuovo = NULL ;

		char * pt = (char *) fw ;
		fwOfs = 0 ;
		if (0 != memcmp(pt, INIZIO, DIM_INIZIO)) {
			DBG_ERR ;
			break ;
		}
		pt += DIM_INIZIO ;
		fwOfs += DIM_INIZIO ;
		fwDim -= DIM_INIZIO ;

		if ('-' == *pt) {
			++pt ;
			++fwOfs ;
			--fwDim ;
		}
		else {
			while ('*' == *pt) {
				++pt ;
				++fwOfs ;
				--fwDim ;
			}
		}

		if (0 != memcmp(pt, FINE, DIM_FINE)) {
			DBG_ERR ;
			break ;
		}

		fwOfs += DIM_FINE ;
		fwDim -= DIM_FINE ;

		esito = true ;

	} while (false) ;

	mbedtls_aes_free(&aes) ;

	return esito ;
}

void AGG_beg(uint32_t dim)
{
	os_free(nuovo) ;
	os_free(fw) ;

	nuovo = os_malloc(dim) ;
	dimNuovo = dim ;
}

bool AGG_dat(const void * v, uint32_t dim, uint32_t ofs)
{
	bool esito = false ;

	do {
		if (NULL == nuovo)
			break ;

		if (NULL == v)
			break ;

		if (0 == dim)
			break ;

		if (dim + ofs > dimNuovo)
			break ;

		memcpy(nuovo + ofs, v, dim) ;

		esito = true ;

	} while (false) ;

	return esito ;
}

bool AGG_end(void)
{
	bool esito = false ;

	do {
		if (NULL == nuovo)
			break ;

		if ( !valido() ) {
			DBG_ERR ;
			break ;
		}

		if ( !decifra() ) {
			DBG_ERR ;
			break ;
		}

		const esp_partition_t * part = esp_ota_get_next_update_partition(NULL) ;
		if (NULL == part) {
			DBG_ERR ;
			break ;
		}
		ESP_LOGI(TAG, "partizione da %d byte", part->size) ;

		if (part->size < fwDim) {
			DBG_ERR ;
			break ;
		}

		esp_ota_handle_t update_handle = 0 ;
		esp_err_t err = esp_ota_begin(part, OTA_SIZE_UNKNOWN, &update_handle) ;
	    if (err != ESP_OK) {
			DBG_ERR ;
			break ;
	    }

	    err = esp_ota_write(update_handle, (const void *) (fw + fwOfs), fwDim) ;
	    if (err != ESP_OK) {
			DBG_ERR ;
			break ;
	    }

	    err = esp_ota_end(update_handle) ;
	    if (err != ESP_OK) {
			DBG_ERR ;
			break ;
	    }

	    err = esp_ota_set_boot_partition(part) ;
	    if (err != ESP_OK) {
			DBG_ERR ;
			break ;
	    }

	    os_free(fw) ;
	    fw = NULL ;

	    esito = true ;
	    //esp_restart();

	} while (false) ;

	return esito ;
}

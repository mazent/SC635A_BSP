#include "bsp.h"
#include "prod.h"

#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char * TAG = "prod";

// cfr sc635.csv
#define PARTITION		"prod"

// cfr prod.csv
#define NAMESPACE		"prod"
#define KEY_BSN			"scheda"
#define KEY_PSN			"prodotto"

static bool inited = false ;

static const nvs_handle INVALID_HANDLE = NOT(0) ;

static bool init(void)
{
	do {
		if (inited)
			break ;

	    esp_err_t err = nvs_flash_init_partition(PARTITION) ;
	    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
	    	ESP_LOGW(TAG, "cancello %s", PARTITION) ;
#if 0
	    	// This is a read-only partition!
	    	break ;
#else
	        // partition was truncated and needs to be erased
	        err = nvs_flash_erase_partition(PARTITION) ;
	        if (err != ESP_OK) {
	        	ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;
	        	break ;
	        }

	        // Retry nvs_flash_init
	        err = nvs_flash_init_partition(PARTITION) ;
		    if (err != ESP_OK)
		    	ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;
#endif
	    }
        inited = err == ESP_OK ;

	} while (false) ;

	return inited ;
}

bool PROD_read_board(PROD_BSN * p)
{
	bool esito = false ;
	nvs_handle h = INVALID_HANDLE ;

	do {
		if (NULL == p)
			break ;

		if ( !init() )
			break ;

		esp_err_t err = nvs_open_from_partition(PARTITION, NAMESPACE, NVS_READONLY, &h) ;
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;
			break ;
		}

		err = nvs_get_str(h, KEY_BSN, p->bsn, &p->len) ;
		if (err != ESP_OK)
			ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;

		esito = err == ESP_OK ;
	} while (false) ;

	if (INVALID_HANDLE != h)
		nvs_close(h) ;

    return esito ;
}

bool PROD_write_board(const char * p)
{
	bool esito = false ;
	nvs_handle h = INVALID_HANDLE ;

	do {
		if (NULL == p)
			break ;

		if ( !init() )
			break ;

		esp_err_t err = nvs_open_from_partition(PARTITION, NAMESPACE, NVS_READWRITE, &h) ;
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;
			break ;
		}

		err = nvs_set_str(h, KEY_BSN, p) ;
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;
			break ;
		}

		err = nvs_commit(h);
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;
			break ;
		}

		esito = true ;
	} while (false) ;

	if (INVALID_HANDLE != h)
		nvs_close(h) ;

    return esito ;

}

bool PROD_read_product(PROD_PSN * p)
{
	bool esito = false ;
	nvs_handle h = INVALID_HANDLE ;

	do {
		if (NULL == p)
			break ;

		if ( !init() )
			break ;

		esp_err_t err = nvs_open_from_partition(PARTITION, NAMESPACE, NVS_READONLY, &h) ;
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;
			break ;
		}

		err = nvs_get_str(h, KEY_PSN, p->psn, &p->len) ;
		esito = err == ESP_OK ;
	} while (false) ;

	if (INVALID_HANDLE != h)
		nvs_close(h) ;

    return esito ;
}

bool PROD_write_product(const char * p)
{
	bool esito = false ;
	nvs_handle h = INVALID_HANDLE ;

	do {
		if (NULL == p)
			break ;

		if ( !init() )
			break ;

		esp_err_t err = nvs_open_from_partition(PARTITION, NAMESPACE, NVS_READWRITE, &h) ;
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;
			break ;
		}

		err = nvs_set_str(h, KEY_PSN, p) ;
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;
			break ;
		}

		err = nvs_commit(h);
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "%s[%d]: %s", __FILE__, __LINE__, esp_err_to_name(err)) ;
			break ;
		}

		esito = true ;
	} while (false) ;

	if (INVALID_HANDLE != h)
		nvs_close(h) ;

    return esito ;
}

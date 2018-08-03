#include "cavo.h"

/*
 * Presenza del cavo ethernet nel
 * connettore RJ45
 */

#include "driver/gpio.h"


#define CAVORJ45_SEL		GPIO_SEL_36
#define CAVORJ45    		GPIO_NUM_36

static const gpio_config_t cfg = {
	.pin_bit_mask = CAVORJ45_SEL,
	.mode = GPIO_MODE_INPUT,
	// Basso == cavo inserito
	.intr_type = GPIO_INTR_ANYEDGE,
	// GPIO34-39 non hanno pull up/down
	.pull_up_en = GPIO_PULLUP_DISABLE,
	.pull_down_en = GPIO_PULLDOWN_DISABLE
};


static void vuota(void)
{
}

static PF_CRJ cbCRJ = vuota ;

static void IRAM_ATTR cavo_isr(void * v)
{
	UNUSED(v) ;

	cbCRJ() ;
}


bool CRJ_beg(PF_CRJ cb)
{
	bool esito = false ;

	assert(cb) ;
	if (cb)
		cbCRJ = cb ;

	do {
		if (ESP_OK != gpio_config(&cfg))
			break ;

		if (ESP_OK != gpio_isr_handler_add(CAVORJ45, cavo_isr, NULL))
			break ;

		esito = true ;
	} while (false) ;

	return esito ;
}

void CRJ_end(void)
{
	gpio_isr_handler_remove(CAVORJ45) ;

	cbCRJ = vuota ;
}

bool CRJ_in(void)
{
	return 0 == gpio_get_level(CAVORJ45) ;
}

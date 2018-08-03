#include "tasto.h"

/*
 * Il bottone viene mosso da un TPS3421EG,
 * che crea un prispolo basso
 */

#include "driver/gpio.h"


#define BUTTON_SEL		GPIO_SEL_39
#define BUTTON    		GPIO_NUM_39

static const gpio_config_t cfg = {
	.pin_bit_mask = BUTTON_SEL,
	.mode = GPIO_MODE_INPUT,
	// Alla fine torna alto
	.intr_type = GPIO_INTR_POSEDGE,
	// GPIO34-39 non hanno pull up/down
	.pull_up_en = GPIO_PULLUP_DISABLE,
	.pull_down_en = GPIO_PULLDOWN_DISABLE
};


static void tst_vuota(void)
{
}

static PF_TST cbTst = tst_vuota ;

static void IRAM_ATTR button_isr(void * v)
{
	UNUSED(v) ;

	cbTst() ;
}


bool TST_beg(PF_TST cb)
{
	bool esito = false ;

	assert(cb) ;
	if (cb)
		cbTst = cb ;

	do {
		if (ESP_OK != gpio_config(&cfg))
			break ;

		if (ESP_OK != gpio_isr_handler_add(BUTTON, button_isr, NULL))
			break ;

		esito = true ;
	} while (false) ;

	return esito ;
}

void TST_end(void)
{
	gpio_isr_handler_remove(BUTTON) ;

	CHECK_IT(ESP_OK == gpio_reset_pin(BUTTON)) ;

	cbTst = tst_vuota ;
}


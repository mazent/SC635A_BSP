#include "rid.h"

#include "driver/gpio.h"

#define FINE_SEL		GPIO_SEL_34
#define FINE    		GPIO_NUM_34

#define RISUL_SEL		GPIO_SEL_35
#define RISUL    		GPIO_NUM_35

#define INIZIO_SEL		GPIO_SEL_32
#define INIZIO    		GPIO_NUM_32


static const gpio_config_t cfg_ris = {
	.pin_bit_mask = RISUL_SEL,
	.mode = GPIO_MODE_INPUT,
	.intr_type = GPIO_INTR_DISABLE,
	// GPIO34-39 non hanno pull up/down
	.pull_up_en = GPIO_PULLUP_DISABLE,
	.pull_down_en = GPIO_PULLDOWN_DISABLE
};

static const gpio_config_t cfg_fin = {
	.pin_bit_mask = FINE_SEL,
	.mode = GPIO_MODE_INPUT,
	.intr_type = GPIO_INTR_NEGEDGE,
	// GPIO34-39 non hanno pull up/down
	.pull_up_en = GPIO_PULLUP_DISABLE,
	.pull_down_en = GPIO_PULLDOWN_DISABLE
};

static const gpio_config_t cfg = {
	.pin_bit_mask = INIZIO_SEL,
	.mode = GPIO_MODE_OUTPUT,
	.intr_type = GPIO_INTR_DISABLE,
	.pull_up_en = GPIO_PULLUP_DISABLE,
	.pull_down_en = GPIO_PULLDOWN_DISABLE
};


static void vuota(void)
{
}

static PF_RID cbRID = vuota ;

static enum {
	SCOLL,
	ATTESA,
	ESITO
} stato = SCOLL ;


static void IRAM_ATTR rid_isr(void * v)
{
	UNUSED(v) ;

	if (ATTESA == stato) {
		stato = ESITO ;

		cbRID() ;
	}
}

bool RID_beg(PF_RID cb)
{
	bool esito = false ;

	assert(cb) ;
	if (cb)
		cbRID = cb ;

	do {
		if (ESP_OK != gpio_config(&cfg))
			break ;
		gpio_set_level(INIZIO, 0) ;
		stato = SCOLL ;

		if (ESP_OK != gpio_config(&cfg_ris))
			break ;

		if (ESP_OK != gpio_config(&cfg_fin))
			break ;

		if (ESP_OK != gpio_isr_handler_add(FINE, rid_isr, NULL))
			break ;

		esito = true ;
	} while (false) ;

	return esito ;
}

void RID_end(void)
{
	gpio_isr_handler_remove(RISUL) ;
	CHECK_IT(ESP_OK == gpio_reset_pin(RISUL)) ;
	CHECK_IT(ESP_OK == gpio_reset_pin(FINE)) ;
	CHECK_IT(ESP_OK == gpio_reset_pin(INIZIO)) ;

	cbRID = vuota ;
}

bool RID_start(void)
{
	if (SCOLL == stato) {
		stato = ATTESA ;
		gpio_set_level(INIZIO, 1) ;

		return true ;
	}
	else
		return false ;
}

void RID_stop(void)
{
	stato = SCOLL ;
	gpio_set_level(INIZIO, 0) ;
}

bool RID_doip(bool * p)
{
	assert(p) ;

	if (NULL == p)
		return false ;
	else if (ESITO == stato) {
		*p = 0 != gpio_get_level(RISUL) ;
		return true ;
	}
	else
		return false ;
}


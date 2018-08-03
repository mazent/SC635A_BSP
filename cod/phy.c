#include "phy.h"

#include "driver/gpio.h"

#define PHY_SEL		GPIO_SEL_2
#define PHY    		GPIO_NUM_2

static const gpio_config_t cfg = {
	.pin_bit_mask = PHY_SEL,
	.mode = GPIO_MODE_OUTPUT,
	.intr_type = GPIO_INTR_DISABLE,
	.pull_up_en = GPIO_PULLUP_DISABLE,
	.pull_down_en = GPIO_PULLDOWN_DISABLE
};


bool PHY_beg(void)
{
	bool esito = ESP_OK == gpio_config(&cfg) ;

	if (esito)
		gpio_set_level(PHY, 0) ;

	return esito ;
}

void PHY_end(void)
{
	CHECK_IT(ESP_OK == gpio_reset_pin(PHY)) ;
}

void PHY_reset(uint8_t dms)
{
	gpio_set_level(PHY, 1) ;

	osDelay(dms * 10) ;

	gpio_set_level(PHY, 0) ;
}

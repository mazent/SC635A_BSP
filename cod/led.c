#include "led.h"

#include "driver/gpio.h"

#define LED_SEL		GPIO_SEL_4
#define LED    		GPIO_NUM_4

static const gpio_config_t cfg = {
	.pin_bit_mask = LED_SEL,
	.mode = GPIO_MODE_OUTPUT,
	.intr_type = GPIO_INTR_DISABLE,
	.pull_up_en = GPIO_PULLUP_DISABLE,
	.pull_down_en = GPIO_PULLDOWN_DISABLE
};


bool LED_beg(void)
{
	return ESP_OK == gpio_config(&cfg) ;
}

void LED_end(void)
{
	CHECK_IT(ESP_OK == gpio_reset_pin(LED)) ;
}

void LED_rosso(bool si)
{
	gpio_set_level(LED, si ? 1 : 0) ;
}

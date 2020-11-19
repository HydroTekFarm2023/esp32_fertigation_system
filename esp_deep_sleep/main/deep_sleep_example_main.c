#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp32/ulp.h"
#include "driver/gpio.h"
#include "driver/touch_pad.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "soc/sens_periph.h"
#include "soc/rtc.h"

/* GPIO2 is used to enter into deep sleep.
   User can change this macro as per application requirement */
#define GPIO_INPUT_IO    2

#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO)

#define INTR_FLAG_DEFAULT 0

/* GPIO2 is used to wakeup from deep sleep.
User can change this macro as per application requirement */
#define ext_wakeup_pin 2

#define ext_wakeup_pin_mask (1ULL << ext_wakeup_pin)

/* Variable used to detect whether GPIO2 interrupt is triggered or not */
bool intr_triggered = false;

static void IRAM_ATTR gpio_isr_handler (void* arg)
{
	intr_triggered = true;
}

void app_main(void)
{
	gpio_config_t gpio_conf;
	
	//interrupt of rising edge
    gpio_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
	//bit mask of the pins, used GPIO2 here.
	gpio_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	//set as input mode
	gpio_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
	gpio_conf.pull_up_en = 1;
	gpio_config(&gpio_conf);
	
	//install gpio isr service
    gpio_install_isr_service(INTR_FLAG_DEFAULT);
	
	//isr handler for specific gpio pin (GPIO2)
    gpio_isr_handler_add(GPIO_INPUT_IO, gpio_isr_handler, (void*) GPIO_INPUT_IO);
	
	/* Detect whether boot is caused because of deep sleep reset or not */
	if((esp_sleep_get_wakeup_cause()) == ESP_SLEEP_WAKEUP_EXT1)
	{
		uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
        if (wakeup_pin_mask != 0)
		{
			int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
            printf("Wake up from GPIO %d\n", pin);
		}	
	}
	else
	{
		printf("Not a deep sleep reset\n");
	}
	
	while(1)
	{
		if(intr_triggered == true)
		{
			intr_triggered = false;
			printf("Enabling EXT1 wakeup on pins GPIO%d \n", ext_wakeup_pin);
			esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
			printf("Entering deep sleep\n");
			esp_deep_sleep_start();
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}	
}
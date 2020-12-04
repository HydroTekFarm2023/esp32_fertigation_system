#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp32/ulp.h"
#include "driver/gpio.h"
#include "driver/touch_pad.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "soc/sens_periph.h"
#include "soc/rtc.h"
#include "driver/timer.h"
#include "driver/periph_ctrl.h"

/* GPIO2 is used to enter into deep sleep.
   User can change this macro as per application requirement */
#define GPIO_INPUT_IO         4

#define GPIO_INPUT_PIN_SEL    (1ULL<<GPIO_INPUT_IO)

#define INTR_FLAG_DEFAULT     0

/* GPIO2 is used to wakeup from deep sleep.
User can change this macro as per application requirement */
#define ext_wakeup_pin        GPIO_INPUT_IO

#define ext_wakeup_pin_mask   (1ULL << ext_wakeup_pin)

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (3)      // sample test interval for the first timer

/*
 * Timer group0 ISR handler
 */
void IRAM_ATTR timer_group0_isr(void *para)
{
   // clear timer interrupt
   timer_group_intr_clr_in_isr(TIMER_GROUP_0, TIMER_0);
   // configure GPIO for wakeup device from sleep mode
   esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
   // start sleep mode
   esp_deep_sleep_start();
}

static void IRAM_ATTR gpio_isr_handler (void* arg)
{
   // reset last timer
   timer_pause(TIMER_GROUP_0, TIMER_0);
   // reset timer counter
   timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
   // restart/start timer
   timer_start(TIMER_GROUP_0, TIMER_0);
}

static void tg0_timer_init(int timer_idx, double timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = 0,
    }; // default clock source is APB
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* Timer's counter will initially start from value below. */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
          (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);
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

   tg0_timer_init(TIMER_0, TIMER_INTERVAL0_SEC);

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
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

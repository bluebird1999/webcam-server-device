/*
 * gpio_control.h
 *
 *  Created on: Sep 24, 2020
 *      Author: lijunxin
 */

#ifndef GPIO_CONTRIL_H_
#define GPIO_CONTRIL_H_

/*
 * header
 */
#include "config.h"

/*
 * define
 */
#define LED_OFF					0
#define LED_ON					1
#define LED_FLASH				1

#define GPIO_ON 		1
#define GPIO_OFF 		0

#define LED1 			1
#define LED2 			2
/*
 * structure
 */


/*
 * function
 */

int init_led_gpio(device_config_t *rconfig);
int uninit_led_gpio();
int set_blue_led_on();
int set_blue_led_off();
int set_orange_led_on();
int set_orange_led_off();
int ctl_spk_enable(int on_off);
int ctl_ircut(int on_off);
int ctl_irled(int on_off);
int ctl_motor595_enable(int on_off);
int get_led_status(int led_index);

#endif /* GPIO_CONTRIL_H_ */

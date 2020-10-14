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

/*
 * define
 */

/*
 * structure
 */


/*
 * function
 */

int init_led_gpio();
int uninit_led_gpio();
int set_blue_led_on();
int set_blue_led_off();
int set_orange_led_on();
int set_orange_led_off();
int ctl_spk_enable(int on_off);
int ctl_ircut(int on_off);
int ctl_irled(int on_off);

#endif /* GPIO_CONTRIL_H_ */

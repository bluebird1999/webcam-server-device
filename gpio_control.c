/*
* @Author: Marte
* @Date:   2020-09-24 10:56:09
* @Last Modified by:   Marte
* @Last Modified time: 2020-09-24 18:18:43
*/

#include <stdio.h>
#include <sys/vfs.h>
#include <string.h>
#include <sys/mount.h>
#include <time.h>
#include "../../tools/tools_interface.h"
#include "rts_io_gpio.h"
#include "gpio_control.h"
#include "device_interface.h"

//define
#define GPIO_OUTPUT     		1
#define LED_ON					1
#define LED_OFF					0
#define DOMAIN_GPIO_SYSTEM 		0
#define LED1_GPIO				9 //blue
#define LED2_GPIO				22 //orange
#define SPK_GPIO				11

//static
static struct rts_gpio *rts_gpio_led1 = NULL;
static struct rts_gpio *rts_gpio_led2 = NULL;
static struct rts_gpio *rts_gpio_spk  = NULL;

static int set_gpio_value(struct rts_gpio *rts_gpio, int value);

int ctl_spk_enable(int on_off)
{
	return set_gpio_value(rts_gpio_spk, on_off);
}

int set_blue_led_on()
{
	return set_gpio_value(rts_gpio_led1, LED_ON);
}

int set_blue_led_off()
{
	return set_gpio_value(rts_gpio_led1, LED_OFF);
}

int set_orange_led_on()
{
	return set_gpio_value(rts_gpio_led2, LED_ON);
}

int set_orange_led_off()
{
	return set_gpio_value(rts_gpio_led2, LED_OFF);
}

int init_led_gpio()
{
	//led1
	rts_gpio_led1 = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, LED1_GPIO);
	if(!rts_gpio_led1)
	{
		log_err("can not requset gpio num %d\n", LED1_GPIO);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_led1, GPIO_OUTPUT))
	{
		log_err("can not set gpio %d dir\n", LED1_GPIO);
		rts_io_gpio_free(rts_gpio_led1);
		return -1;
	}

	//led2
	rts_gpio_led2 = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, LED2_GPIO);
	if(!rts_gpio_led2)
	{
		log_err("can not requset gpio num %d\n", LED2_GPIO);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_led2, GPIO_OUTPUT))
	{
		log_err("can not set gpio %d dir\n", LED2_GPIO);
		rts_io_gpio_free(rts_gpio_led2);
		return -1;
	}

	//spk
	rts_gpio_spk = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, SPK_GPIO);
	if(!rts_gpio_spk)
	{
		log_err("can not requset gpio num %d\n", SPK_GPIO);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_spk, GPIO_OUTPUT))
	{
		log_err("can not set gpio %d dir\n", SPK_GPIO);
		rts_io_gpio_free(rts_gpio_spk);
		return -1;
	}
	return 0;
}

int uninit_led_gpio()
{
	rts_io_gpio_free(rts_gpio_led1);
	rts_io_gpio_free(rts_gpio_led2);
	rts_io_gpio_free(rts_gpio_spk);

	return 0;
}

int set_gpio_value(struct rts_gpio *rts_gpio, int value)
{
	if(rts_gpio == NULL)
		return -1;

	if(rts_io_gpio_set_value(rts_gpio, value))
	{
		log_err("set gpio failed\n");
		rts_io_gpio_free(rts_gpio);
		return -1;
	}

	if(rts_io_gpio_get_value(rts_gpio) != value)
		log_err("get gpio value failed\n");

	return 0;
}




















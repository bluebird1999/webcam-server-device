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
#define IRCUR_AIN				16
#define IRCUR_BIN				17
#define IRLED					6
#define MOTOR_595ENABLE			0

//static
static struct rts_gpio *rts_gpio_led1 			= NULL;
static struct rts_gpio *rts_gpio_led2 			= NULL;
static struct rts_gpio *rts_gpio_spk  			= NULL;
static struct rts_gpio *rts_gpio_ircutain  		= NULL;
static struct rts_gpio *rts_gpio_ircutbin  		= NULL;
static struct rts_gpio *rts_gpio_irled		  	= NULL;
static struct rts_gpio *rts_gpio_motorable		= NULL;

static int set_gpio_value(struct rts_gpio *rts_gpio, int value);

int ctl_ircut(int on_off)
{
	int ret;
	//open
	if(on_off == 1)
	{
		ret = set_gpio_value(rts_gpio_ircutain, LED_ON);
		if(ret)
		{
			log_qcy(DEBUG_SERIOUS, "set gpio %d failed\n",IRCUR_AIN);
			return -1;
		}
		ret = set_gpio_value(rts_gpio_ircutbin, LED_OFF);
		if(ret)
		{
			log_qcy(DEBUG_SERIOUS, "set gpio %d failed\n",IRCUR_BIN);
			return -1;
		}
	} else if(on_off == 0){
		ret = set_gpio_value(rts_gpio_ircutbin, LED_ON);
		if(ret)
		{
			log_qcy(DEBUG_SERIOUS, "set gpio %d failed\n",IRCUR_AIN);
			return -1;
		}
		ret = set_gpio_value(rts_gpio_ircutain, LED_OFF);
		if(ret)
		{
			log_qcy(DEBUG_SERIOUS, "set gpio %d failed\n",IRCUR_BIN);
			return -1;
		}
	}

	return ret;
}

int ctl_irled(int on_off)
{
	//1 -> disable
	//0 -> enable
	if(on_off != 0 && on_off != 1)
		return -1;
	return set_gpio_value(rts_gpio_irled, !on_off);
}

int ctl_motor595_enable(int on_off)
{
	//1 -> disable
	//0 -> enable
	if(on_off != 0 && on_off != 1)
		return -1;
	return set_gpio_value(rts_gpio_motorable, !on_off);
}

int ctl_spk_enable(int on_off)
{
	if(on_off != 0 && on_off != 1)
		return -1;
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
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", LED1_GPIO);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_led1, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", LED1_GPIO);
		rts_io_gpio_free(rts_gpio_led1);
		return -1;
	}

	//led2
	rts_gpio_led2 = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, LED2_GPIO);
	if(!rts_gpio_led2)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", LED2_GPIO);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_led2, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", LED2_GPIO);
		rts_io_gpio_free(rts_gpio_led2);
		return -1;
	}

	//spk
	rts_gpio_spk = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, SPK_GPIO);
	if(!rts_gpio_spk)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", SPK_GPIO);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_spk, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", SPK_GPIO);
		rts_io_gpio_free(rts_gpio_spk);
		return -1;
	}

	//ircutain
	rts_gpio_ircutain = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, IRCUR_AIN);
	if(!rts_gpio_ircutain)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", IRCUR_AIN);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_ircutain, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", IRCUR_AIN);
		rts_io_gpio_free(rts_gpio_ircutain);
		return -1;
	}

	//ircutbin
	rts_gpio_ircutbin = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, IRCUR_BIN);
	if(!rts_gpio_ircutbin)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", IRCUR_BIN);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_ircutbin, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", IRCUR_BIN);
		rts_io_gpio_free(rts_gpio_ircutbin);
		return -1;
	}

	//irled
	rts_gpio_irled = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, IRLED);
	if(!rts_gpio_irled)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", IRLED);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_irled, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", IRLED);
		rts_io_gpio_free(rts_gpio_irled);
		return -1;
	}

	//motor_enable
	rts_gpio_motorable = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, MOTOR_595ENABLE);
	if(!rts_gpio_motorable)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", MOTOR_595ENABLE);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_motorable, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", MOTOR_595ENABLE);
		rts_io_gpio_free(rts_gpio_motorable);
		return -1;
	}

	return 0;
}

int uninit_led_gpio()
{
	if(rts_gpio_led1)
		rts_io_gpio_free(rts_gpio_led1);
	if(rts_gpio_led2)
		rts_io_gpio_free(rts_gpio_led2);
	if(rts_gpio_spk)
		rts_io_gpio_free(rts_gpio_spk);
	if(rts_gpio_ircutain)
		rts_io_gpio_free(rts_gpio_ircutain);
	if(rts_gpio_ircutbin)
		rts_io_gpio_free(rts_gpio_ircutbin);
	if(rts_gpio_irled)
		rts_io_gpio_free(rts_gpio_irled);
	if(rts_gpio_motorable)
		rts_io_gpio_free(rts_gpio_motorable);

	return 0;
}

int set_gpio_value(struct rts_gpio *rts_gpio, int value)
{
	if(rts_gpio == NULL)
		return -1;

	if(rts_io_gpio_set_value(rts_gpio, value))
	{
		log_qcy(DEBUG_SERIOUS, "set gpio failed\n");
		rts_io_gpio_free(rts_gpio);
		return -1;
	}

	if(rts_io_gpio_get_value(rts_gpio) != value)
		log_qcy(DEBUG_SERIOUS, "get gpio value failed\n");

	return 0;
}




















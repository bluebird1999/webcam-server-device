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
#define DOMAIN_GPIO_SYSTEM 		0
#define DOMAIN_GPIO_MCU 		1
//#define LED1_GPIO				9 //blue
//#define LED2_GPIO				22 //orange
//#define SPK_GPIO				11
//#define IRCUR_AIN				16
//#define IRCUR_BIN				17
//#define IRLED					6
//#define MOTOR_595ENABLE			0

//static
static struct rts_gpio *rts_gpio_led1 			= NULL;
static struct rts_gpio *rts_gpio_led2 			= NULL;
static struct rts_gpio *rts_gpio_spk  			= NULL;
static struct rts_gpio *rts_gpio_ircutain  		= NULL;
static struct rts_gpio *rts_gpio_ircutbin  		= NULL;
static struct rts_gpio *rts_gpio_irled		  	= NULL;
static struct rts_gpio *rts_gpio_motorable		= NULL;

static int led1_status = 0;
static int led2_status = 0;

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
			log_qcy(DEBUG_SERIOUS, "set gpio failed\n");
			return -1;
		}
		ret = set_gpio_value(rts_gpio_ircutbin, LED_OFF);
		if(ret)
		{
			log_qcy(DEBUG_SERIOUS, "set gpio failed\n");
			return -1;
		}
	} else if(on_off == 0){
		ret = set_gpio_value(rts_gpio_ircutbin, LED_ON);
		if(ret)
		{
			log_qcy(DEBUG_SERIOUS, "set gpio failed\n");
			return -1;
		}
		ret = set_gpio_value(rts_gpio_ircutain, LED_OFF);
		if(ret)
		{
			log_qcy(DEBUG_SERIOUS, "set gpio failed\n");
			return -1;
		}
	}

	return ret;
}

int ctl_irled(device_config_t *rconfig, int on_off)
{
	//1 -> enable
	//0 -> disable
	if(on_off != 0 && on_off != 1)
		return -1;
	return set_gpio_value(rts_gpio_irled, rconfig->irled_effect_level ? (on_off ? LED_ON : LED_OFF): (on_off ? LED_OFF : LED_ON));
}

int ctl_motor595_enable(device_config_t *rconfig, int on_off)
{
	//1 -> disable
	//0 -> enable
	if(on_off != 0 && on_off != 1)
		return -1;
	return set_gpio_value(rts_gpio_motorable, rconfig->moto_595_effect_level ? (on_off ? LED_ON : LED_OFF): (on_off ? LED_OFF : LED_ON));
}

int get_led_status(int led_index)
{
	if(led_index == 1)
		return led1_status;
	else if(led_index == 2)
		return led2_status;

	return -1;
}

int ctl_spk_enable(device_config_t *rconfig, int on_off)
{
	//1 -> enable
	//0 -> disable
	if(on_off != 0 && on_off != 1)
		return -1;
	if(rts_gpio_spk != NULL)
		return set_gpio_value(rts_gpio_spk, rconfig->spk_effect_level ? (on_off ? LED_ON : LED_OFF): (on_off ? LED_OFF : LED_ON));
	else
		return 0;
}

int set_blue_led_status(device_config_t *rconfig, int on_off)
{
	if(led1_status == on_off)
		return 0;
	led1_status = on_off;
	return set_gpio_value(rts_gpio_led1, rconfig->led1_effect_level ? (on_off ? LED_ON : LED_OFF): (on_off ? LED_OFF : LED_ON) );
}

int set_orange_led_status(device_config_t *rconfig, int on_off)
{
	if(led2_status == on_off)
		return 0;
	led2_status = on_off;
	return set_gpio_value(rts_gpio_led2, rconfig->led2_effect_level ? (on_off ? LED_ON : LED_OFF): (on_off ? LED_OFF : LED_ON) );
}

int init_led_gpio(device_config_t *rconfig)
{
	//led1
	if(rconfig->led1_gpio_mcu)
		rts_gpio_led1 = rts_io_gpio_request(DOMAIN_GPIO_MCU, rconfig->led1_gpio);
	else
		rts_gpio_led1 = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, rconfig->led1_gpio);
	if(!rts_gpio_led1)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", rconfig->led1_gpio);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_led1, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", rconfig->led1_gpio);
		rts_io_gpio_free(rts_gpio_led1);
		return -1;
	}

	//led2
	if(rconfig->led2_gpio_mcu)
		rts_gpio_led2 = rts_io_gpio_request(DOMAIN_GPIO_MCU, rconfig->led2_gpio);
	else
		rts_gpio_led2 = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, rconfig->led2_gpio);
	if(!rts_gpio_led2)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", rconfig->led2_gpio);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_led2, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", rconfig->led2_gpio);
		rts_io_gpio_free(rts_gpio_led2);
		return -1;
	}

	//spk
	if(rconfig->spk_gpio_mcu)
		rts_gpio_spk = rts_io_gpio_request(DOMAIN_GPIO_MCU, rconfig->spk_gpio);
	else
		rts_gpio_spk = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, rconfig->spk_gpio);
	if(!rts_gpio_spk)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", rconfig->spk_gpio);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_spk, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", rconfig->spk_gpio);
		rts_io_gpio_free(rts_gpio_spk);
		return -1;
	}

	//ircutain
	if(rconfig->ircut_ain_mcu)
		rts_gpio_ircutain = rts_io_gpio_request(DOMAIN_GPIO_MCU, rconfig->ircut_ain);
	else
		rts_gpio_ircutain = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, rconfig->ircut_ain);
	if(!rts_gpio_ircutain)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", rconfig->ircut_ain);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_ircutain, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", rconfig->ircut_ain);
		rts_io_gpio_free(rts_gpio_ircutain);
		return -1;
	}

	//ircutbin
	if(rconfig->ircut_bin_mcu)
		rts_gpio_ircutbin = rts_io_gpio_request(DOMAIN_GPIO_MCU, rconfig->ircut_bin);
	else
		rts_gpio_ircutbin = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, rconfig->ircut_bin);
	if(!rts_gpio_ircutbin)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", rconfig->ircut_bin);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_ircutbin, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", rconfig->ircut_bin);
		rts_io_gpio_free(rts_gpio_ircutbin);
		return -1;
	}

	//irled
	if(rconfig->irled_mcu)
		rts_gpio_irled = rts_io_gpio_request(DOMAIN_GPIO_MCU, rconfig->irled);
	else
		rts_gpio_irled = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, rconfig->irled);
	if(!rts_gpio_irled)
	{
		log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", rconfig->irled);
		return -1;
	}

	if(rts_io_gpio_set_direction(rts_gpio_irled, GPIO_OUTPUT))
	{
		log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", rconfig->irled);
		rts_io_gpio_free(rts_gpio_irled);
		return -1;
	}

	if(rconfig->motor_enable)
	{
//		//motor_enable
//		if(rconfig->motor_595enable_mcu)
//			rts_gpio_motorable = rts_io_gpio_request(DOMAIN_GPIO_MCU, rconfig->motor_595enable);
//		else
//			rts_gpio_motorable = rts_io_gpio_request(DOMAIN_GPIO_SYSTEM, rconfig->motor_595enable);
//		if(!rts_gpio_motorable)
//		{
//			log_qcy(DEBUG_SERIOUS, "can not requset gpio num %d\n", rconfig->motor_595enable);
//			return -1;
//		}
//
//		if(rts_io_gpio_set_direction(rts_gpio_motorable, GPIO_OUTPUT))
//		{
//			log_qcy(DEBUG_SERIOUS, "can not set gpio %d dir\n", rconfig->motor_595enable);
//			rts_io_gpio_free(rts_gpio_motorable);
//			return -1;
//		}
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




















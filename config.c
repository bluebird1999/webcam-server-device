/*
 * config_device.c
 *
 *  Created on: Nov 13, 2020
 *      Author: lijunxin
 */

/*
 * header
 */
//system header
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
//program header
#include "../../tools/tools_interface.h"
//#include "../../manager/manager_interface.h"
#include "../../manager/global_interface.h"
//server header
#include "config.h"

/*
 * static
 */
//variable
static pthread_rwlock_t			lock;
static device_config_t			device_config;
static config_map_t device_config_map[] = {
	{"volume_step",     			&(device_config.volume_step),      			cfg_u32, 		"10",0,0,100,},
	{"day_night_lim",     			&(device_config.day_night_lim),      		cfg_u32, 		"3000",0,0,3100,},
	{"motor_enable",      			&(device_config.motor_enable),       		cfg_u32,		"1",0,0,1,},
	{"motor_step",      			&(device_config.motor_step),       			cfg_u32,		"150",0,0,500,},
	{"motor_speed",      			&(device_config.motor_speed),   			cfg_u32,		"1",0,0,4,},
	{"led1_gpio",      				&(device_config.led1_gpio),   				cfg_u32,		"9",0,0,100,},
	{"led2_gpio",      				&(device_config.led2_gpio),   				cfg_u32,		"22",0,0,100,},
	{"spk_gpio",      				&(device_config.spk_gpio),   				cfg_u32,		"11",0,0,100,},
	{"ircut_ain",      				&(device_config.ircut_ain),   				cfg_u32,		"16",0,0,100,},
	{"ircut_bin",      				&(device_config.ircut_bin),   				cfg_u32,		"17",0,0,100,},
	{"irled",      					&(device_config.irled),   					cfg_u32,		"6",0,0,100,},
	{"motor_595enable",      		&(device_config.motor_595enable),   		cfg_u32,		"0",0,0,100,},
	{"user_mount_path",     		&(device_config.user_mount_path),      		cfg_string, 	"/opt",0,0,10,},
	{"sd_mount_path",  				&(device_config.sd_mount_path),				cfg_string, 	"/mnt/media",0,0,1000,},
    {NULL,},
};
//function

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * interface
 */
int config_device_read(device_config_t *rconfig)
{
	int ret,ret1=0;
	pthread_rwlock_init(&lock, NULL);
	ret = pthread_rwlock_wrlock(&lock);
	if(ret)	{
		log_qcy(DEBUG_SERIOUS, "add lock fail, ret = %d\n", ret);
		return ret;
	}
	ret = read_config_file(device_config_map, CONFIG_DEVICE_CONFIG_PATH);
	ret1 = pthread_rwlock_unlock(&lock);
	if (ret1)
		log_qcy(DEBUG_SERIOUS, "add unlock fail, ret = %d\n", ret);
	memcpy(rconfig,&device_config,sizeof(device_config_t));
	return ret;
}

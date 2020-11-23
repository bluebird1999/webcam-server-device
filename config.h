/*
 * config_device.h
 *
 *  Created on: Nov 13, 2020
 *      Author: lijunxin
 */

#ifndef DEVICE_CONFIG_H_
#define DEVICE_CONFIG_H_

/*
 * header
 */
#include "string.h"
//#include "device_interface.h"

/*
 * define
 */
#define 	CONFIG_DEVICE_CONFIG_PATH			"/opt/qcy/config/device_profile.config"

/*
 * structure
 */
typedef struct device_config_t {
	unsigned int			volume_step;
	unsigned int			day_night_lim;
	unsigned int			storage_detect;
	unsigned int			storage_detect_notify;
	unsigned int			storage_detect_lim;
	unsigned int			motor_enable;
	unsigned int			motor_step;
	unsigned int			motor_speed;
	unsigned int			led1_gpio;
	unsigned int			led2_gpio;
	unsigned int			spk_gpio;
	unsigned int			ircut_ain;
	unsigned int			ircut_bin;
	unsigned int			irled;
	unsigned int			motor_595enable;
	char					user_mount_path[64];
	char					sd_mount_path[64];
} device_config_t;
/*
 * function
 */
int config_device_read(device_config_t*);

#endif /* DEVICE_CONFIG_H_ */
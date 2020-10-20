/*
 * device_interface.h
 *
 *  Created on: Sep 24, 2020
 *      Author: lijunxin
 */

#ifndef SERVER_DEVICE_INTERFACE_H_
#define SERVER_DEVICE_INTERFACE_H_

/*
 * header
 */
#include "../../manager/manager_interface.h"

/*
 * define
 */
#define		SERVER_DEVICE_VERSION_STRING		"alpha-3.4"

//control
#define		DEVICE_CTRL_SD_INFO					0x0001
#define		DEVICE_CTRL_LED						0x0003
#define		DEVICE_CTRL_AMPLIFIER				0x0009
#define		DEVICE_CTRL_ADJUST_AUDIO_VOLUME		0x000a
#define		DEVICE_CTRL_PART_INFO				0x000c
#define		DEVICE_CTRL_PART_USER_INFO			0x000d
#define		DEVICE_CTRL_IR_SWITCH				0x0007
#define		DEVICE_CTRL_IR_MODE					0x0008
#define  	DEVICE_CTRL_MOTOR_HOR_RIGHT			0x000b
#define  	DEVICE_CTRL_MOTOR_HOR_LEFT			0x000f
#define 	DEVICE_CTRL_MOTOR_VER_UP			0x0011
#define 	DEVICE_CTRL_MOTOR_VER_DOWN			0x0012
//need times, about 48s
#define 	DEVICE_CTRL_MOTOR_RESET				0x0013

#define		DEVICE_ACTION_USER_FORMAT			0x000e
#define		DEVICE_ACTION_SD_FORMAT				0x0002


//message
#define		MSG_DEVICE_BASE						(SERVER_DEVICE<<16)
#define		MSG_DEVICE_SIGINT					MSG_DEVICE_BASE | 0x0000
#define		MSG_DEVICE_SIGINT_ACK				MSG_DEVICE_BASE | 0x1000
#define		MSG_DEVICE_GET_PARA					MSG_DEVICE_BASE | 0x0010
#define		MSG_DEVICE_GET_PARA_ACK				MSG_DEVICE_BASE | 0x1010
#define		MSG_DEVICE_ACTION					MSG_DEVICE_BASE | 0x0020
#define		MSG_DEVICE_ACTION_ACK				MSG_DEVICE_BASE | 0x1020
#define		MSG_DEVICE_CTRL_DIRECT				MSG_DEVICE_BASE | 0x0012
#define		MSG_DEVICE_CTRL_DIRECT_ACK			MSG_DEVICE_BASE | 0x1012

/*
 * structure
 */
/* if type  > 0,  volume must be -1;
 * if volume >= 0,  event must be 0;
 *
 * int_out : control mic volume or speaker volume
 * 			 0 -> mic
 * 			 1 -> speaker
 *
 * type:  0 -> no event, control by volume
 * 		  1 -> volume up
 * 		  2 -> volume down
 * 		  3 -> voulme mute
 * 		  4 -> volume unmute
 * volume: -1 -> no volume, control by event
 * 		   >= 0 && <= 100 -> set volume (range 0 ~ 100)
 */
//audio		------------------------------------------------------------
typedef struct audio_info_t {
	unsigned int 	in_out;
	unsigned int 	type;
			 int	volume;
} audio_info_t;

//sd		------------------------------------------------------------
typedef struct sd_info_ack_t {
	unsigned long	plug;
	unsigned long	totalBytes;
	unsigned long	usedBytes;
	unsigned long	freeBytes;
} sd_info_ack_t;

//part 		------------------------------------------------------------
typedef struct part_info
{
    char name[16];
    unsigned int size;  //KB in units
}part_info_t;

typedef struct part_msg_info
{
    int part_num;
    part_info_t part_info_sum[10];
}part_msg_info_t;

//iot struct ------------------------------------------------------------
typedef struct device_iot_config_t {
	int ir_mode;
	int ircut_onoff;
	int led1_onoff;
	int led2_onoff;
	int amp_on_off;
	sd_info_ack_t 		sd_iot_info;
	sd_info_ack_t 		user_part_iot_info;
	audio_info_t 		audio_iot_info;
	part_msg_info_t		part_iot_info;
} device_iot_config_t;

/*
 * function
 */
//need times, about 50s
int server_device_start(void);
int server_device_message(message_t *msg);

#endif /* SERVER_DEVICE_INTERFACE_H_ */

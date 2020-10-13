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

#define		DEVICE_CTRL_SD_INFO					0x0001


//#define		MSG_DEVICE_WIFI						MSG_DEVICE_BASE | 0x0003
//#define		MSG_DEVICE_GET_WIFISTATUS			MSG_DEVICE_BASE | 0x0004
//#define		MSG_DEVICE_GET_WIFISTATUS_ACK		MSG_DEVICE_BASE | 0x1004
//
//#define		MSG_DEVICE_GET_IRLEDSTATUS			MSG_DEVICE_BASE | 0x0005
//#define		MSG_DEVICE_GET_IRLEDSTATUS_ACK		MSG_DEVICE_BASE | 0x1005
//#define		MSG_DEVICE_IRLED					MSG_DEVICE_BASE | 0x0006
//
//#define		MSG_DEVICE_IRCUT					MSG_DEVICE_BASE | 0x0007
//#define		MSG_DEVICE_GET_IRCUTSTATUS			MSG_DEVICE_BASE | 0x0008
//#define		MSG_DEVICE_GET_IRCUTSTATUS_ACK		MSG_DEVICE_BASE | 0x1008

#define		DEVICE_CTRL_AMPLIFIER				0x0009
#define		DEVICE_CTRL_ADJUST_AUDIO_VOLUME		0x000a

//#define		MSG_DEVICE_MOTOR					MSG_DEVICE_BASE | 0x000b
#define		DEVICE_CTRL_PART_INFO				0x000c
#define		DEVICE_CTRL_PART_USER_INFO			0x000d


#define		DEVICE_ACTION_USER_FORMAT				0x000e
#define		DEVICE_ACTION_SD_FORMAT				0x0002


#define		MSG_DEVICE_LED1_ON					MSG_DEVICE_BASE | 0x100f
#define		MSG_DEVICE_LED1_OFF					MSG_DEVICE_BASE | 0x200f
#define		MSG_DEVICE_LED2_ON					MSG_DEVICE_BASE | 0x300f
#define		MSG_DEVICE_LED2_OFF					MSG_DEVICE_BASE | 0x400f

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
 * if volume > 0,  event must be 0;
 *
 * type:  0 -> no event, control by volume
 * 		  1 -> volume up
 * 		  2 -> volume down
 * 		  3 -> voulme mute
 * 		  4 -> volume unmute
 * volume: -1 -> no volume, control by event
 * 		   >=0 -> set volume (range 0 ~ 100)
 */
//audio	------------------------------------------------------------
typedef struct audio_info_t {
	unsigned int 	type;
	unsigned int	volume;
} audio_info_t;

//sd	------------------------------------------------------------
typedef struct sd_info_ack_t {
	unsigned long	plug;
	unsigned long	totalBytes;
	unsigned long	usedBytes;
	unsigned long	freeBytes;
} sd_info_ack_t;

//part 	------------------------------------------------------------
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
	int on_off;
	sd_info_ack_t 		sd_iot_info;
	sd_info_ack_t 		user_part_iot_info;
	audio_info_t 		audio_iot_info;
	part_msg_info_t		part_iot_info;
} device_iot_config_t;

/*
 * function
 */
int server_device_start(void);
int server_device_message(message_t *msg);

#endif /* SERVER_DEVICE_INTERFACE_H_ */

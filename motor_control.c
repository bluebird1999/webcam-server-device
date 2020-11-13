/*
* @Author: Marte
* @Date:   2020-09-24 10:56:09
* @Last Modified by:   Marte
* @Last Modified time: 2020-09-24 18:18:43
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include "../../tools/tools_interface.h"
#include "motor_control.h"
#include "gpio_control_interface.h"
#include "device_interface.h"
#include "config.h"

static int fd = 0;
static ptzctrl_info_t ptz_info;
static int motor_status = MOTOR_NONE;

int motor_reset()
{
	int ret = 0;

	if(motor_status != MOTOR_READY)
	{
		log_qcy(DEBUG_SERIOUS, "motor not ready");
		return -1;
	}

	motor_status = MOTOR_RESET;
	//need time, must wait dirver finish
	ioctl(fd, RTS_PTZ_IOC_RESET, NULL);
	sleep(5);

	motor_status = MOTOR_READY;
	return ret;
}

int init_motor()
{
	int ret = 0;

	memset(&ptz_info, 0, sizeof(ptzctrl_info_t));

	//enable 595
	ctl_motor595_enable(1);

	fd = open("/dev/rts-ptz", O_RDWR);
	if(!fd)
	{
		log_qcy(DEBUG_SERIOUS, "can not open /dev/rts-ptz, please check");
		ret = -1;
		goto err;
	}

	//reset
	//need time, about 48s
	ioctl(fd, RTS_PTZ_IOC_RESET, NULL);
	sleep(3);

	motor_status = MOTOR_READY;

	return ret;
err:
	if(fd)
		close(fd);

	return ret;
}

int motor_auto_move_stop()
{
//	if(motor_status != MOTOR_AUTO_MOVE)
//	{
//		log_qcy(DEBUG_SERIOUS, "motor not in auto move status");
//		return -1;
//	}
//
//	motor_status = MOTOR_READY;
//	return 0;
	log_qcy(DEBUG_SERIOUS, "motor_auto_move_stop");
	return 0;
}

int motor_auto_move()
{
//	if(motor_status != MOTOR_READY)
//	{
//		log_qcy(DEBUG_SERIOUS, "motor not ready");
//		return -1;
//	}
//
//	motor_status = MOTOR_AUTO_MOVE;
//	return 0;
	log_qcy(DEBUG_SERIOUS, "motor_auto_move");
	return 0;
}

int control_motor(int x_y, int dir, device_config_t config_t)
{
	if(motor_status != MOTOR_READY)
	{
		log_qcy(DEBUG_SERIOUS, "motor not ready");
		return -1;
	}

	motor_status = MOTOR_CTRL;

	if(x_y == MOTOR_X)
	{
		ptz_info.xmotor_info.steps = config_t.motor_step;
		ptz_info.xmotor_info.dir = dir;
		ptz_info.xmotor_info.speed = config_t.motor_speed;

		ptz_info.ymotor_info.dir = DIR_NONE;
		ptz_info.ymotor_info.speed = config_t.motor_speed;
		ptz_info.ymotor_info.steps = config_t.motor_step;

	} else if (x_y == MOTOR_Y) {

		ptz_info.xmotor_info.dir = DIR_NONE;
		ptz_info.xmotor_info.speed = config_t.motor_speed;
		ptz_info.xmotor_info.steps = config_t.motor_step;

		ptz_info.ymotor_info.dir = dir;
		ptz_info.ymotor_info.speed = config_t.motor_speed;
		ptz_info.ymotor_info.steps = config_t.motor_step;

	} else if (x_y == MOTOR_BOTH) {

		switch(dir)
		{
			case DIR_LEFT_UP :
				ptz_info.xmotor_info.dir = DIR_LEFT;
				ptz_info.ymotor_info.dir = DIR_UP;
				break;
			case DIR_LEFT_DOWN:
				ptz_info.xmotor_info.dir = DIR_LEFT;
				ptz_info.ymotor_info.dir = DIR_DOWN;
				break;
			case DIR_RIGHT_UP:
				ptz_info.xmotor_info.dir = DIR_RIGHT;
				ptz_info.ymotor_info.dir = DIR_UP;
				break;
			case DIR_RIGHT_DOWN:
				ptz_info.xmotor_info.dir = DIR_RIGHT;
				ptz_info.ymotor_info.dir = DIR_DOWN;
				break;
			default:
				log_qcy(DEBUG_SERIOUS, "not support dir");
				ptz_info.xmotor_info.dir = DIR_NONE;
				ptz_info.ymotor_info.dir = DIR_NONE;
				break;
		}

		ptz_info.xmotor_info.speed = config_t.motor_speed;
		ptz_info.xmotor_info.steps = config_t.motor_step;

		ptz_info.ymotor_info.speed = config_t.motor_speed;
		ptz_info.ymotor_info.steps = config_t.motor_step;

	}

	ioctl(fd, RTS_PTZ_IOC_DRIVE, &ptz_info);

	motor_status = MOTOR_READY;
	return 0;
}

void motor_release()
{
	motor_status = MOTOR_NONE;

	if(fd)
		close(fd);
}

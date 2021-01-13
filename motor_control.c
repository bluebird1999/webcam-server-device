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

int check_motor_res_status()
{
	if(motor_status != MOTOR_READY)
	{
		log_qcy(DEBUG_SERIOUS, "motor not ready");
		return -1;
	}

	ioctl(fd, RTS_PTZ_IOC_G_INFO, &ptz_info);

	log_qcy(DEBUG_SERIOUS, "x is %d, y is %d\n", ptz_info.xmotor_info.pos, ptz_info.ymotor_info.pos);
	if((ptz_info.xmotor_info.pos == ptz_info.xmotor_info.max_steps / 2) && (ptz_info.ymotor_info.pos == ptz_info.ymotor_info.max_steps / 2))
		return 0;
	else
		return -1;

}

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

int init_motor(device_config_t config_t)
{
	int ret = 0;

	memset(&ptz_info, 0, sizeof(ptzctrl_info_t));

	//enable 595
	ctl_motor595_enable(&config_t ,1);

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
	if(motor_status != MOTOR_AUTO_MOVE)
	{
		log_qcy(DEBUG_SERIOUS, "motor not in auto move status");
		return -1;
	}

	ioctl(fd, RTS_PTZ_IOC_STOP, NULL);

	motor_status = MOTOR_READY;

	log_qcy(DEBUG_SERIOUS, "-- motor_auto_move_stop");
	return 0;
}

int motor_auto_roate(unsigned int x_step, int x_dir, unsigned int y_step, int y_dir, device_config_t config_t)
{
	if(motor_status != MOTOR_READY)
	{
		log_qcy(DEBUG_SERIOUS, "motor not ready");
		return -1;
	}

	motor_status = MOTOR_CTRL;

	memset(&ptz_info, 0, sizeof(ptzctrl_info_t));

	ptz_info.xmotor_info.steps = x_step;
	ptz_info.ymotor_info.steps = y_step;
	ptz_info.xmotor_info.dir = x_dir;
	ptz_info.ymotor_info.dir = y_dir;
	ptz_info.xmotor_info.speed = config_t.motor_speed;
	ptz_info.ymotor_info.speed = config_t.motor_speed;

	ioctl(fd, RTS_PTZ_IOC_DRIVE, &ptz_info);

	log_qcy(DEBUG_SERIOUS, "-- motor_auto_roate");
	motor_status = MOTOR_READY;
	return 0;
}

int motor_auto_move(int dir, device_config_t config_t)
{
	if(motor_status != MOTOR_READY)
	{
		log_qcy(DEBUG_SERIOUS, "motor not ready");
		return -1;
	}

	memset(&ptz_info, 0, sizeof(ptzctrl_info_t));

	switch(dir)
	{
		case DIR_AUTO_LEFT :
			ptz_info.xmotor_info.dir = DIR_LEFT;
			break;
		case DIR_AUTO_RIGHT:
			ptz_info.xmotor_info.dir = DIR_RIGHT;
			break;
		case DIR_AUTO_UP:
			ptz_info.ymotor_info.dir = DIR_UP;
			break;
		case DIR_AUTO_DOWN:
			ptz_info.ymotor_info.dir = DIR_DOWN;
			break;
		default:
			log_qcy(DEBUG_SERIOUS, "not support dir");
			ptz_info.xmotor_info.dir = DIR_NONE;
			ptz_info.ymotor_info.dir = DIR_NONE;
			break;
	}

	ptz_info.xmotor_info.speed = config_t.motor_speed;
	ptz_info.ymotor_info.speed = config_t.motor_speed;

	ioctl(fd, RTS_PTZ_IOC_RUN, &ptz_info);

	motor_status = MOTOR_AUTO_MOVE;
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

	memset(&ptz_info, 0, sizeof(ptzctrl_info_t));

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

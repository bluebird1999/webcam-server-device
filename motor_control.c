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

static int fd = 0;
static int x_cur_step = 0;
static int y_cur_step = 0;
static ptzctrl_info_t ptz_info;
static int motor_status = MOTOR_NONE;
static int motor_auto_cur_dir_x = DIR_LEFT;
static int motor_auto_cur_dir_y = DIR_DOWN;

int motor_reset()
{
	int ret = 0;

	if(motor_status != MOTOR_READY)
	{
		log_err("motor not ready");
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
		log_err("can not open /dev/rts-ptz, please check");
		ret = -1;
		goto err;
	}

	//reset
	//need time, about 48s
	ioctl(fd, RTS_PTZ_IOC_RESET, NULL);
	//sleep(15);

	ioctl(fd, RTS_PTZ_IOC_G_INFO, &ptz_info);

	if(ptz_info.xmotor_info.max_degrees == 0 || ptz_info.xmotor_info.max_steps == 0)
	{
		log_err("get x-motor info failed");
		ret = -1;
		goto err;
	}

	if(ptz_info.ymotor_info.max_degrees == 0 || ptz_info.ymotor_info.max_steps == 0)
	{
		log_err("get y-motor info failed");
		ret = -1;
		goto err;
	}

	x_cur_step = ptz_info.xmotor_info.pos;
	y_cur_step = ptz_info.ymotor_info.pos;
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
//		log_err("motor not in auto move status");
//		return -1;
//	}
//
//	motor_status = MOTOR_READY;
//	return 0;
	log_err("motor_auto_move_stop");
	return 0;
}

int motor_auto_move()
{
//	if(motor_status != MOTOR_READY)
//	{
//		log_err("motor not ready");
//		return -1;
//	}
//
//	motor_status = MOTOR_AUTO_MOVE;
//	return 0;
	log_err("motor_auto_move");
	return 0;
}

int control_motor(int x_y, int dir, int speed)
{
	if(motor_status != MOTOR_READY)
	{
		log_err("motor not ready");
		return -1;
	}

	motor_status = MOTOR_CTRL;

	if(x_y == MOTOR_X)
	{
		if(x_cur_step + MOTOR_STEP >= ptz_info.xmotor_info.max_steps ||
				x_cur_step - MOTOR_STEP <= 0)
		{
			log_info("x-motor has reached the end");
			return -1;
		}

		ptz_info.xmotor_info.dir = dir;
		ptz_info.xmotor_info.speed = speed;
		ptz_info.xmotor_info.steps = MOTOR_STEP;

		ptz_info.ymotor_info.dir = DIR_NONE;
		ptz_info.ymotor_info.speed = speed;
		ptz_info.ymotor_info.steps = MOTOR_STEP;

		if(dir == DIR_RIGHT)
			x_cur_step += MOTOR_STEP;
		else if (dir == DIR_LEFT)
			x_cur_step -= MOTOR_STEP;
	} else if (x_y == MOTOR_Y) {

		if(y_cur_step + MOTOR_STEP >= ptz_info.ymotor_info.max_steps ||
				y_cur_step - MOTOR_STEP <= 0)
		{
			log_info("y-motor has reached the end");
			return -1;
		}

		ptz_info.xmotor_info.dir = DIR_NONE;
		ptz_info.xmotor_info.speed = speed;
		ptz_info.xmotor_info.steps = MOTOR_STEP;

		ptz_info.ymotor_info.dir = dir;
		ptz_info.ymotor_info.speed = speed;
		ptz_info.ymotor_info.steps = MOTOR_STEP;

		if(dir == DIR_UP)
			y_cur_step += MOTOR_STEP;
		else if (dir == DIR_DOWN)
			y_cur_step -= MOTOR_STEP;
	} else if (x_y == MOTOR_BOTH) {

		switch(dir)
		{
			case DIR_LEFT_UP :
				ptz_info.xmotor_info.dir = DIR_LEFT;
				ptz_info.ymotor_info.dir = DIR_UP;
				x_cur_step -= MOTOR_STEP;
				y_cur_step += MOTOR_STEP;
				break;
			case DIR_LEFT_DOWN:
				ptz_info.xmotor_info.dir = DIR_LEFT;
				ptz_info.ymotor_info.dir = DIR_DOWN;
				x_cur_step -= MOTOR_STEP;
				y_cur_step -= MOTOR_STEP;
				break;
			case DIR_RIGHT_UP:
				ptz_info.xmotor_info.dir = DIR_RIGHT;
				ptz_info.ymotor_info.dir = DIR_UP;
				x_cur_step += MOTOR_STEP;
				y_cur_step += MOTOR_STEP;
				break;
			case DIR_RIGHT_DOWN:
				ptz_info.xmotor_info.dir = DIR_RIGHT;
				ptz_info.ymotor_info.dir = DIR_DOWN;
				x_cur_step += MOTOR_STEP;
				y_cur_step -= MOTOR_STEP;
				break;
			default:
				log_err("not support dir");
				ptz_info.xmotor_info.dir = DIR_NONE;
				ptz_info.ymotor_info.dir = DIR_NONE;
				break;
		}

		if(x_cur_step + MOTOR_STEP >= ptz_info.xmotor_info.max_steps || x_cur_step - MOTOR_STEP <= 0)
			ptz_info.xmotor_info.dir = DIR_NONE;

		if(y_cur_step + MOTOR_STEP >= ptz_info.ymotor_info.max_steps || y_cur_step - MOTOR_STEP <= 0)
			ptz_info.ymotor_info.dir = DIR_NONE;

		if((x_cur_step + MOTOR_STEP >= ptz_info.xmotor_info.max_steps || x_cur_step - MOTOR_STEP <= 0) &&
				(y_cur_step + MOTOR_STEP >= ptz_info.ymotor_info.max_steps || y_cur_step - MOTOR_STEP <= 0))
		{
			log_info("x-motor and y-motor has reached the end");
			return -1;
		}

		ptz_info.xmotor_info.speed = speed;
		ptz_info.xmotor_info.steps = MOTOR_STEP;

		ptz_info.ymotor_info.speed = speed;
		ptz_info.ymotor_info.steps = MOTOR_STEP;

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

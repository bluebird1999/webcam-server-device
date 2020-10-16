/*
 * motor_control.h
 *
 *  Created on: Sep 24, 2020
 *      Author: lijunxin
 */

#ifndef MOTOR_CONTRIL_H_
#define MOTOR_CONTRIL_H_

/*
 * header
 */

/*
 * define
 */
#define MOTOR_X			1
#define MOTOR_Y			2

#define MOTOR_STEP		300

#define DIR_NONE		0
//for MOTOR_Y
#define DIR_UP			1
#define DIR_DOWN		-1
//for MOTOR_X
#define DIR_RIGHT		1
#define DIR_LEFT		-1

#define SPEED_NORMAL	2
#define SPEED_LOW		4
#define SPEED_HIGH		1

#define RTS_PTZ_IOC_MAGIC		'm'
#define RTS_PTZ_IOC_DRIVE		_IOW(RTS_PTZ_IOC_MAGIC, 1, struct ptzctrl_info)
#define RTS_PTZ_IOC_RUN			_IOW(RTS_PTZ_IOC_MAGIC, 2, struct ptzctrl_info)
#define RTS_PTZ_IOC_STOP		_IO(RTS_PTZ_IOC_MAGIC, 3)
#define RTS_PTZ_IOC_RESET		_IO(RTS_PTZ_IOC_MAGIC, 4)
#define RTS_PTZ_IOC_G_INFO		_IOR(RTS_PTZ_IOC_MAGIC, 5, struct ptzctrl_info)
#define RTS_PTZ_IOC_G_POS		_IOR(RTS_PTZ_IOC_MAGIC, 6, struct ptzctrl_info)
#define RTS_PTZ_IOC_S_POS		_IOW(RTS_PTZ_IOC_MAGIC, 7, struct ptzctrl_info)
#define RTS_PTZ_IOC_IS_RUNNING		_IOR(RTS_PTZ_IOC_MAGIC, 8, struct ptzctrl_info)

/*
 * structure
 */
typedef struct motor_info {
	int dir;
	unsigned int speed;
	unsigned int steps;
	unsigned int pos;
	unsigned int is_running;

	unsigned int max_steps;
	unsigned int max_degrees;
}motor_info_t;

typedef struct ptzctrl_info {
	motor_info_t xmotor_info;
	motor_info_t ymotor_info;
}ptzctrl_info_t;

/*
 * function
 */
int init_motor();
int control_motor(int x_y, int dir, int speed);
void motor_release();

#endif /* MOTOR_CONTRIL_H_ */

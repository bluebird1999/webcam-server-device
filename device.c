/*
 * device.c
 *
 *  Created on: Sep 23, 2020
 *      Author: lijunxin
 */

/*
 * header
 */
//system header
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <rtscamkit.h>
#include <rtsavapi.h>
#include <rtsvideo.h>
//program header
#include "../../tools/tools_interface.h"
#include "../../manager/manager_interface.h"
#include "../../server/audio/audio_interface.h"
#include "../../server/video/video_interface.h"
#include "../../server/miio/miio_interface.h"
#include "../../server/miss/miss_interface.h"
//server header
#include "device.h"
#include "device_interface.h"
#include "sd_control_interface.h"
#include "audio_control_interface.h"
#include "part_control_interface.h"
#include "gpio_control_interface.h"
#include "motor_control_interface.h"
/*
 * static
 */
//variable
static server_info_t 		info;
static message_buffer_t		message;

//function
//common
static void *server_func(void);
static int server_message_proc(void);
static int server_none(void);
static int server_wait(void);
static int server_setup(void);
static int server_idle(void);
static int server_start(void);
static int server_run(void);
static int server_stop(void);
static int server_restart(void);
static int server_error(void);
static int server_release(void);
static int server_get_status(int type);
static int server_set_status(int type, int st);
static void server_thread_termination(void);
static int send_iot_ack(message_t *org_msg, message_t *msg, int id, int receiver, int result, void *arg, int size);
static int send_message(int receiver, message_t *msg);

//specific
static int iot_get_sd_info(device_iot_config_t *tmp);
static int iot_get_part_info(device_iot_config_t *tmp);
static int iot_adjust_volume(void* arg);
static int iot_ctrl_led(void* arg);
static int iot_ctrl_amplifier(void* arg);
static int iot_ctrl_ircut(void* arg);
static int iot_ctrl_irled(void* arg);
static int iot_ctrl_motor(int x_y, int dir);
static int iot_ctrl_motor_reset();
/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */
static int iot_ctrl_motor_reset()
{
	int ret;

	ret = motor_reset();

	return ret;
}

static int iot_ctrl_motor(int x_y, int dir)
{
	int ret;

	ret = control_motor(x_y, dir, SPEED_NORMAL);

	return ret;
}

static int iot_ctrl_irled(void* arg)
{
	device_iot_config_t *tmp = NULL;
	int ret;

	if(arg == NULL)
		return -1;

	tmp = (device_iot_config_t *)arg;

	ret = ctl_irled(tmp->ir_mode);

	return ret;
}

static int iot_ctrl_ircut(void* arg)
{
	device_iot_config_t *tmp = NULL;
	int ret;

	if(arg == NULL)
		return -1;

	tmp = (device_iot_config_t *)arg;

	ret = ctl_ircut(tmp->ircut_onoff);

	return ret;
}

static int iot_ctrl_amplifier(void* arg)
{
	device_iot_config_t *tmp = NULL;
	int ret;

	if(arg == NULL)
		return -1;

	tmp = (device_iot_config_t *)arg;

	ret = ctl_spk(&tmp->amp_on_off);

	return ret;
}

static int iot_ctrl_led(void* arg)
{
	device_iot_config_t *tmp = NULL;
	int ret;

	if(arg == NULL)
		return -1;

	tmp = (device_iot_config_t *)arg;

	if(tmp->led1_onoff == 1)
		ret = set_blue_led_on();
	else if (tmp->led1_onoff == 0)
		ret = set_blue_led_off();

	if(tmp->led2_onoff == 1)
		ret = set_orange_led_on();
	else if (tmp->led2_onoff == 0)
		ret = set_orange_led_off();

	return ret;
}

static int iot_adjust_volume(void* arg)
{
	device_iot_config_t *tmp = NULL;
	audio_info_t_m ctrl_audio;
	int ret;

	if(arg == NULL)
		return -1;

	tmp = (device_iot_config_t *)arg;

	//check the parameters
	if(tmp->audio_iot_info.in_out != 1 || tmp->audio_iot_info.in_out != 0)
	{
		ret = -1;
		goto err;
	}
	if(tmp->audio_iot_info.type > 4 || tmp->audio_iot_info.type < 0)
	{
		ret = -1;
		goto err;
	}
	if(tmp->audio_iot_info.volume < -1 || tmp->audio_iot_info.volume > 100)
	{
		ret = -1;
		goto err;
	}

	memset(&ctrl_audio, 0, sizeof(ctrl_audio));
	ctrl_audio.type = tmp->audio_iot_info.type;
	ctrl_audio.volume = tmp->audio_iot_info.volume;

	if(tmp->audio_iot_info.in_out == VOLUME_SPEAKER)
		ret = adjust_audio_volume(&ctrl_audio);
	else if(tmp->audio_iot_info.in_out == VOLUME_MIC)
		ret = adjust_input_audio_volume(&ctrl_audio);

	return ret;
err:
	log_err("invalid parameters\n");
	return ret;
}

static int iot_get_part_user_info(device_iot_config_t *tmp)
{
	void *para = NULL;
	sd_info_ack_t *info = NULL;
	int ret;

	if(tmp == NULL)
		return -1;

	memset(tmp,0,sizeof(device_iot_config_t));
	ret = get_user_info(&para);
	if(!ret)
	{
		info = (sd_info_ack_t *)para;
		tmp->user_part_iot_info = *info;
	}

	if(para != NULL)
		free(para);

	return ret;
}

static int iot_get_part_info(device_iot_config_t *tmp)
{
	void *para = NULL;
	part_msg_info_t *info = NULL;
	int ret;

	if(tmp == NULL)
		return -1;

	memset(tmp,0,sizeof(device_iot_config_t));
	ret = get_part_info(&para);
	if(!ret)
	{
		info = (part_msg_info_t *)para;
		tmp->part_iot_info = *info;
	}

	if(para != NULL)
		free(para);

	return ret;
}

static int iot_get_sd_info(device_iot_config_t *tmp)
{
	void *para = NULL;
	sd_info_ack_t *info = NULL;
	int ret;

	if(tmp == NULL)
		return -1;
	memset(tmp,0,sizeof(device_iot_config_t));
	ret = get_sd_info(&para);
	if(!ret)
	{
		info = (sd_info_ack_t *)para;
		tmp->sd_iot_info.plug = info->plug;
		tmp->sd_iot_info.usedBytes = info->usedBytes;
		tmp->sd_iot_info.totalBytes = info->totalBytes;
		tmp->sd_iot_info.freeBytes = info->freeBytes;
	}

	if(para != NULL)
		free(para);

	return ret;
}

static int send_iot_ack(message_t *org_msg, message_t *msg, int id, int receiver, int result, void *arg, int size)
{
	int ret = 0;
    /********message body********/
	msg_init(msg);
	memcpy(&(msg->arg_pass), &(org_msg->arg_pass),sizeof(message_arg_t));
	msg->message = id | 0x1000;
	msg->sender = msg->receiver = SERVER_DEVICE;
	msg->result = result;
	msg->arg = arg;
	msg->arg_size = size;
	ret = send_message(receiver, msg);
	/***************************/
	return ret;
}

static int send_message(int receiver, message_t *msg)
{
	int st;
	switch(receiver) {
	case SERVER_CONFIG:
//		st = server_config_message(msg);
		break;
	case SERVER_DEVICE:
		break;
	case SERVER_KERNEL:
		break;
	case SERVER_REALTEK:
		break;
	case SERVER_MIIO:
		st = server_miio_message(msg);
		break;
	case SERVER_MISS:
		st = server_miss_message(msg);
		break;
	case SERVER_MICLOUD:
		break;
	case SERVER_AUDIO:
		st = server_audio_message(msg);
		break;
	case SERVER_RECORDER:
		break;
	case SERVER_PLAYER:
		break;
	case SERVER_MANAGER:
		st = manager_message(msg);
		break;
	}
	return st;
}

/*
 * helper
 */
static void server_thread_termination(void)
{
	message_t msg;
	memset(&msg, 0, sizeof(message_t));
	msg.message = MSG_DEVICE_SIGINT;

	uninit_led_gpio();
	motor_release();

	manager_message(&msg);
}

static int server_release(void)
{
	return 0;
}

static int server_set_status(int type, int st)
{
	int ret=-1;
	ret = pthread_rwlock_wrlock(&info.lock);
	if(ret)	{
		log_err("add lock fail, ret = %d", ret);
		return ret;
	}
	if(type == STATUS_TYPE_STATUS)
		info.status = st;
	else if(type==STATUS_TYPE_EXIT)
		info.exit = st;
	ret = pthread_rwlock_unlock(&info.lock);
	if (ret)
		log_err("add unlock fail, ret = %d", ret);
	return ret;
}

static int server_get_status(int type)
{
	int st;
	int ret;
	ret = pthread_rwlock_wrlock(&info.lock);
	if(ret)	{
		log_err("add lock fail, ret = %d", ret);
		return ret;
	}
	if(type == STATUS_TYPE_STATUS)
		st = info.status;
	else if(type== STATUS_TYPE_EXIT)
		st = info.exit;
	ret = pthread_rwlock_unlock(&info.lock);
	if (ret)
		log_err("add unlock fail, ret = %d", ret);
	return st;
}
//-------------------------------------------需要修改
static int server_message_proc(void)
{
	int ret = 0, ret1 = 0;
	message_t msg;
	message_t send_msg;
	device_iot_config_t tmp;
	msg_init(&msg);
	msg_init(&send_msg);
	ret = pthread_rwlock_wrlock(&message.lock);
	if(ret)	{
		log_err("add message lock fail, ret = %d\n", ret);
		return ret;
	}
	ret = msg_buffer_pop(&message, &msg);
	ret1 = pthread_rwlock_unlock(&message.lock);
	if (ret1)
		log_err("add message unlock fail, ret = %d\n", ret1);
	if( ret == -1)
		return -1;
	else if( ret == 1)
		return 0;
	switch(msg.message){
	case MSG_MANAGER_EXIT:
		server_set_status(STATUS_TYPE_EXIT,1);
		break;
	case MSG_MANAGER_TIMER_ACK:
		((HANDLER)msg.arg)();
		break;
	case MSG_DEVICE_GET_PARA:
		if( msg.arg_in.cat == DEVICE_CTRL_SD_INFO ) {
			ret = iot_get_sd_info(&tmp);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_GET_PARA_ACK, msg.receiver, ret,
					&tmp, sizeof(device_iot_config_t));
		} else if( msg.arg_in.cat == DEVICE_CTRL_PART_INFO) {
			ret = iot_get_part_info(&tmp);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_GET_PARA_ACK, msg.receiver, ret,
					&tmp, sizeof(device_iot_config_t));
		} else if( msg.arg_in.cat == DEVICE_CTRL_PART_USER_INFO) {
			ret = iot_get_part_user_info(&tmp);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_GET_PARA_ACK, msg.receiver, ret,
					&tmp, sizeof(device_iot_config_t));
		}
		break;
	case MSG_DEVICE_ACTION:
		if( msg.arg_in.cat == DEVICE_ACTION_SD_FORMAT) {
			ret = format_sd();
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_ACTION_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_ACTION_USER_FORMAT ) {
			ret = format_userdata();
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_ACTION_ACK, msg.receiver, ret,
					NULL, 0);
		}
		break;
	case MSG_DEVICE_CTRL_DIRECT:
		if( msg.arg_in.cat == DEVICE_CTRL_AMPLIFIER ) {
			ret = iot_ctrl_amplifier(msg.arg);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_ADJUST_AUDIO_VOLUME ) {
			ret = iot_adjust_volume(msg.arg);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_LED ) {
			ret = iot_ctrl_led(msg.arg);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_IR_SWITCH ) {
			ret = iot_ctrl_ircut(msg.arg);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_IR_MODE ) {
			ret = iot_ctrl_irled(msg.arg);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_HOR_LEFT ) {
			ret = iot_ctrl_motor(MOTOR_X, DIR_LEFT);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_HOR_RIGHT ) {
			ret = iot_ctrl_motor(MOTOR_X, DIR_RIGHT);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_VER_DOWN ) {
			ret = iot_ctrl_motor(MOTOR_Y, DIR_DOWN);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_VER_UP ) {
			ret = iot_ctrl_motor(MOTOR_Y, DIR_UP);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_RESET ) {
			ret = iot_ctrl_motor_reset();
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		}
		break;
	}
	msg_free(&msg);
	return ret;
}

/*
 * state machine
 */
static int server_none(void)
{
	int ret = 0;
	server_set_status(STATUS_TYPE_STATUS, STATUS_WAIT);
	return ret;
}

static int server_wait(void)
{
	int ret = 0;
	server_set_status(STATUS_TYPE_STATUS, STATUS_SETUP);
	return ret;
}

static int server_setup(void)
{
	int ret = 0;
	rts_set_log_mask(RTS_LOG_MASK_CONS);
	ret = init_part_info();
	if(ret)
	{
		log_err("init_part_info init failed");
		ret = -1;
		goto err;
	}

	ret = init_led_gpio();
	if(ret)
	{
		log_err("init_led_gpio init failed");
		ret = -1;
		goto err;
	}

	ret = init_motor();
	if(ret)
	{
		log_err("init_motor init failed");
		ret = -1;
		goto err;
	}

	server_set_status(STATUS_TYPE_STATUS, STATUS_IDLE);
	return ret;

err:
	server_set_status(STATUS_TYPE_STATUS, STATUS_ERROR);
	return ret;
}

static int server_idle(void)
{
	int ret = 0;
	server_set_status(STATUS_TYPE_STATUS, STATUS_START);
	return ret;
}

static int server_start(void)
{
	int ret = 0;
	server_set_status(STATUS_TYPE_STATUS, STATUS_RUN);
	return ret;
}

static int server_run(void)
{
	int ret = 0;
	if( server_message_proc()!= 0)
		log_err("error in message proc");
	return ret;
}

static int server_stop(void)
{
	int ret = 0;
	uninit_led_gpio();
	return ret;
}

static int server_restart(void)
{
	int ret = 0;
	uninit_led_gpio();
	motor_release();
	return ret;
}

static int server_error(void)
{
	int ret = 0;
	uninit_led_gpio();
	server_release();
	motor_release();
	return ret;
}

static void *server_func(void)
{
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
	misc_set_thread_name("server_device");
	pthread_detach(pthread_self());
	while( !info.exit ) {
	switch(info.status){
		case STATUS_NONE:
			server_none();
			break;
		case STATUS_WAIT:
			server_wait();
			break;
		case STATUS_SETUP:
			server_setup();
			break;
		case STATUS_IDLE:
			server_idle();
			break;
		case STATUS_START:
			server_start();
			break;
		case STATUS_RUN:
			server_run();
			break;
		case STATUS_STOP:
			server_stop();
			break;
		case STATUS_RESTART:
			server_restart();
			break;
		case STATUS_ERROR:
			server_error();
			break;
		}
//		usleep(100);//100ms
	}
	server_release();
	log_info("-----------thread exit: server_device-----------");
	message_t msg;
    /********message body********/
	msg_init(&msg);
	msg.message = MSG_MANAGER_EXIT_ACK;
	msg.sender = SERVER_DEVICE;
	/****************************/
	manager_message(&msg);
	pthread_exit(0);
}

/*
 * external interface
 */
int server_device_start(void)
{
	int ret=-1;
	if( message.init == 0) {
		msg_buffer_init(&message, MSG_BUFFER_OVERFLOW_NO);
	}
	pthread_rwlock_init(&info.lock, NULL);
	ret = pthread_create(&info.id, NULL, server_func, NULL);
	if(ret != 0) {
		log_err("device server create error! ret = %d",ret);
		 return ret;
	 }
	else {
		log_err("device server create successful!");
		return 0;
	}
}

int server_device_message(message_t *msg)
{
	int ret=0;
	if( server_get_status(STATUS_TYPE_STATUS)!= STATUS_RUN ) {
		log_err("device server is not ready!");
		return -1;
	}
	ret = pthread_rwlock_wrlock(&message.lock);
	if(ret)	{
		log_err("add message lock fail, ret = %d\n", ret);
		return ret;
	}
	ret = msg_buffer_push(&message, msg);
	if( ret!=0 )
		log_err("message push in device error =%d", ret);
	ret = pthread_rwlock_unlock(&message.lock);
	if (ret)
		log_err("add message unlock fail, ret = %d\n", ret);
	return ret;
}

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
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <linux/netlink.h>
#include <rtscamkit.h>
#include <rtsavapi.h>
#include <rtsvideo.h>
#include <rts_io_adc.h>
#include <linux/input.h>
//#include <dmalloc.h>
//program header
#include "../../tools/tools_interface.h"
#include "../../manager/manager_interface.h"
#include "../../server/audio/audio_interface.h"
#include "../../server/video/video_interface.h"
#include "../../server/miio/miio_interface.h"
#include "../../server/miss/miss_interface.h"
#include "../../server/recorder/recorder_interface.h"
#include "../../server/speaker/speaker_interface.h"
//server header
#include "device.h"
#include "config.h"
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
static int					led1_sleep_t[3] = {1, 100000, 200000}; //us
static int					led2_sleep_t[3] = {2, 200000, 800000}; //us
static int 					daynight_mode_func_exit = 0;
static int 					led_flash_func_exit = 0;
static int 					storage_detect_func_lock = 0;
static int 					daynight_mode_func_lock = 0;
static int 					led_flash_func_lock = 0;
static int 					sd_card_insert;
static int 					motor_reset_thread_flag = 0;
static int					umount_server_flag = 0;
static int					umount_flag = 0;
static int					step_motor_init_flag = 0;
static device_config_t		device_config_;
static server_info_t 		info;
static message_buffer_t		message;
static message_t 			rev_msg_tmp;
static pthread_mutex_t		d_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t		d_cond = PTHREAD_COND_INITIALIZER;

static server_name_t server_name_tt[] = {
		{0, "config"},
		{1, "device"},
		{2, "kernel"},
		{3, "realtek"},
		{4, "miio"},
		{5, "miss"},
		{6, "micloud"},
		{7, "video"},
		{8, "audio"},
		{9, "recorder"},
		{10, "player"},
		{11, "speaker"},
		{12, "video2"},
		{13, "scanner"},
		{14, "video3"},
		{32, "manager"},
};
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
static int heart_beat_proc(void);
static int server_get_status(int type);
static int server_set_status(int type, int st);
static void server_thread_termination(void);
static int send_iot_ack(message_t *org_msg, message_t *msg, int id, int receiver, int result, void *arg, int size);
static int send_message(int receiver, message_t *msg);

//specific
static int iot_get_sd_info(device_iot_config_t *tmp);
static int iot_get_part_info(device_iot_config_t *tmp);
static int iot_get_part_user_info(device_iot_config_t *tmp);
static int iot_get_led_status(device_iot_config_t *tmp);
static int iot_adjust_volume(void* arg);
static int iot_ctrl_led(void* arg);
static int iot_ctrl_amplifier(void* arg);
static int iot_ctrl_day_night(void* arg);
static int iot_ctrl_motor_auto(int status);
static int iot_ctrl_motor(int x_y, int dir);
static int iot_ctrl_motor_reset();
static int iot_umount_sd(int umount_orformat);
static void *motor_init_func(void *arg);
static void *storage_detect_func(void *arg);
static void *daynight_mode_func(void *arg);
static void *led_flash_func(void *arg);
static void *motor_reset_func(void *arg);
static char *get_string_name(int i);
static int video_isp_set_attr(unsigned int id, int value);


#define BUTTON_DOWN 1
#define BUTTON_UP   0
#define WIFI_RESET_FILE_SH "/opt/qcy/bin/wifi_reset_factory.sh wifi_reset"

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */



static void *motor_reset_func(void *arg)
{
	int ret = 0;

    signal(SIGINT, (__sighandler_t)server_thread_termination);
    signal(SIGTERM, (__sighandler_t)server_thread_termination);
	misc_set_thread_name("motor_reset_thread");
    pthread_detach(pthread_self());

    motor_reset_thread_flag = 1;

	ret = motor_reset();
	if(ret)
		log_qcy(DEBUG_SERIOUS, "motor_reset not ready or failed");

	motor_reset_thread_flag = 0;
	pthread_exit(0);
}

static int iot_ctrl_motor_reset()
{
	int ret = 0;
	static pthread_t motor_reset_tid = 0;

	if(motor_reset_thread_flag == 0)
	{
		if (ret |= pthread_create(&motor_reset_tid, NULL, motor_reset_func, NULL)) {
			log_qcy(DEBUG_SERIOUS, "create motor_reset_func thread failed, ret = %d\n", ret);
			ret = -1;
		}
	} else {
		log_qcy(DEBUG_SERIOUS, "motor_reset_func is working now");
		ret = 0;
	}

	return ret;
}

static int iot_ctrl_motor(int x_y, int dir)
{
	int ret;

	ret = control_motor(x_y, dir, device_config_);

	return ret;
}

static int iot_ctrl_motor_auto(int status)
{
	int ret = 0;

	if(status == MOTOR_AUTO)
	{
		ret = motor_auto_move();
	} else if(status == MOTOR_STOP) {
		ret = motor_auto_move_stop();
	}

	return ret;
}

static int iot_ctrl_day_night(void* arg)
{
	int ret = 0;
	device_iot_config_t *tmp = NULL;
	static pthread_t day_night_mode_tid = 0;

	if(arg == NULL)
		return -1;

	tmp = (device_iot_config_t *)arg;

	if(tmp->day_night_mode == DAY_NIGHT_AUTO)
	{
		if(day_night_mode_tid == 0)
		{
			daynight_mode_func_exit = 0;
			if (ret |= pthread_create(&day_night_mode_tid, NULL, daynight_mode_func, NULL)) {
				log_qcy(DEBUG_SERIOUS, "create daynight_mode_func thread failed, ret = %d\n", ret);
				ret = -1;
			}
		} else {
			log_qcy(DEBUG_SERIOUS, "now is day_night automode");
			ret = 0;
		}
	}
	else if (tmp->day_night_mode == DAY_NIGHT_OFF)
	{

		if(day_night_mode_tid != 0)
		{
			//pthread_cancel(day_night_mode_tid);
			daynight_mode_func_exit = 1;
			day_night_mode_tid = 0;
		}
		while(!server_get_status(STATUS_TYPE_EXIT) && daynight_mode_func_exit);
		ret = ctl_ircut(GPIO_ON);
		ret |= ctl_irled(&device_config_, GPIO_OFF);
	}
	else if (tmp->day_night_mode == DAY_NIGHT_ON)
	{

		if(day_night_mode_tid != 0)
		{
			//pthread_cancel(day_night_mode_tid);
			daynight_mode_func_exit = 1;
			day_night_mode_tid = 0;
		}
		while(!server_get_status(STATUS_TYPE_EXIT) && daynight_mode_func_exit);
		ret = ctl_ircut(GPIO_OFF);
		ret |= ctl_irled(&device_config_, GPIO_ON);
	}

	return ret;
}

static int iot_umount_sd(int umount_orformat)
{
	int ret = 0;
	message_t send_msg;

	msg_init(&send_msg);
	//ret = umount_sd();

	send_msg.sender = send_msg.receiver = SERVER_DEVICE;
	send_msg.message = MSG_DEVICE_ACTION;
	send_msg.arg_in.cat = DEVICE_ACTION_SD_EJECTED;
	send_msg.arg_in.wolf = 1;

	if(umount_orformat)
		send_msg.arg_pass.wolf = 1;

	server_player_message(&send_msg);
	server_recorder_message(&send_msg);

	return ret;
}

static int iot_ctrl_amplifier(void* arg)
{
	device_iot_config_t *tmp = NULL;
	int ret;

	if(arg == NULL)
		return -1;

	tmp = (device_iot_config_t *)arg;

	ret = ctl_spk(&device_config_, &tmp->amp_on_off);

	return ret;
}

static int iot_ctrl_led(void* arg)
{
	int ret = 0;
	device_iot_config_t *tmp = NULL;
	static pthread_t led1_flash_tid = 0;
	static pthread_t led2_flash_tid = 0;

	if(arg == NULL)
		return -1;

	tmp = (device_iot_config_t *)arg;

	if(tmp->led1_onoff == LED_ON)
	{
		if(led1_flash_tid != 0)
		{
			led_flash_func_exit = 1;
			led1_flash_tid = 0;
		}
		while(!server_get_status(STATUS_TYPE_EXIT) && led_flash_func_exit);
		ret = set_blue_led_status(&device_config_, LED_ON);
	}
	else if (tmp->led1_onoff == LED_OFF)
	{
		if(led1_flash_tid != 0)
		{
			led_flash_func_exit = 1;
			led1_flash_tid = 0;
		}
		while(!server_get_status(STATUS_TYPE_EXIT) && led_flash_func_exit);
		ret = set_blue_led_status(&device_config_, LED_OFF);
	}
	else if (tmp->led1_onoff == LED_FLASH) 	//twinkle
	{
		//add
		if(led2_flash_tid != 0)
		{
			led_flash_func_exit = 1;
			led2_flash_tid = 0;
			while(!server_get_status(STATUS_TYPE_EXIT) && led_flash_func_exit);
		}
		led_flash_func_exit = 0;
		if (ret |= pthread_create(&led1_flash_tid, NULL, led_flash_func, led1_sleep_t)) {
			log_qcy(DEBUG_SERIOUS, "create led_flash_func thread failed, ret = %d\n", ret);
			ret = -1;
		}
	}

	if(tmp->led2_onoff == LED_ON)
	{
		if(led2_flash_tid != 0)
		{
			led_flash_func_exit = 1;
			led2_flash_tid = 0;
		}
		while(!server_get_status(STATUS_TYPE_EXIT) && led_flash_func_exit);
		ret = set_orange_led_status(&device_config_, LED_ON);
	}
	else if (tmp->led2_onoff == LED_OFF)
	{
		if(led2_flash_tid != 0)
		{
			led_flash_func_exit = 1;
			led2_flash_tid = 0;
		}
		while(!server_get_status(STATUS_TYPE_EXIT) && led_flash_func_exit);

		ret = set_orange_led_status(&device_config_, LED_OFF);
	}
	else if (tmp->led2_onoff == LED_FLASH) 	//twinkle
	{
		//add
		if(led1_flash_tid != 0)
		{
			led_flash_func_exit = 1;
			led1_flash_tid = 0;
			while(!server_get_status(STATUS_TYPE_EXIT) && led_flash_func_exit);
		}
		led_flash_func_exit = 0;
		if (ret |= pthread_create(&led2_flash_tid, NULL, led_flash_func, led2_sleep_t)) {
			log_qcy(DEBUG_SERIOUS, "create led_flash_func thread failed, ret = %d\n", ret);
			ret = -1;
		}
	}

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
	if(tmp->audio_iot_info.in_out != 1 && tmp->audio_iot_info.in_out != 0)
	{
		log_qcy(DEBUG_SERIOUS, "in_out parameters error");
		ret = -1;
		goto err;
	}
	if(tmp->audio_iot_info.type > 4 || tmp->audio_iot_info.type < 0)
	{
		log_qcy(DEBUG_SERIOUS, "type parameters error");
		ret = -1;
		goto err;
	}
	if(tmp->audio_iot_info.volume < -1 || tmp->audio_iot_info.volume > 100)
	{
		log_qcy(DEBUG_SERIOUS, "volume parameters error");
		ret = -1;
		goto err;
	}

	memset(&ctrl_audio, 0, sizeof(ctrl_audio));
	ctrl_audio.type = tmp->audio_iot_info.type;
	ctrl_audio.volume = tmp->audio_iot_info.volume;

	if(tmp->audio_iot_info.in_out == VOLUME_SPEAKER)
		ret = adjust_audio_volume(&ctrl_audio, device_config_);
	else if(tmp->audio_iot_info.in_out == VOLUME_MIC)
		ret = adjust_input_audio_volume(&ctrl_audio, device_config_);

	return ret;
err:
	log_qcy(DEBUG_SERIOUS, "invalid parameters\n");
	return ret;
}

static int iot_get_led_status(device_iot_config_t *tmp)
{
	int ret = 0;

	if(tmp == NULL)
		return -1;

	memset(tmp,0,sizeof(device_iot_config_t));

	ret = get_led_status(LED1);
	if(ret > 0)
		tmp->led1_onoff = ret;

	ret = get_led_status(LED2);
	if(ret > 0)
		tmp->led2_onoff = ret;

	return (ret == -1 ) ? -1 : 0;
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
	ret = get_sd_info(&para, device_config_);
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
		st = server_kernel_message(msg);
		break;
	case SERVER_REALTEK:
		st = server_realtek_message(msg);
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
		st = server_recorder_message(msg);
		break;
	case SERVER_SPEAKER:
		st = server_speaker_message(msg);
		break;
	case SERVER_PLAYER:
		st = server_player_message(msg);
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

	while(storage_detect_func_lock || led_flash_func_lock || daynight_mode_func_lock)
	{
//		log_qcy(DEBUG_SERIOUS, "server_release device ---- ");
		usleep(50 * 1000); //50ms
	}

	uninit_led_gpio();

	if(device_config_.motor_enable)
		motor_release();
	memset(&info,0,sizeof(server_info_t));
	return 0;
}

static int server_set_status(int type, int st)
{
	int ret=-1;
	ret = pthread_rwlock_wrlock(&info.lock);
	if(ret)	{
		log_qcy(DEBUG_SERIOUS, "add lock fail, ret = %d", ret);
		return ret;
	}
	if(type == STATUS_TYPE_STATUS)
		info.status = st;
	else if(type==STATUS_TYPE_EXIT)
		info.exit = st;
	ret = pthread_rwlock_unlock(&info.lock);
	if (ret)
		log_qcy(DEBUG_SERIOUS, "add unlock fail, ret = %d", ret);
	return ret;
}

static int server_get_status(int type)
{
	int st;
	int ret;
	ret = pthread_rwlock_wrlock(&info.lock);
	if(ret)	{
		log_qcy(DEBUG_SERIOUS, "add lock fail, ret = %d", ret);
		return ret;
	}
	if(type == STATUS_TYPE_STATUS)
		st = info.status;
	else if(type== STATUS_TYPE_EXIT)
		st = info.exit;
	ret = pthread_rwlock_unlock(&info.lock);
	if (ret)
		log_qcy(DEBUG_SERIOUS, "add unlock fail, ret = %d", ret);
	return st;
}
//-------------------------------------------需要修改
static int server_message_proc(void)
{
	int ret = 0, ret1 = 0;
	static int format_flag = 0;
	message_t msg;
	message_t send_msg;
	device_iot_config_t tmp;
	msg_init(&msg);
	msg_init(&send_msg);

	pthread_mutex_lock(&d_mutex);
	if( message.head == message.tail ) {
		if( info.status==STATUS_RUN ) {
			pthread_cond_wait(&d_cond,&d_mutex);
		}
	}
	pthread_mutex_unlock(&d_mutex);

	ret = pthread_rwlock_wrlock(&message.lock);
	if(ret)	{
		log_qcy(DEBUG_SERIOUS, "add message lock fail, ret = %d\n", ret);
		return ret;
	}
	ret = msg_buffer_pop(&message, &msg);
	ret1 = pthread_rwlock_unlock(&message.lock);
	if (ret1)
		log_qcy(DEBUG_SERIOUS, "add message unlock fail, ret = %d\n", ret1);
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
		} else if( msg.arg_in.cat == DEVICE_CTRL_LED_STATUS) {
			ret = iot_get_led_status(&tmp);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_GET_PARA_ACK, msg.receiver, ret,
					&tmp, sizeof(device_iot_config_t));
		}
		break;
	case MSG_DEVICE_ACTION:
		if( msg.arg_in.cat == DEVICE_ACTION_SD_FORMAT) {
			umount_server_flag = 0;
			ret = iot_umount_sd(1);
			msg_init(&rev_msg_tmp);
			msg_deep_copy(&rev_msg_tmp, &msg);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_ACTION_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_ACTION_USER_FORMAT ) {
			//ret = format_userdata();
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_ACTION_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_ACTION_SD_UMOUNT ) {

			if(!umount_flag)
			{
				umount_server_flag = 0;
				ret = iot_umount_sd(0);
				msg_init(&rev_msg_tmp);
				msg_deep_copy(&rev_msg_tmp, &msg);
			}
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_ACTION_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_ACTION_SD_EJECTED_ACK ) {

			misc_set_bit(&umount_server_flag, msg.receiver, 1);
			format_flag |= msg.arg_pass.wolf;
			if(umount_server_flag == 1536)
			{
				system("sync");
				system("sync");
				sleep(1);

				if(format_flag)
				{
					ret = format_sd();
				}else{
					ret = umount_sd();
					umount_flag = 1;
				}
//				send_iot_ack(&rev_msg_tmp, &send_msg, MSG_DEVICE_ACTION_ACK, rev_msg_tmp.receiver, ret,
//						NULL, 0);
				umount_server_flag = 0;
				format_flag = 0;
			}
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
		} else if( msg.arg_in.cat == DEVICE_CTRL_DAY_NIGHT_MODE ) {
			ret = iot_ctrl_day_night(msg.arg);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_HOR_LEFT ) {
			ret = iot_ctrl_motor(MOTOR_STEP_Y, DIR_LEFT);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_HOR_RIGHT ) {
			ret = iot_ctrl_motor(MOTOR_STEP_Y, DIR_RIGHT);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_VER_DOWN ) {
			ret = iot_ctrl_motor(MOTOR_STEP_X, DIR_DOWN);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_VER_UP ) {
			ret = iot_ctrl_motor(MOTOR_STEP_X, DIR_UP);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_RESET ) {
			ret = iot_ctrl_motor_reset();
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_LEFT_UP ) {
			ret = iot_ctrl_motor(MOTOR_BOTH, DIR_LEFT_UP);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_LEFT_DOWN ) {
			ret = iot_ctrl_motor(MOTOR_BOTH, DIR_LEFT_DOWN);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_RIGHT_UP ) {
			ret = iot_ctrl_motor(MOTOR_BOTH, DIR_RIGHT_UP);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_RIGHT_DOWN ) {
			ret = iot_ctrl_motor(MOTOR_BOTH, DIR_RIGHT_DOWN);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_AUTO ) {
			ret = iot_ctrl_motor_auto(MOTOR_AUTO);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		} else if( msg.arg_in.cat == DEVICE_CTRL_MOTOR_STOP ) {
			ret = iot_ctrl_motor_auto(MOTOR_STOP);
			send_iot_ack(&msg, &send_msg, MSG_DEVICE_CTRL_DIRECT_ACK, msg.receiver, ret,
					NULL, 0);
		}
		break;
	default:
		log_qcy(DEBUG_SERIOUS, "not support message");
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

static void *storage_detect_func(void *arg)
{
	int i;
	fd_set fds;
	int ret = 0;
	message_t msg;
	char *ptr = NULL;
	int event_fd = 0;
	server_status_t st;
	struct statfs statFS;
	int hotplug_sock = 0;
	struct sockaddr_nl snl;
	unsigned long freeBytes;
	char buf[SIZE1024] = {0};
	struct input_event event;
	struct timeval timeout={0,1};
	int key_down_flag = 0;

    signal(SIGINT, (__sighandler_t)server_thread_termination);
    signal(SIGTERM, (__sighandler_t)server_thread_termination);
	misc_set_thread_name("storage_detect_thread");
    pthread_detach(pthread_self());

    storage_detect_func_lock = 1;
    sd_card_insert = get_sd_plug_status();
    memset(&snl, 0, sizeof(struct sockaddr_nl));

    msg_init(&msg);
    msg.sender = msg.receiver = SERVER_DEVICE;

    snl.nl_family = AF_NETLINK;
	snl.nl_pid = getpid();
	snl.nl_groups = 1;
	hotplug_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (hotplug_sock == -1) {
		log_qcy(DEBUG_SERIOUS, "error getting socket");
		goto error;
	}

    ret = bind(hotplug_sock, (struct sockaddr *) &snl, sizeof(struct sockaddr_nl));
    if (ret < 0) {
    	log_qcy(DEBUG_SERIOUS, "system_hotplug_recelve bind failed");
        goto error;
    }

    if(device_config_.reset_key_detect_enable)
    {
	    event_fd = open("/dev/input/event0", O_RDONLY);
		if(event_fd == -1)
		{
			log_qcy(DEBUG_SERIOUS, "open error\n");
			goto error;
		}
    }


    while(!server_get_status(STATUS_TYPE_EXIT))
    {
		//exit logic
		st = server_get_status(STATUS_TYPE_STATUS);
    	if( st != STATUS_RUN ) {
			if ( st == STATUS_IDLE || st == STATUS_SETUP || st == STATUS_START)
				continue;
			else
				break;
		}

    	FD_ZERO(&fds);
    	FD_SET(hotplug_sock,&fds);
    	if(device_config_.reset_key_detect_enable)
    		FD_SET(event_fd,&fds);
    	timeout.tv_sec=3;
    	timeout.tv_usec=0;

        switch(select((hotplug_sock > event_fd ? hotplug_sock+1 : event_fd +1),&fds,NULL,NULL,&timeout))
        {
            case -1:
            	log_qcy(DEBUG_SERIOUS, "select error");
                goto error;
                break;
            case 0:
                if(sd_card_insert)
                {
					if(is_mounted(device_config_.sd_mount_path) == 1)
					{
						if (statfs(device_config_.sd_mount_path, &statFS) == -1){
							log_qcy(DEBUG_SERIOUS, "statfs failed for path->[%s]\n", device_config_.sd_mount_path);
							goto error;
						}
						freeBytes = (unsigned int)((long long)statFS.f_bfree * (long long)statFS.f_frsize / 1024);
						if(freeBytes / 1024 < device_config_.storage_detect_lim)
						{
							msg.message = MSG_DEVICE_ACTION;
							msg.arg_in.cat = DEVICE_ACTION_SD_CAP_ALARM;
							for(i=0;i<MAX_SERVER;i++) {
								if( misc_get_bit( device_config_.storage_detect_notify, i) ) {
									log_qcy(DEBUG_VERBOSE, "lim send to ->[%d]\n",i);
									send_message(i, &msg);
								}
							}
						}
					}
                }
                if(key_down_flag)
                {
                	key_down_flag = 0;
                	msg.message = MSG_SPEAKER_CTL_PLAY;
					msg.arg_in.cat = DEVICE_ACTION_SD_CAP_ALARM;
					msg.arg_in.cat = SPEAKER_CTL_RESET;
					send_message(SERVER_SPEAKER, &msg);
					sleep(5);
					system(WIFI_RESET_FILE_SH);
                }
                break;
            default:
                if(FD_ISSET(hotplug_sock,&fds))
                {
                    ret = recv(hotplug_sock, buf, SIZE1024 , 0);
                    if(ret > 0)
                    {
                    	if(sd_card_insert == 0)
                    	{
							ptr = strstr(buf, "add@/devices/platform/ocp/18300000.sdhc");
							if(ptr != NULL)
							{
								sd_card_insert = 1;
								umount_flag = 0;
								msg.message = MSG_DEVICE_ACTION;
								msg.arg_in.cat = DEVICE_ACTION_SD_INSERT;
								msg.arg_in.dog = SD_STATUS_PLUG;
								while(!is_mounted(device_config_.sd_mount_path))
									usleep(1000 * 500);

								for(i=0;i<MAX_SERVER;i++) {
									if( misc_get_bit( device_config_.storage_detect_notify, i) ) {
										log_qcy(DEBUG_VERBOSE, "insert send to ->[%d]\n",i);
										send_message(i, &msg);
									}
								}
							}
                    	}
                    	if(sd_card_insert == 1)
                    	{
							ptr = strstr(buf, "remove@/devices/platform/ocp/18300000.sdhc");
							if(ptr != NULL)
							{
								if(!umount_flag)
								{
									umount_server_flag = 0;
									msg.message = MSG_DEVICE_ACTION;
									msg.arg_in.cat = DEVICE_ACTION_SD_EJECTED;
									msg.arg_in.wolf = 1;
									for(i=0;i<MAX_SERVER;i++) {
										if( misc_get_bit( device_config_.storage_detect_notify, i) ) {
											log_qcy(DEBUG_VERBOSE, "remove send to ->[%d]\n",i);
											send_message(i, &msg);
										}
									}
									umount_flag = 1;
								}
								sd_card_insert = 0;

//								server_player_interrupt_routine(1);
//								server_recorder_interrupt_routine(1);
							}
                    	}
                    }
                }
                if(device_config_.reset_key_detect_enable && FD_ISSET(event_fd,&fds))
				{
                	ret = read(event_fd, &event, sizeof(event));
                	if(ret < 0)
                	{
                		log_qcy(DEBUG_SERIOUS, "input read error\n");
                		goto error;
                	}
                	if(event.code == KEY_WPS_BUTTON)
                	{
                		if(event.value == BUTTON_DOWN)
                		{
                			log_qcy(DEBUG_SERIOUS, "wps key down\n");
                			key_down_flag = 1;
                			//start_time = time_get_now_ms();
                		}
//                		else if(event.value == BUTTON_UP)
//                		{
//                			log_qcy(DEBUG_SERIOUS, "wps key up\n");
//                			end_time = time_get_now_ms();
//
//                			interval_time = end_time - start_time;
//                			if(interval_time > 3000 && interval_time < 10000)
//                			{
//                				msg.message = MSG_SPEAKER_CTL_PLAY;
//                				msg.arg_in.cat = DEVICE_ACTION_SD_CAP_ALARM;
//                				msg.arg_in.cat = SPEAKER_CTL_RESET;
//                				send_message(SERVER_SPEAKER, &msg);
//                				sleep(3);
//                				system(WIFI_RESET_FILE_SH);
//                			}
//                		}
                	}
				}
                break;
        }
    }

error:
	log_qcy(DEBUG_SERIOUS, "-----%s thread exit -----", "storage_detect_thread");
	storage_detect_func_lock = 0;
	if(hotplug_sock)
		close(hotplug_sock);
	if(event_fd)
		close(event_fd);
	hotplug_sock = -1;
	pthread_exit(0);
}

static void *motor_init_func(void *arg)
{
	int ret = 0;

    signal(SIGINT, (__sighandler_t)server_thread_termination);
    signal(SIGTERM, (__sighandler_t)server_thread_termination);
	misc_set_thread_name("motor_init_func");
    pthread_detach(pthread_self());

	ret = init_motor(device_config_);
	if(ret)
	{
		log_qcy(DEBUG_SERIOUS, "init_motor init failed");
	}

	step_motor_init_flag = 1;
	pthread_exit(0);
}

static int video_isp_set_attr(unsigned int id, int value)
{
	struct rts_video_control ctrl;
	int ret;
	ret = rts_av_get_isp_ctrl(id, &ctrl);
	if (ret) {
		log_qcy(DEBUG_SERIOUS, "get isp attr fail, ret = %d\n", ret);
		return ret;
	}
	//value = isp_get_valid_value(id, value, &ctrl);
/*	log_qcy(DEBUG_SERIOUS, "%s min = %d, max = %d, step = %d, default = %d, cur = %d\n",
			 ctrl.name, ctrl.minimum, ctrl.maximum,
			 ctrl.step, ctrl.default_value, ctrl.current_value);
*/
	ctrl.current_value = value;
	ret = rts_av_set_isp_ctrl(id, &ctrl);
	if (ret) {
		log_qcy(DEBUG_SERIOUS, "set isp attr fail, ret = %d\n", ret);
		return ret;
	}
	ret = rts_av_get_isp_ctrl(id, &ctrl);
	if (ret) {
		log_qcy(DEBUG_SERIOUS, "get isp attr fail, ret = %d\n", ret);
		return ret;
	}
/*
	log_qcy(DEBUG_SERIOUS, "%s min = %d, max = %d, step = %d, default = %d, cur = %d\n",
			 ctrl.name, ctrl.minimum, ctrl.maximum,
			 ctrl.step, ctrl.default_value, ctrl.current_value);
*/
	return 0;
}

static void *led_flash_func(void *arg)
{
	server_status_t st;

    signal(SIGINT, (__sighandler_t)server_thread_termination);
    signal(SIGTERM, (__sighandler_t)server_thread_termination);
	misc_set_thread_name("led_flash_func_thread");
    pthread_detach(pthread_self());

    led_flash_func_lock = 1;

    while(!server_get_status(STATUS_TYPE_EXIT) && !led_flash_func_exit)
    {
		//exit logic
		st = server_get_status(STATUS_TYPE_STATUS);
    	if( st != STATUS_RUN ) {
			if ( st == STATUS_IDLE || st == STATUS_SETUP || st == STATUS_START)
				continue;
			else
				break;
		}

    	if(*(int *)arg == 1)
    	{
    		set_blue_led_status(&device_config_, LED_ON);
    	}else{
    		set_orange_led_status(&device_config_, LED_ON);
    	}

    	usleep(*(int *)((int *)arg + 1));

    	if(*(int *)arg == 1)
    	{
    		set_blue_led_status(&device_config_, LED_OFF);
    	}else{
    		set_orange_led_status(&device_config_, LED_OFF);
    	}

    	usleep(*(int *)((int *)arg + 2));
    }

    led_flash_func_exit = 0;
    led_flash_func_lock = 0;

    log_qcy(DEBUG_SERIOUS, "-----%s thread exit -----", "led_flash_func_thread");
    pthread_exit(0);
}

static void *daynight_mode_func(void *arg)
{
	int ret = 0;
	int value = 0;
	int old_value = 0;
	int lim_value = 0;
	server_status_t st;

    signal(SIGINT, (__sighandler_t)server_thread_termination);
    signal(SIGTERM, (__sighandler_t)server_thread_termination);
	misc_set_thread_name("daynight_mode_thread");
    pthread_detach(pthread_self());

	if(device_config_.soft_hard_ldr)
	{
		lim_value = 1;
	} else {
		lim_value = device_config_.day_night_lim;
	}

	daynight_mode_func_lock = 1;

    while(!server_get_status(STATUS_TYPE_EXIT) && !daynight_mode_func_exit)
    {
		//exit logic
		st = server_get_status(STATUS_TYPE_STATUS);
    	if( st != STATUS_RUN ) {
			if ( st == STATUS_IDLE || st == STATUS_SETUP || st == STATUS_START)
				continue;
			else
				break;
		}

    	if(device_config_.soft_hard_ldr)
    	{
    		value = rts_av_get_isp_daynight_statis();
    		log_qcy(DEBUG_VERBOSE, "day night mode value = %d", value);

    		if(value != old_value)
    		{
				if(value >= lim_value)
				{
					//night
					old_value = value;
					ret = ctl_ircut(GPIO_OFF);
					ret |= ctl_irled(&device_config_, GPIO_ON);

					video_isp_set_attr(RTS_VIDEO_CTRL_ID_IR_MODE, RTS_ISP_IR_NIGHT);
					video_isp_set_attr(RTS_VIDEO_CTRL_ID_GRAY_MODE, RTS_ISP_IR_NIGHT);

				} else {
					//day
					old_value = value;
					ret = ctl_ircut(GPIO_ON);
					ret |= ctl_irled(&device_config_, GPIO_OFF);

					video_isp_set_attr(RTS_VIDEO_CTRL_ID_IR_MODE, RTS_ISP_IR_DAY);
					video_isp_set_attr(RTS_VIDEO_CTRL_ID_GRAY_MODE, RTS_ISP_IR_DAY);
				}
    		}
    	} else {
    		value = rts_io_adc_get_value(ADC_CHANNEL_0);
    		log_qcy(DEBUG_VERBOSE, "day night mode value = %d", value);

    		if(value != old_value)
    		{
				if(value >= lim_value)
				{
					//day
					old_value = value;
					ret = ctl_ircut(GPIO_ON);
					ret |= ctl_irled(&device_config_, GPIO_OFF);
				} else {
					//night
					old_value = value;
					ret = ctl_ircut(GPIO_OFF);
					ret |= ctl_irled(&device_config_, GPIO_ON);
				}
    		}
    	}

    	if(ret)
    		log_qcy(DEBUG_SERIOUS, "day night mode set failed");

		sleep(1);
    }

	log_qcy(DEBUG_SERIOUS, "-----%s thread exit -----", "daynight_mode_thread");
	daynight_mode_func_exit = 0;
	daynight_mode_func_lock = 0;
	pthread_exit(0);
}

static int server_setup(void)
{
	int ret = 0;
	static pthread_t motor_tid = 0;
	static pthread_t storage_detect_tid = 0;
	rts_set_log_mask(RTS_LOG_MASK_CONS);

	ret = config_device_read(&device_config_);
	if( ret )
	{
		log_qcy(DEBUG_SERIOUS, "config_manager_read failed");
		ret = -1;
		goto err;
	}

	ret = init_part_info();
	if(ret)
	{
		log_qcy(DEBUG_SERIOUS, "init_part_info init failed");
		ret = -1;
		goto err;
	}

	ret = init_led_gpio(&device_config_);
	if(ret)
	{
		log_qcy(DEBUG_SERIOUS, "init_led_gpio init failed");
		ret = -1;
		goto err;
	}

	if(device_config_.motor_enable && !step_motor_init_flag)
	{
		if ((ret = pthread_create(&motor_tid, NULL, motor_init_func, NULL))) {
			log_qcy(DEBUG_SERIOUS, "create motor init thread failed, ret=%d\n", ret);
			ret = -1;
			goto err;
		}
	}

	if(device_config_.storage_detect)
	{
		if ((ret = pthread_create(&storage_detect_tid, NULL, storage_detect_func, NULL))) {
			log_qcy(DEBUG_SERIOUS, "create storage_detect_func thread failed, ret=%d\n", ret);
			ret = -1;
			goto err;
		}
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
	audio_info_t_m ctrl_audio;

	memset(&ctrl_audio, 0, sizeof(ctrl_audio));
	ctrl_audio.volume = 100;

	ret = adjust_audio_volume(&ctrl_audio, device_config_);
	ret |= adjust_input_audio_volume(&ctrl_audio, device_config_);
	if(ret)
	{
		log_qcy(DEBUG_SERIOUS, "adjust_input_audio_volume failed\n");
		goto restart;
	}

	//day mode
	ret = ctl_ircut(GPIO_ON);
	ret |= ctl_irled(&device_config_ ,GPIO_OFF);

	ret = ctl_spk_enable(&device_config_, GPIO_OFF);
	if(ret)
	{
		log_qcy(DEBUG_SERIOUS, "close spk failed\n");
		goto restart;
	}

restart:
	if(ret)
		server_set_status(STATUS_TYPE_STATUS, STATUS_RESTART);
	else
		server_set_status(STATUS_TYPE_STATUS, STATUS_RUN);
	return ret;
}

static int server_run(void)
{
	int ret = 0;
	if( server_message_proc()!= 0)
		log_qcy(DEBUG_SERIOUS, "error in message proc");
	return ret;
}

static int server_stop(void)
{
	int ret = 0;
	return ret;
}

static int server_restart(void)
{
	int ret = 0;
	server_set_status(STATUS_TYPE_EXIT,1);
	server_release();
	server_set_status(STATUS_TYPE_STATUS, STATUS_NONE);
	return ret;
}

static int server_error(void)
{
	int ret = 0;
	server_set_status(STATUS_TYPE_EXIT,1);
	return ret;
}

static int heart_beat_proc(void)
{
	int ret = 0;
	message_t msg;
	long long int tick = 0;
	tick = time_get_now_stamp();
	if( (tick - info.tick) > 10 ) {
		info.tick = tick;
	    /********message body********/
		msg_init(&msg);
		msg.message = MSG_MANAGER_HEARTBEAT;
		msg.sender = msg.receiver = SERVER_DEVICE;
		msg.arg_in.cat = info.status;
		msg.arg_in.dog = info.thread_start;
		ret = manager_message(&msg);
		/***************************/
	}
	return ret;
}


static void *server_func(void)
{
    signal(SIGINT, (__sighandler_t)server_thread_termination);
    signal(SIGTERM, (__sighandler_t)server_thread_termination);
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
		usleep(1000 * 100);//100ms
		heart_beat_proc();
	}
	server_release();
	log_qcy(DEBUG_INFO, "-----------thread exit: server_device-----------");
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
	ret = pthread_create(&info.id, NULL, (void *)server_func, NULL);
	if(ret != 0) {
		log_qcy(DEBUG_SERIOUS, "device server create error! ret = %d",ret);
		 return ret;
	}
	else {
		log_qcy(DEBUG_SERIOUS, "device server create successful!");
		return 0;
	}
}

static char *get_string_name(int i)
{
	char *ret = NULL;

	if(i == SERVER_MANAGER)
		ret = server_name_tt[sizeof(server_name_tt)/sizeof(server_name_t) - 1].name;
	else
		ret = server_name_tt[i].name;

	return ret;
}

int server_device_message(message_t *msg)
{
	int ret=0;

	if( server_get_status(STATUS_TYPE_STATUS)!= STATUS_RUN ) {
		log_qcy(DEBUG_SERIOUS, "device server is not ready!");
		return -1;
	}
	ret = pthread_rwlock_wrlock(&message.lock);
	if(ret)	{
		log_qcy(DEBUG_SERIOUS, "add message lock fail, ret = %d\n", ret);
		return ret;
	}
	ret = msg_buffer_push(&message, msg);
	if( ret!=0 )
		log_qcy(DEBUG_SERIOUS, "message push in device error =%d", ret);
	else {
		pthread_mutex_lock(&d_mutex);
		pthread_cond_signal(&d_cond);
		pthread_mutex_unlock(&d_mutex);
	}
	log_qcy(DEBUG_INFO, "push into the device message queue: sender=%s, message=%x, ret=%d", get_string_name(msg->sender), msg->message, ret);
	ret = pthread_rwlock_unlock(&message.lock);
	if (ret)
		log_qcy(DEBUG_SERIOUS, "add message unlock fail, ret = %d\n", ret);
	return ret;
}

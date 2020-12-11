/*
* @Author: lijunxin
* @Date:   2020-09-30 14:52:05
* @Last Modified by:   lijunxin
* @Last Modified time: 2020-09-30 14:54:55
*/

#include <stdio.h>
#include "audio_control.h"
#include "time.h"
#include <rtsamixer.h>
#include "../../tools/tools_interface.h"
#include "../../manager/global_interface.h"
#include "gpio_control_interface.h"
#include "config.h"

static int value_cur;
static int value_input_cur;

int ctl_spk(int *para)
{
	int ret;

	ret = ctl_spk_enable(*para);
	return ret;
}

int adjust_input_audio_volume(audio_info_t_m *para, device_config_t config_t)
{
	audio_info_t_m *audio_control_info;
	int ret;
	int value_t;

	audio_control_info = para;

	log_qcy(DEBUG_INFO, "volume = %d",audio_control_info->volume);
	if ((audio_control_info->volume <= 100 && audio_control_info->volume >= 0) || audio_control_info->volume == -1)
	{

		log_qcy(DEBUG_INFO, "type = %d",audio_control_info->type);
		log_qcy(DEBUG_INFO, "volume = %d",audio_control_info->volume);

		ret = rts_audio_get_capture_volume(&value_input_cur);
		if (ret) {
			log_qcy(DEBUG_SERIOUS, "rts_audio_get_capture_volume failed");
			return -1;
		}

		if (audio_control_info->type > 0 && audio_control_info->volume == -1)
		{
			switch(audio_control_info->type){
				case VOLUME_UP:
					if (value_input_cur + config_t.volume_step >= 100)
					{
						value_input_cur = 90;
					}
					ret = rts_audio_set_capture_volume(value_input_cur + config_t.volume_step);
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_set_capture_volume failed");
						return -1;
					}
					ret = rts_audio_get_capture_volume(&value_t);
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_get_capture_volume failed");
						return -1;
					}
					if (value_input_cur + config_t.volume_step == value_t)
						log_qcy(DEBUG_INFO, "adjust  volume success");
					break;
				case VOLUME_DOWN:
					if (value_input_cur - config_t.volume_step <= 0)
					{
						value_input_cur = 10;
					}
					ret = rts_audio_set_capture_volume(value_input_cur - config_t.volume_step);
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_set_playback_volume failed");
						return -1;
					}
					ret = rts_audio_get_capture_volume(&value_t);
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_get_playback_volume failed");
						return -1;
					}
					if (value_input_cur - config_t.volume_step == value_t)
						log_qcy(DEBUG_INFO, "adjust volume success");
					break;
				case VOLUME_MUTE:
					ret = rts_audio_capture_mute();
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_capture_mute failed");
						return -1;
					}
					log_qcy(DEBUG_INFO, "the current capture is mute");
					break;
				case VOLUME_UNMUTE:
					ret = rts_audio_capture_unmute();
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_capture_unmute  failed");
						return -1;
					}
					log_qcy(DEBUG_INFO, "the current capture is unmute");
					break;
				default :
					log_qcy(DEBUG_SERIOUS, "invalid param");
					break;
			}
		}
		else if (audio_control_info->type == 0 && audio_control_info->volume >= 0)
		{
			if (value_input_cur == audio_control_info->volume)
				return 0;
			else
			{
				ret = rts_audio_set_capture_volume(audio_control_info->volume);
				if (ret) {
					log_qcy(DEBUG_SERIOUS, "rts_audio_set_capture_volume  failed");
					return -1;
				}
				ret = rts_audio_get_capture_volume(&value_input_cur);
				if (ret) {
					log_qcy(DEBUG_SERIOUS, "rts_audio_get_capture_volume  failed");
					return -1;
				}
				if (value_input_cur == audio_control_info->volume)
					log_qcy(DEBUG_INFO, "set volume success");
			}
		}
	}
	else {
		log_qcy(DEBUG_SERIOUS, "invalid param");
		return -1;
	}
	return 0;
}

int adjust_audio_volume(audio_info_t_m *para, device_config_t config_t)
{
	audio_info_t_m *audio_control_info;
	int ret;
	int value_t;

	audio_control_info = para;

	//log_qcy(DEBUG_INFO, "volume = %d",audio_control_info->volume);
	if ((audio_control_info->volume <= 100 && audio_control_info->volume >= 0) || audio_control_info->volume == -1)
	{

		//log_qcy(DEBUG_INFO, "type = %d",audio_control_info->type);
		//log_qcy(DEBUG_INFO, "volume = %d",audio_control_info->volume);

		ret = rts_audio_get_playback_volume(&value_cur);
		if (ret) {
			log_qcy(DEBUG_SERIOUS, "rts_audio_get_playback_volume failed");
			return -1;
		}

		if (audio_control_info->type > 0 && audio_control_info->volume == -1)
		{
			switch(audio_control_info->type){
				case VOLUME_UP:
					if (value_cur + config_t.volume_step >= 100)
					{
						value_cur = 90;
					}
					ret = rts_audio_set_playback_volume(value_cur + config_t.volume_step);
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_set_playback_volume failed");
						return -1;
					}
					ret = rts_audio_get_playback_volume(&value_t);
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_get_playback_volume failed");
						return -1;
					}
					if (value_cur + config_t.volume_step == value_t)
						log_qcy(DEBUG_INFO, "adjust  volume success");
					break;
				case VOLUME_DOWN:
					if (value_cur - config_t.volume_step <= 0)
					{
						value_cur = 10;
					}
					ret = rts_audio_set_playback_volume(value_cur - config_t.volume_step);
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_set_playback_volume failed");
						return -1;
					}
					ret = rts_audio_get_playback_volume(&value_t);
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_get_playback_volume failed");
						return -1;
					}
					if (value_cur - config_t.volume_step == value_t)
						log_qcy(DEBUG_INFO, "adjust volume success");
					break;
				case VOLUME_MUTE:
					ret = rts_audio_playback_mute();
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_playback_mute failed");
						return -1;
					}
					log_qcy(DEBUG_INFO, "the current playback is mute");
					break;
				case VOLUME_UNMUTE:
					ret = rts_audio_playback_unmute();
					if (ret) {
						log_qcy(DEBUG_SERIOUS, "rts_audio_playback_unmute failed");
						return -1;
					}
					log_qcy(DEBUG_INFO, "the current playback is unmute");
					break;
				default :
					log_qcy(DEBUG_SERIOUS, "invalid param");
					break;
			}
		}
		else if (audio_control_info->type == 0 && audio_control_info->volume >= 0)
		{
			if (value_cur == audio_control_info->volume)
				return 0;
			else
			{
				ret = rts_audio_set_playback_volume(audio_control_info->volume);
				if (ret) {
					log_qcy(DEBUG_SERIOUS, "rts_audio_set_playback_volume failed");
					return -1;
				}
				ret = rts_audio_get_playback_volume(&value_cur);
				if (ret) {
					log_qcy(DEBUG_SERIOUS, "rts_audio_get_playback_volume failed");
					return -1;
				}
				if (value_cur == audio_control_info->volume)
					log_qcy(DEBUG_INFO, "set volume success");
			}
		}
	}
	else {
		log_qcy(DEBUG_SERIOUS, "invalid param");
		return -1;
	}
	return 0;
}

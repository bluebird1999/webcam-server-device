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
#include "gpio_control_interface.h"

static int value_cur;

int ctl_spk(int *para)
{
	int ret;

	ret = ctl_spk_enable(*para);
	return ret;
}

int adjust_audio_volume(audio_info_t_m *para)
{
	audio_info_t_m *audio_control_info;
	int ret;
	int value_t;

	audio_control_info = para;

	log_err("volume = %d",audio_control_info->volume);
	if ((audio_control_info->volume < 100 && audio_control_info->volume > 0) || audio_control_info->volume == -1)
	{

		log_err("type = %d",audio_control_info->type);
		log_err("volume = %d",audio_control_info->volume);

		ret = rts_audio_get_playback_volume(&value_cur);
		if (ret) {
			log_err("rts_audio_get_playback_volume failed");
			return -1;
		}

		if (audio_control_info->type > 0 && audio_control_info->volume == -1)
		{
			switch(audio_control_info->type){
				case VOLUME_UP:
					if (value_cur + VOLUME_STEP >= 100)
					{
						value_cur = 90;
					}
					ret = rts_audio_set_playback_volume(value_cur + VOLUME_STEP);
					if (ret) {
						log_err("rts_audio_set_playback_volume failed");
						return -1;
					}
					ret = rts_audio_get_playback_volume(&value_t);
					if (ret) {
						log_err("rts_audio_get_playback_volume failed");
						return -1;
					}
					if (value_cur + VOLUME_STEP == value_t)
						log_info("adjust  volume success");
					break;
				case VOLUME_DOWN:
					if (value_cur - VOLUME_STEP <= 0)
					{
						value_cur = 10;
					}
					ret = rts_audio_set_playback_volume(value_cur - VOLUME_STEP);
					if (ret) {
						log_err("rts_audio_set_playback_volume failed");
						return -1;
					}
					ret = rts_audio_get_playback_volume(&value_t);
					if (ret) {
						log_err("rts_audio_get_playback_volume failed");
						return -1;
					}
					if (value_cur - VOLUME_STEP == value_t)
						log_info("adjust volume success");
					break;
				case VOLUME_MUTE:
					ret = rts_audio_playback_mute();
					if (ret) {
						log_err("rts_audio_playback_mute failed");
						return -1;
					}
					log_info("the current playback is mute");
					break;
				case VOLUME_UNMUTE:
					ret = rts_audio_playback_unmute();
					if (ret) {
						log_err("rts_audio_playback_unmute failed");
						return -1;
					}
					log_info("the current playback is unmute");
					break;
				default :
					log_err("invalid param");
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
					log_err("rts_audio_set_playback_volume failed");
					return -1;
				}
				ret = rts_audio_get_playback_volume(&value_cur);
				if (ret) {
					log_err("rts_audio_get_playback_volume failed");
					return -1;
				}
				if (value_cur == audio_control_info->volume)
					log_info("set volume success");
			}
		}
	}
	else {
		log_err("invalid param");
		return -1;
	}
	return 0;
}

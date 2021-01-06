/*
 * audio_control.h
 *
 *  Created on: Sep 24, 2020
 *      Author: lijunxin
 */

#ifndef AUDIO_CONTRIL_H_
#define AUDIO_CONTRIL_H_

/*
 * header
 */
#include "config.h"
/*
 * define
 */
#define VOLUME_NONE		0
#define VOLUME_UP		1
#define VOLUME_DOWN		2
#define VOLUME_MUTE		3
#define VOLUME_UNMUTE		4

//#define VOLUME_STEP 	10

/*
 * structure
 */
//must align audio_info_t
typedef struct audio_info_t_m {
	unsigned int 	type;
	unsigned int	volume;
} audio_info_t_m;

/*
 * function
 */

int adjust_audio_volume(audio_info_t_m *para, device_config_t config_t);
int adjust_input_audio_volume(audio_info_t_m *para, device_config_t config_t);
int ctl_spk(device_config_t *config_t, int *para);

#endif /* AUDIO_CONTRIL_H_ */

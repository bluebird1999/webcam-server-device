/*
 * device.h
 *
 *  Created on: Sep 24, 2020
 *      Author: lijunxin
 */

#ifndef SERVER_DEVICE_H_
#define SERVER_DEVICE_H_

/*
 * header
 */


/*
 * define
 */
#define VOLUME_MIC		0
#define VOLUME_SPEAKER	1

/*
 * structure
 */
typedef struct audio_curinfo_t {
    unsigned int    volume;
    unsigned int    mute;
} aduio_curinfo_t;


/*
 * function
 */

#endif /* SERVER_DEVICE_H_ */

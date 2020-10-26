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

#define DAY_NIGHT_LIM	3000

/*
 * structure
 */
typedef struct server_name
{
	int index;
	char name[64];
}server_name_t;

/*
 * function
 */

#endif /* SERVER_DEVICE_H_ */

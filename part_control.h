/*
 * part_control.h
 *
 *  Created on: Sep 24, 2020
 *      Author: lijunxin
 */

#ifndef PART_CONTRIL_H_
#define PART_CONTRIL_H_

/*
 * header
 */

/*
 * define
 */
#define SIZE1024 	1024
#define SIZE64	 	64
#define CMDLINE_FILE "/proc/cmdline"
#define USER_MOUNT_PATH "/opt"
/*
 * structure
 */


/*
 * function
 */
int init_part_info();
int get_part_info(void **para);
int get_user_info(void **para);
int format_userdata();
#endif /* PART_CONTRIL_H_ */

/*
 * sd_control.h
 *
 *  Created on: Sep 24, 2020
 *      Author: lijunxin
 */

#ifndef SD_CONTRIL_H_
#define SD_CONTRIL_H_

/*
 * header
 */
#include "config.h"

/*
 * define
 */
#define SIZE 					128
#define SIZE1024 				1024
#define SD_MOUNT_PATH 			"/mnt/media"
#define VFAT_FORMAT_TOOL_PATH 	"/sbin/mkfs.vfat"
#define MOUNT_PROC_PATH 		"/proc/mounts"
#define SD_PLUG_PATH 			"/sys/devices/platform/ocp/18300000.sdhc/cd"
#define MMC_BLOCK_PATH			"/dev/mmcblk0p1"
#define MMC_BLOCK_PATH_PAR		"/dev/mmcblk0"

#define FREE_T(x) {\
	free(x);\
	x=NULL;\
}


/*
 * structure
 */

typedef struct space_info_t {
    unsigned long   totalBytes;
    unsigned long   usedBytes;
    unsigned long   freeBytes;
} space_info_t;

/*
 * function
 */

int get_sd_info(void **para, device_config_t config_t);
//int get_storage_info(char * mountpoint, space_info_t *sd_info);
int format_sd();
int umount_sd();
int is_mounted(char *mount_path);
int get_sd_plug_status();

#endif /* SD_CONTRIL_H_ */

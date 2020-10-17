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

/*
 * define
 */
#define SIZE 128
#define SIZE1024 1024
#define SD_MOUNT_PATH "/mnt/media"
#define VFAT_FORMAT_TOOL_PATH "/sbin/mkfs.vfat"
#define MOUNT_PROC_PATH "/proc/mounts"
#define SD_PLUG_PATH "/sys/devices/platform/ocp/18300000.sdhc/cd"

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

int get_sd_info(void **para);
//int get_storage_info(char * mountpoint, space_info_t *sd_info);
int format_sd();

#endif /* SD_CONTRIL_H_ */
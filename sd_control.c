/*
* @Author: Marte
* @Date:   2020-09-24 10:56:09
* @Last Modified by:   Marte
* @Last Modified time: 2020-09-24 18:18:43
*/

#include <stdio.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../../tools/tools_interface.h"
#include "sd_control.h"
#include "device_interface.h"
#include "config.h"

static int get_sd_block_mountpath(char *block_path_t, char *mountpath_t);
static int exec_t(char *cmd);
static void *format_fun(void *arg);
static int get_rule(char *block_path, char *mountpath, char *src);
static int get_storage_info(char * mountpoint, space_info_t *info);
static int get_sd_status(device_config_t config_t);
static int sd_getFSType_supp(char *devPath);



static bool sd_format_status_t = false;


static int get_storage_info(char * mountpoint, space_info_t *info)
{
    struct statfs statFS;

    if(mountpoint == NULL || info == NULL)
        return -1;

    if (statfs(mountpoint, &statFS) == -1){
        log_qcy(DEBUG_SERIOUS, "statfs failed for path->[%s]\n", mountpoint);
        return (-1);
    }

    info->totalBytes = (unsigned int)((long long)statFS.f_blocks * (long long)statFS.f_frsize / 1024);
    info->freeBytes = (unsigned int)((long long)statFS.f_bfree * (long long)statFS.f_frsize / 1024);
    info->usedBytes = (unsigned int)(info->totalBytes - info->freeBytes);
    return 0;
}

static int sd_getFSType_supp(char *devPath)
{
    int fd = 0, ret;
    char data[256] = {0};

    if(!devPath)
    {
        log_qcy(DEBUG_SERIOUS,  "device is NULL!\n");
        return -1;
    }

    //read the tag data for exfat/fat32
    fd = open(devPath, O_RDONLY);
    if(!fd)
    {
        log_qcy(DEBUG_SERIOUS,  "Open device failed!\n");
        return -1;
    }
    ret = read(fd, data, sizeof(data));
    close(fd);
    if(!ret)
    {
        log_qcy(DEBUG_SERIOUS,  "read device file failed!\n");
        return -1;
    }

    if(!strncmp((data+0x52), "FAT32", strlen("FAT32"))) //th offset with 0x52 is fat32 tag;
    {
        log_qcy(DEBUG_VERBOSE,  "dev:%s, filesystem:fat32!\n", devPath);
        ret = 0;
    }
    else if(!strncmp((data+0x3), "EXFAT", strlen("EXFAT"))) //the offset with 0x3 is exfat tag;
    {
        log_qcy(DEBUG_VERBOSE,  "dev:%s, filesystem:exfat!\n", devPath);
        ret = 0;
    }
    else
    {
        log_qcy(DEBUG_SERIOUS,  "dev:%s, Unknown filesystem type!\n", devPath);
        ret = -1;
    }

    return ret;
}

int is_mounted(char *mount_path)
{
	FILE *fp = NULL;
	char data[SIZE1024] = {0};

    fp = fopen(MOUNT_PROC_PATH, "r");
    if (fp == NULL) {
        log_qcy(DEBUG_SERIOUS, "fopen: fail\n");
        return -1;
    }

    if (fread(data, 1, sizeof(data), fp) == -1) {
        log_qcy(DEBUG_SERIOUS, "fread: fail\n");
        fclose(fp);
        return -1;
    }

    fflush(fp);
    fclose(fp);

    if(strstr(data, mount_path))
    	return 1;

	return 0;
}

static int get_sd_status(device_config_t config_t)
{
	int ret = 0;
	int sd_status = 0;
	struct statfs statFS;

	sd_status = get_sd_plug_status();
	if(sd_status == SD_STATUS_NO)
		ret = SD_STATUS_NO;
	else
	{
		if(sd_format_status_t)
		{
			sd_status = SD_STATUS_FMT;
		}
		else if(sd_getFSType_supp(MMC_BLOCK_PATH))
		{
			sd_status = SD_STATUS_ERR;
		}
		else if(is_mounted(SD_MOUNT_PATH))
		{
			sd_status = SD_STATUS_PLUG;
		}
		else
		{
			sd_status = SD_STATUS_EJECTED;
		}
	}

	ret = sd_status;
	return ret;
}

int get_sd_info(void **para, device_config_t config_t)
{
    int ret;
    struct sd_info_ack_t sd_info;
    struct space_info_t space_info_t;

    sd_info.plug = get_sd_status(config_t);
    if(sd_info.plug == SD_STATUS_NO || sd_info.plug == SD_STATUS_EJECTED)
    {
    	log_qcy(DEBUG_VERBOSE, "can not find sd card\n");
        sd_info.totalBytes = 0;
        sd_info.usedBytes = 0;
        sd_info.freeBytes = 0;
    	goto ejected_or_no;
    }

    ret = get_storage_info(SD_MOUNT_PATH, &space_info_t);
    if(ret)
    {
        log_qcy(DEBUG_SERIOUS, "set_storage_info fail\n");
        return -1;
    }

    sd_info.totalBytes = space_info_t.totalBytes;
    sd_info.usedBytes = space_info_t.usedBytes;
    sd_info.freeBytes = space_info_t.freeBytes;

ejected_or_no:

    *para = calloc( sizeof(sd_info_ack_t), 1);
    if( *para == NULL ) {
        log_qcy(DEBUG_SERIOUS, "memory allocation failed");
        return -1;
    }

    memcpy((sd_info_ack_t*)(*para), &sd_info, sizeof(sd_info_ack_t));

    return 0;
}

int umount_sd()
{
	int plug;
	int ret = 0;
	int i = 0;
    char *block_path = NULL;
    char *mountpath = NULL;
    block_path = calloc(1, SIZE);
    mountpath = calloc(1, SIZE);

//	plug = get_sd_plug_status();
//

	ret = get_sd_block_mountpath(block_path, mountpath);
	if(ret < 0)
	{
		log_qcy(DEBUG_SERIOUS, "get_sd_block_mountpath prase failed\n");
		return -1;
	}

	log_qcy(DEBUG_VERBOSE, "umount mountpath = %s\n",mountpath);
	if(mountpath != NULL)
	{
//		if(is_mounted(mountpath) == 1)
//		{
			ret = umount2(mountpath, MNT_FORCE);
			if(ret)
			{
				log_qcy(DEBUG_SERIOUS, "umount failed\n");
				while(ret && i < 3)
				{
					ret = umount2(mountpath, MNT_FORCE);
					i++;
					usleep(500 * 1000);
				}
				if(ret)
				{
					log_qcy(DEBUG_SERIOUS, "system umount\n");
					system("umount  /mnt/media");
					ret = 0;
				}
			}
//		}
	}

	FREE_T(block_path);
	FREE_T(mountpath);

	return ret;
}

int get_sd_plug_status()
{
    FILE *fp = NULL;
    char data[SIZE] = {0};

    fp = fopen(SD_PLUG_PATH, "r");
    if (fp == NULL) {
        log_qcy(DEBUG_SERIOUS, "fopen: fail\n");
        return -1;
    }

    if (fread(data, 1, sizeof(data), fp) == -1) {
        log_qcy(DEBUG_SERIOUS, "fread: fail\n");
        fclose(fp);
        return -1;
    }

    log_qcy(DEBUG_VERBOSE, "get sd status = %c\n", data[0]);

    fflush(fp);
    fclose(fp);
    if (data[0] == '1')
    	return SD_STATUS_PLUG;
    else {
    	return SD_STATUS_NO;
    }
}

int get_rule(char *block_path, char *mountpath, char *src)
{
    if(block_path == NULL || mountpath == NULL || src == NULL)
        return -1;

    char *src_tmp = src;
    while(*src_tmp != ' ')
    {
        *block_path++ = *src_tmp;
        src_tmp++;
    }

    src_tmp++;

    while(*src_tmp != ' ')
    {
        *mountpath++ = *src_tmp;
        src_tmp++;
    }

    return 0;
}

static int get_sd_block_mountpath(char *block_path_t, char *mountpath_t)
{
    FILE *fp = NULL;
    char data[SIZE1024] = {0};
    char *mount_rule = NULL;
    char block_path[SIZE] = {0};
    char mountpath[SIZE] = {0};
    int ret = 0;

    if(block_path_t == NULL || mountpath_t == NULL)
        return -1;

    fp = fopen(MOUNT_PROC_PATH, "r");
    if (fp == NULL) {
        log_qcy(DEBUG_SERIOUS, "fopen: fail\n");
        return -1;
    }

    if (fread(data, 1, sizeof(data), fp) == -1) {
        log_qcy(DEBUG_SERIOUS, "fread: fail\n");
        fclose(fp);
        return -1;
    }

    fflush(fp);
    fclose(fp);

    mount_rule = strstr(data, "/dev/mmcblk");
    if(get_rule(block_path, mountpath, mount_rule))
    {
        log_qcy(DEBUG_VERBOSE, "can not prase rule\n");
        if(!access(MMC_BLOCK_PATH, R_OK))
        	memcpy(block_path_t, MMC_BLOCK_PATH, strlen(MMC_BLOCK_PATH));
        else if(!access(MMC_BLOCK_PATH_PAR, R_OK))
        	memcpy(block_path_t, MMC_BLOCK_PATH_PAR, strlen(MMC_BLOCK_PATH_PAR));

        memcpy(mountpath_t, SD_MOUNT_PATH, strlen(SD_MOUNT_PATH));

        ret = 1;
    } else {
        memcpy(block_path_t, block_path, strlen(block_path));
        memcpy(mountpath_t, mountpath, strlen(mountpath));

        ret = 2;
    }

    return ret;
}

static int exec_t(char *cmd)
{
	int ret;
	char buff[SIZE] = {0};
	FILE *fstream = NULL;

    if(cmd == NULL)
    	return -1;

    if(NULL == (fstream = popen(cmd,"r")))
    {
        log_qcy(DEBUG_SERIOUS, "execute command failed, cmd = %s\n", cmd);
        return -1;
    }

    //clear fb cache
    while(NULL != fgets(buff, sizeof(buff), fstream));

    ret = pclose(fstream);
    if (!WIFEXITED(ret))
    {
    	log_info("exec pclose failed\n");
    	return -1;
    }

    return 0;
}

void *format_fun(void *arg)
{
	char cmd[SIZE] = {0};
    char *block_path;
    char *mountpath;
    int ret;
    int i = 0;
    message_t msg;
    block_path = calloc(1, SIZE);
    mountpath = calloc(1, SIZE);

	misc_set_thread_name("format_sd_thread");
    pthread_detach(pthread_self());

    ret = get_sd_block_mountpath(block_path, mountpath);
    if(ret < 0)
    {
        log_qcy(DEBUG_SERIOUS, "get_sd_block_mountpath prase failed\n");
        goto err;
    }

    if(ret == 2)
    {
		if(mountpath != NULL)
		{
			ret = umount2(mountpath, MNT_FORCE);
			if(ret)
			{
				log_qcy(DEBUG_SERIOUS, "umount failed\n");
				while(ret && i < 3)
				{
					ret = umount2(mountpath, MNT_FORCE);
					i++;
					usleep(500 * 1000);
				}
				if(ret)
				{
					log_qcy(DEBUG_SERIOUS, "system umount\n");
					system("umount  /mnt/media");
					ret = 0;
				}
			}
		}

    }

    snprintf(cmd, SIZE, "%s %s",VFAT_FORMAT_TOOL_PATH, block_path);

    log_qcy(DEBUG_VERBOSE, "cmd = %s \n", cmd);

    ret = exec_t(cmd);
    if(ret)
    {
    	log_qcy(DEBUG_SERIOUS, "exec_t error");
    	//if exec failed,mount the original
    	ret = mount(block_path, mountpath, "vfat", 0, NULL);
    	goto err;
    }

    ret = mount(block_path, mountpath, "vfat", 0, NULL);
    if(ret)
    {
        log_qcy(DEBUG_SERIOUS, "mount failed\n");
        goto err;
    }

err:


    if(ret)
    	log_info("format sd error\n");
    else
    {
    	log_info("format sd success\n");
    }

    if(is_mounted(mountpath))
    {
    	system("sync");
		sleep(1);
		msg_init(&msg);
		msg.sender = msg.receiver = SERVER_DEVICE;
		msg.message = MSG_DEVICE_ACTION;
		msg.arg_in.cat = DEVICE_ACTION_SD_INSERT;
		server_recorder_message(&msg);
		server_player_message(&msg);
    }


	sd_format_status_t = false;
	FREE_T(block_path);
	FREE_T(mountpath);
    pthread_exit(0);
}

int format_sd()
{
    char plug;

    int ret;
    pthread_t format_tid;

    plug = get_sd_plug_status();

    if(plug == 1)
    {
    	sd_format_status_t = true;
        if ((ret = pthread_create(&format_tid, NULL, format_fun, NULL))) {
        	log_qcy(DEBUG_SERIOUS, "create format thread failed, ret = %d\n", ret);
        	return -1;
        }

    } else {
    	log_qcy(DEBUG_SERIOUS, "can not find sd card\n");
	}

    return 0;
}

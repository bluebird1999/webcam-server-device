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
#include "../../tools/tools_interface.h"
#include "sd_control.h"
#include "device_interface.h"

static int get_sd_plug_status();
static int get_sd_block_mountpath(char *block_path_t, char *mountpath_t);
static int exec_t(char *cmd);
static int format_fun(char *block_path_t);
static int get_rule(char *block_path, char *mountpath, char *src);
static int get_storage_info(char * mountpoint, space_info_t *info);

static int get_storage_info(char * mountpoint, space_info_t *info)
{
    struct statfs statFS;

    if(mountpoint == NULL || info == NULL)
        return -1;

    if (statfs(mountpoint, &statFS) == -1){
        log_err("statfs failed for path->[%s]\n", mountpoint);
        return (-1);
    }

    info->totalBytes = (unsigned int)(statFS.f_blocks * (unsigned int)statFS.f_frsize / 1024);
    info->freeBytes = (unsigned int)(statFS.f_bfree * (unsigned int)statFS.f_frsize / 1024);
    info->usedBytes = (unsigned int)(info->totalBytes - info->freeBytes);
    return 0;
}

int get_sd_info(void **para)
{
    struct sd_info_ack_t sd_info;
    struct space_info_t space_info_t;
    int ret;

    sd_info.plug = get_sd_plug_status();
    if(sd_info.plug != 1)
    {
    	log_err("can not find sd card\n");
    	return -1;
    }

    ret = get_storage_info(SD_MOUNT_PATH, &space_info_t);
    if(ret)
    {
        log_err("set_storage_info fail\n");
        return -1;
    }

    sd_info.totalBytes = space_info_t.totalBytes;
    sd_info.usedBytes = space_info_t.usedBytes;
    sd_info.freeBytes = space_info_t.freeBytes;

    *para = calloc( sizeof(sd_info_ack_t), 1);
    if( *para == NULL ) {
        log_err("memory allocation failed");
        return -1;
    }

    memcpy((sd_info_ack_t*)(*para), &sd_info, sizeof(sd_info_ack_t));

    return 0;
}

static int get_sd_plug_status()
{
    FILE *fp = NULL;
    char data[SIZE] = {0};

    fp = fopen(SD_PLUG_PATH, "r");
    if (fp == NULL) {
        log_err("fopen: fail\n");
        return -1;
    }

    if (fread(data, 1, sizeof(data), fp) == -1) {
        log_err("fread: fail\n");
        fclose(fp);
        return -1;
    }

    log_info("get sd status = %c\n", data[0]);

    fflush(fp);
    fclose(fp);
    if (data[0] == '1')
    	return 1;
    else {
    	return 0;
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

    if(block_path_t == NULL || mountpath_t == NULL)
        return -1;

    fp = fopen(MOUNT_PROC_PATH, "r");
    if (fp == NULL) {
        log_err("fopen: fail\n");
        return -1;
    }

    if (fread(data, 1, sizeof(data), fp) == -1) {
        log_err("fread: fail\n");
        fclose(fp);
        return -1;
    }

    fflush(fp);
    fclose(fp);

    mount_rule = strstr(data, "/dev/mmcblk");
    if(get_rule(block_path, mountpath, mount_rule))
    {
        log_err("can not prase rule\n");
        return -1;
    }

    memcpy(block_path_t, block_path, SIZE);
    memcpy(mountpath_t, mountpath, SIZE);

    return 0;
}

static int exec_t(char *cmd)
{
    FILE *fstream=NULL;
//  char buff[SIZE1024] = {0};

    if(cmd == NULL)
    	return -1;

    if(NULL == (fstream = popen(cmd,"r")))
    {
        printf("execute command failed\n");
        return -1;
    }
/*
    if(NULL!=fgets(buff, sizeof(buff), fstream))
    {
        printf("%s",buff);
    }
    else
    {
        pclose(fstream);
        return -1;
    }
*/
    pclose(fstream);
    return 0;
}

int format_fun(char *block_path_t)
{
	if(block_path_t == NULL)
		return -1;

    char cmd[SIZE] = {0};
    snprintf(cmd, SIZE, "%s %s",VFAT_FORMAT_TOOL_PATH, block_path_t);
    //printf("cmd = %s\n", cmd);
    return exec_t(cmd);
}

int format_sd()
{
    char plug;
    char *block_path;
    char *mountpath;
    int ret;
    block_path = malloc(SIZE);
    mountpath = malloc(SIZE);
    plug = get_sd_plug_status();

    if(plug == 1)
    {
        ret = get_sd_block_mountpath(block_path, mountpath);
        if(ret)
        {
            log_err("get_sd_block_mountpath prase failed\n");
            return -1;
        }

        ret = umount(mountpath);
        if(ret)
        {
            log_err("umount failed\n");
            return -1;
        }

        ret = format_fun(block_path);
        if(ret)
        {
            log_err("format_fun failed\n");
            return -1;
        }

        ret = mount(block_path, mountpath, "vfat", 0, NULL);
        if(ret)
        {
            log_err("mount failed\n");
            return -1;
        }

        log_info("format sd success\n");

    } else {
    	log_err("can not find sd card\n");
	}

    free(block_path);
    free(mountpath);
    return 0;
}

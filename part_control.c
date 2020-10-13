/*
* @Author: Marte
* @Date:   2020-09-24 10:56:09
* @Last Modified by:   Marte
* @Last Modified time: 2020-09-24 18:18:43
*/

#include <stdio.h>
#include <sys/vfs.h>
#include <string.h>
#include <sys/mount.h>
#include "../../tools/tools_interface.h"
#include "part_control.h"
#include "device_interface.h"

static part_msg_info_t my_part_info;

static int prase_string(char *src, unsigned int *size, char *name);

int format_userdata()
{
	//empty implementation
	return 0;
}

int get_user_info(void **para)
{
	sd_info_ack_t user_info;
	struct statfs statFS;

    if (statfs(USER_MOUNT_PATH, &statFS) == -1){
        log_err("statfs failed for path->[%s], please confirm the userdata is mounted\n", USER_MOUNT_PATH);
        return (-1);
    }

    user_info.totalBytes = (unsigned int)(statFS.f_blocks * (unsigned int)statFS.f_frsize / 1024);
    user_info.freeBytes = (unsigned int)(statFS.f_bfree * (unsigned int)statFS.f_frsize / 1024);
    user_info.usedBytes = (unsigned int)(user_info.totalBytes - user_info.freeBytes);
    user_info.plug = 1;

    *para = calloc( sizeof(sd_info_ack_t), 1);
    if( *para == NULL ) {
        log_err("memory allocation failed");
        return -1;
    }

    memcpy((sd_info_ack_t*)(*para), &user_info, sizeof(sd_info_ack_t));
    return 0;
}

int get_part_info(void **para)
{
    *para = calloc( sizeof(part_msg_info_t), 1);
    if( *para == NULL ) {
        log_err("memory allocation failed");
        return -1;
    }

	memcpy((part_msg_info_t*)(*para), &my_part_info, sizeof(part_msg_info_t));

	return 0;
}

int prase_string(char *src, unsigned int *size, char *name)
{
	if(src == NULL)
		return -1;
    int i = 0;
    //384k@0k(boot)  -> a simple
    char size_tmp[16] = {0};
    //get size
    while(*src != 'k')
    {
        size_tmp[i++] = *src;
        src++;
    }
    *size = atoi(size_tmp);

    //get name
    while(*src++ != '(');
    i = 0;
    while(*src != ')')
    {
        *(name+i) = *src;
        i++;
        src++;
    }
    return 0;
}

int init_part_info()
{
	char data[SIZE1024] = {0};
	char *mtdparts = NULL;
	FILE *fp = NULL;
	int i, j = 0;
	char string_tmp[SIZE64] = {0};

	memset(&my_part_info, 0, sizeof(my_part_info));
    fp = fopen(CMDLINE_FILE, "r");
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

    //prase cmdline, get partition name and num
    mtdparts = strstr(data, "mtdparts");
    while(*mtdparts++ != ')');
    while(*mtdparts != '\0')
    {
        i = 0;
        while(*mtdparts++ != ')')
        {
            *(string_tmp+i) = *mtdparts;
            i++;
        }

        prase_string(string_tmp, &my_part_info.part_info_sum[j].size, my_part_info.part_info_sum[j].name);
        if(!strncmp(my_part_info.part_info_sum[j].name, "userdata", 8))
        {
            j++;
            break;
        }
        j++;
    }
    //get partition num
    my_part_info.part_num = j;

    return 0;
}

/*
 * hello_devfs_ioctl.h - 用户态与内核共用的 ioctl 命令定义
 * 用户态编译时：-I 包含本目录或复制到应用工程
 */
#ifndef __HELLO_DEVFS_IOCTL_H__
#define __HELLO_DEVFS_IOCTL_H__

#include <sys/ioctl.h>

#define HELLO_IOCTL_MAGIC  'H'
#define HELLO_IOCTL_GET_VERSION _IOR(HELLO_IOCTL_MAGIC, 0, unsigned int)
#define HELLO_IOCTL_SET_VALUE   _IOW(HELLO_IOCTL_MAGIC, 1, unsigned int)
#define HELLO_IOCTL_GET_VALUE   _IOR(HELLO_IOCTL_MAGIC, 2, unsigned int)

#endif

/*
 * hello_devfs.h - /dev/hello_drv 字符设备创建与销毁
 */
#ifndef __HELLO_DEVFS_H__
#define __HELLO_DEVFS_H__

void hello_devfs_create(void);
void hello_devfs_destroy(void);

#endif

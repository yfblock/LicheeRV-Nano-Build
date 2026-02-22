/*
 * hello_drv - C 内核模块（Rust FFI 已暂时移除）
 * 本文件：模块入口
 *
 * 加载: insmod hello_drv.ko
 * 卸载: rmmod hello_drv
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "hello_procfs.h"
#include "hello_devfs.h"
#include "hello_i2c.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LicheeRV");
MODULE_DESCRIPTION("Hello driver - C kernel module");
MODULE_VERSION("1.0");

static int __init hello_init(void)
{
	pr_emerg("hello_drv: Hello from C kernel module!\n");
	hello_i2c_list_adapters();
	hello_create_procfs();
	hello_devfs_create();
	return 0;
}

static void __exit hello_exit(void)
{
	hello_devfs_destroy();
	hello_remove_procfs();
	pr_emerg("hello_drv: Goodbye from C kernel module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

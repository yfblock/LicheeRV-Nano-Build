/*
 * esp_drv - ESP 相关内核模块
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include "linux/printk.h"

#include "esp_spi.h"
#include "esp_net.h"
#include "control.h"

static int __init esp_drv_init(void)
{
	int ret, len;
	struct CameraInfo* camera_info;
	uint8_t *buffer;

	pr_info("esp_drv: module init\n");

	ret = esp_spi_init();
	if (ret)
		return ret;

	// ret = esp_net_init();
	// if (ret)
	// 	return ret;	

	ret = esp_spi_xfer_cmd_init();
	if (ret) {
		esp_spi_exit();
		return ret;
	}

	camera_info = esp_spi_xfer_camera_info();
	pr_info("camera info: %p\n", camera_info);
	if (!camera_info) {
		esp_spi_exit();
		return ret;
	}
	pr_info("camera info: width=%d, height=%d, format=%d, connected=%d\n",
		camera_info->width, camera_info->height, camera_info->format, camera_info->connected);
	
	len = esp_spi_xfer_camera_frame(&buffer);
	pr_info("camera frame len: %d\n", len);
	return 0;
}

static void __exit esp_drv_exit(void)
{
	// esp_net_exit();
	esp_spi_exit();
	pr_info("esp_drv: module exit\n");
}

module_init(esp_drv_init);
module_exit(esp_drv_exit);

MODULE_AUTHOR("LicheeRV-Nano-Build");
MODULE_DESCRIPTION("ESP driver kernel module");
MODULE_LICENSE("GPL");
/* 若 SPI 为模块，modprobe 时会先加载 spi_dw_mmio（DesignWare APB SSI） */
MODULE_SOFTDEP("pre: spi_dw_mmio");

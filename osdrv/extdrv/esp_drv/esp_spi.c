/*
 * esp_spi - SPI 总线与收发封装（复用已有从设备，不新申请片选）
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>

#include "esp_spi.h"
#include "asm-generic/io.h"
#include "asm/mmio.h"
#include "control.h"
#include "linux/delay.h"
#include "linux/printk.h"
#include "linux/types.h"

#define ESP_SPI_BUS_NUM	2

static struct spi_controller *esp_spi_ctlr;
static struct spi_device *esp_spi_dev;
static struct CameraInfo camera_info;

/* 匹配控制器下任意已存在的 SPI 从设备 */
static int esp_spi_match_any(struct device *dev, void *data)
{
	return 1;
}

int esp_spi_init(void)
{
	struct device *child;

	// devmem 0x030010D0 32 0x1 # GPIOP 18 SPI2 CS
	// devmem 0x030010DC 32 0x1 # GPIOP 21 SPI2 MISO
	// devmem 0x030010E0 32 0x1 # GPIOP 22 SPI2 MOSI
	// devmem 0x030010E4 32 0x1 # GPIOP 22 SPI2 SCK
	void* devmem = ioremap(0x03001000, 0x1000);
	writel(1, devmem + 0xD0);
	writel(1, devmem + 0xDC);
	writel(1, devmem + 0xE0);
	writel(1, devmem + 0xE4);

	esp_spi_ctlr = spi_busnum_to_master(ESP_SPI_BUS_NUM);
	if (!esp_spi_ctlr) {
		pr_err("esp_spi: spi%d not found\n", ESP_SPI_BUS_NUM);
		return -ENODEV;
	}
	pr_info("esp_spi: spi%d (bus_num=%d) acquired\n",
		ESP_SPI_BUS_NUM, esp_spi_ctlr->bus_num);

	child = device_find_child(&esp_spi_ctlr->dev, NULL, esp_spi_match_any);
	if (!child) {
		pr_err("esp_spi: no SPI device on spi%d (add a child in DT, e.g. spidev@0)\n",
		       ESP_SPI_BUS_NUM);
		put_device(&esp_spi_ctlr->dev);
		esp_spi_ctlr = NULL;
		return -ENODEV;
	}
	esp_spi_dev = to_spi_device(child);
	get_device(&esp_spi_dev->dev);
	esp_spi_dev->mode = SPI_MODE_3;
	esp_spi_dev->max_speed_hz = 100000;
	spi_setup(esp_spi_dev);
	pr_info("esp_spi: using existing device %s (cs=%u)\n",
		dev_name(&esp_spi_dev->dev), esp_spi_dev->chip_select);

	return 0;
}

void esp_spi_exit(void)
{
	if (esp_spi_dev) {
		put_device(&esp_spi_dev->dev);
		esp_spi_dev = NULL;
	}
	if (esp_spi_ctlr) {
		put_device(&esp_spi_ctlr->dev);
		esp_spi_ctlr = NULL;
	}
}

int esp_send_command(uint8_t cmd) {
	int ret;
	mdelay(1);
	ret = spi_write(esp_spi_dev, &cmd, sizeof(cmd));
	mdelay(1);
	pr_info("SPI CMD: %d LEN: %ld\n", cmd, sizeof(cmd));

	if (ret) {
		pr_err("esp_spi: send command failed %d\n", ret);
		return -1;
	}
	return 0;
}

int esp_spi_xfer_cmd_init(void) {
	return esp_send_command(SPI_CMD_INIT);
}

struct CameraInfo* esp_spi_xfer_camera_info(void) {
	int ret = 0;
	int i;
	ret = esp_send_command(SPI_CMD_GET_CAMERA_INFO);
	if (ret) {
		pr_err("esp_spi: get camera info failed %d\n", ret);
		return NULL;
	}
	mdelay(10);
	
	ret = spi_read(esp_spi_dev, &camera_info, sizeof(camera_info));
	for(i = 0; i < sizeof(camera_info); i++) {
		pr_info("camera_info[%d]: %d\n", i, ((uint8_t*)&camera_info)[i]);
	}
	mdelay(1);
	if(ret) {
		pr_err("esp_spi: read camera info failed %d\n", ret);
		return NULL;
	}
	return &camera_info;
}

int esp_spi_xfer_camera_frame(uint8_t **buffer) {
	int len = 0;
	// uint8_t *buffer;

	int ret = esp_send_command(SPI_CMD_GET_CAMERA_FRAME);
	if (ret) {
		pr_err("esp_spi: get camera frame len failed %d\n", ret);
		return -1;
	}
	pr_info("read int size: %ld\n", sizeof(int));
	ret = spi_read(esp_spi_dev, &len, sizeof(int));
	pr_info("read len: %d\n", len);
	mdelay(1);
	if (ret) {
		pr_err("esp_spi: read camera frame len failed %d\n", ret);
		return -1;
	}
	// mdelay(1);
	// *buffer = (uint8_t*)kmalloc(len, GFP_KERNEL);
	// ret = spi_read(esp_spi_dev, *buffer, len);

	return len;
}
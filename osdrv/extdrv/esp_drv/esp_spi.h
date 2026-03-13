/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ESP_SPI_H__
#define __ESP_SPI_H__

#include <linux/types.h>

enum XFER_CMD {
    XFER_CMD_INIT = 0x01,
    XFER_CMD_GET_MAC,
};

/* 初始化 SPI：获取总线并复用已有从设备。成功返回 0 */
int esp_spi_init(void);

/* 释放 SPI 引用 */
void esp_spi_exit(void);

/* 同步收发：tx_buf/tx_len 发送，rx_buf/rx_len 接收，len 取两者较大值 */
int esp_spi_xfer(u8 *tx_buf, unsigned int tx_len, u8 *rx_buf, unsigned int rx_len);

/* 先发 1 字节 cmd，再收发 tx/rx 数据 */
int esp_spi_xfer_cmd(enum XFER_CMD cmd, u8 *tx_buf, unsigned int tx_len,
		     u8 *rx_buf, unsigned int rx_len);

/* 初始化 ESP */
int esp_spi_xfer_cmd_init(void);

struct CameraInfo* esp_spi_xfer_camera_info(void);

int esp_spi_xfer_camera_frame(uint8_t **buffer);

#endif /* __ESP_SPI_H__ */

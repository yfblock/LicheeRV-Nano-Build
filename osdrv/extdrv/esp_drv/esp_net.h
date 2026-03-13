/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ESP_NET_H__
#define __ESP_NET_H__

#include <linux/netdevice.h>

/* 注册虚拟网卡 espN，成功返回 0 */
int esp_net_init(void);

/* 注销并释放网卡 */
void esp_net_exit(void);

/* 从 SPI 收到数据后，调用此函数注入协议栈 */
void esp_net_rx(const void *data, int len);

#endif /* __ESP_NET_H__ */

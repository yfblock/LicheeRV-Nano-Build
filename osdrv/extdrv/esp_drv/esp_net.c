/*
 * esp_net - 虚拟网卡，通过 SPI 与 ESP 芯片通信
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/skbuff.h>

#include "esp_net.h"
#include "esp_spi.h"
#include "linux/printk.h"

static struct net_device *esp_ndev;

static int esp_net_open(struct net_device *dev)
{
	netif_start_queue(dev);
	pr_info("esp_net: %s up\n", dev->name);
	return 0;
}

static int esp_net_stop(struct net_device *dev)
{
	netif_stop_queue(dev);
	pr_info("esp_net: %s down\n", dev->name);
	return 0;
}

static netdev_tx_t esp_net_xmit(struct sk_buff *skb, struct net_device *dev)
{
	// int ret;

	// ret = esp_spi_xfer(skb->data, skb->len, NULL, 0);
	// if (ret) {
	// 	dev->stats.tx_errors++;
	// 	dev_kfree_skb(skb);
	// 	return NETDEV_TX_OK;
	// }

	pr_info("esp net xmit: %s, len: %d\n", dev->name, skb->len);
	print_hex_dump(KERN_INFO, "esp tx: ", DUMP_PREFIX_OFFSET, 16, 1,
		       skb->data, skb->len, true);

	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static const struct net_device_ops esp_netdev_ops = {
	.ndo_open       = esp_net_open,
	.ndo_stop       = esp_net_stop,
	.ndo_start_xmit = esp_net_xmit,
};

static void esp_net_setup(struct net_device *dev)
{
	ether_setup(dev);
	dev->netdev_ops = &esp_netdev_ops;
	dev->flags |= IFF_NOARP;
	dev->features |= NETIF_F_HW_CSUM;
	eth_hw_addr_random(dev);
}

int esp_net_init(void)
{
	int ret;

	esp_ndev = alloc_netdev(0, "esp%d", NET_NAME_ENUM, esp_net_setup);
	if (!esp_ndev)
		return -ENOMEM;

	ret = register_netdev(esp_ndev);
	if (ret) {
		pr_err("esp_net: register_netdev failed %d\n", ret);
		free_netdev(esp_ndev);
		esp_ndev = NULL;
		return ret;
	}

	rtnl_lock();
	dev_open(esp_ndev, NULL);
	rtnl_unlock();

	pr_info("esp_net: registered %s (up)\n", esp_ndev->name);
	return 0;
}

void esp_net_exit(void)
{
	if (esp_ndev) {
		unregister_netdev(esp_ndev);
		free_netdev(esp_ndev);
		esp_ndev = NULL;
	}
}

void esp_net_rx(const void *data, int len)
{
	struct sk_buff *skb;

	if (!esp_ndev || !netif_running(esp_ndev))
		return;

	skb = netdev_alloc_skb(esp_ndev, len);
	if (!skb) {
		esp_ndev->stats.rx_dropped++;
		return;
	}

	skb_put_data(skb, data, len);
	skb->protocol = eth_type_trans(skb, esp_ndev);
	netif_rx(skb);

	esp_ndev->stats.rx_packets++;
	esp_ndev->stats.rx_bytes += len;
}

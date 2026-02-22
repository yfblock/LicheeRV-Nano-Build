/*
 * hello_i2c.c - I2C 初始化：insmod 时列举所有 I2C 适配器；enable 时可选向指定总线/从机发送测试消息
 * 参数通过 procfs（/proc/hello_drv/i2c_bus、i2c_addr、i2c_do_send）读写，无 module_param
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>

/* 由 procfs 读写，init_i2c_enable() 使用当前值 */
static int i2c_bus = 0;
static int i2c_addr = 0x50;
static int i2c_do_send = 1;

int hello_i2c_get_bus(void)       { return i2c_bus; }
int hello_i2c_get_addr(void)     { return i2c_addr; }
int hello_i2c_get_do_send(void)  { return i2c_do_send; }

void hello_i2c_set_bus(int val)       { i2c_bus = val; }
void hello_i2c_set_addr(int val)      { i2c_addr = val; }
void hello_i2c_set_do_send(int val)   { i2c_do_send = val ? 1 : 0; }

/* 回调：遍历到每个 I2C 设备时调用，只处理 adapter，打印后继续 */
static int _list_i2c_adapter(struct device *dev, void *data)
{
	struct i2c_adapter *adap;

	if (dev->type != &i2c_adapter_type)
		return 0;
	adap = to_i2c_adapter(dev);
	pr_emerg("hello_drv: I2C adapter %d: %s\n", adap->nr, adap->name);
	return 0;
}

/* 列举所有 I2C 适配器（在模块 insmod 时调用） */
void hello_i2c_list_adapters(void)
{
	pr_emerg("hello_drv: === I2C adapters (insmod) ===\n");
	i2c_for_each_dev(NULL, _list_i2c_adapter);
	pr_emerg("hello_drv: === end I2C adapters ===\n");
}

/* 在指定总线上向指定从机地址发送一条短测试消息（写 2 字节） */
static void _i2c_send_test_message(int bus_nr, int addr)
{
	struct i2c_adapter *adap;
	struct i2c_msg msg;
	u8 buf[2] = { 0x00, 0x55 }; /* 常见 EEPROM 寄存器 0 + 数据 */
	int ret;

	adap = i2c_get_adapter(bus_nr);
	if (!adap) {
		pr_emerg("hello_drv: i2c_get_adapter(%d) failed\n", bus_nr);
		return;
	}

	msg.addr = (u16)addr;
	msg.flags = 0;           /* 写 */
	msg.len = sizeof(buf);
	msg.buf = buf;

	ret = i2c_transfer(adap, &msg, 1);
	i2c_put_adapter(adap);

	if (ret < 0)
		pr_emerg("hello_drv: i2c_transfer(bus=%d, addr=0x%02x) failed %d\n",
			 bus_nr, addr, ret);
	else if (ret != 1)
		pr_emerg("hello_drv: i2c_transfer(bus=%d, addr=0x%02x) returned %d\n",
			 bus_nr, addr, ret);
	else
		pr_emerg("hello_drv: i2c_transfer(bus=%d, addr=0x%02x) ok, sent %zu bytes\n",
			 bus_nr, addr, sizeof(buf));
}

void init_i2c_enable(void)
{
	if (i2c_do_send && i2c_addr >= 0 && i2c_addr <= 0x7f) {
		pr_emerg("hello_drv: send test message on bus %d addr 0x%02x\n",
			 i2c_bus, i2c_addr);
		_i2c_send_test_message(i2c_bus, i2c_addr);
	}
}

void init_i2c_disable(void)
{
	pr_emerg("hello_drv: init_i2c disable\n");
}

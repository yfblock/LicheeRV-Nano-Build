/*
 * hello_i2c.h - I2C 初始化 enable/disable 与 procfs 可调参数接口
 */
#ifndef __HELLO_I2C_H__
#define __HELLO_I2C_H__

void init_i2c_enable(void);
void init_i2c_disable(void);

/* 列举所有 I2C 适配器（在模块 insmod 时调用） */
void hello_i2c_list_adapters(void);

/* procfs 用：读写 I2C 参数（bus、addr、do_send） */
int hello_i2c_get_bus(void);
int hello_i2c_get_addr(void);
int hello_i2c_get_do_send(void);
void hello_i2c_set_bus(int val);
void hello_i2c_set_addr(int val);
void hello_i2c_set_do_send(int val);

#endif

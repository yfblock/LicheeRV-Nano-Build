/*
 * hello_gpio.h - GPIO 初始化 enable/disable、pinmux、设置高/低 与 procfs 可调参数接口
 */
#ifndef __HELLO_GPIO_H__
#define __HELLO_GPIO_H__

void init_gpio_enable(void);
void init_gpio_disable(void);

/* 将当前控制的 GPIO 设为高或低（line 需与当前 gpio_line 一致；high 1=高 0=低） */
void hello_gpio_set_level(int line, int high);

/* procfs 用：读写 GPIO 参数（line、iomem_bank、pinmux_reg、pinmux_val、initial_high） */
int hello_gpio_get_line(void);
int hello_gpio_get_iomem_bank(void);
int hello_gpio_get_pinmux_reg(void);
int hello_gpio_get_pinmux_val(void);
int hello_gpio_get_initial_high(void);
void hello_gpio_set_line(int val);
void hello_gpio_set_iomem_bank(int val);
void hello_gpio_set_pinmux_reg(int val);
void hello_gpio_set_pinmux_val(int val);
void hello_gpio_set_initial_high(int val);

/* 获取 GPIO 控制器基地址（物理地址），index 0..4 对应 gpio0..gpio4；无效 index 返回 0 */
unsigned long hello_gpio_get_base(unsigned int index);
/* GPIO 控制器数量（sg200x 为 5：porta..porte） */
#define HELLO_GPIO_NR_BANKS 5

/* enable 时 ioremap 后的虚拟基地址，未映射时为 NULL；可用 readl(addr+off)/writel(val, addr+off) */
void __iomem *hello_gpio_get_iomem(void);

#endif

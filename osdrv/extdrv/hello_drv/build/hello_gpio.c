/*
 * hello_gpio.c - 通过 gpiolib 控制 CVitek/DesignWare GPIO + enable 时 ioremap 指定物理地址
 * 支持：可选 pinmux（将指定 pad 切到 GPIO）、设置输出高/低。
 * 参数通过 procfs（/proc/hello_drv/gpio_line、gpio_bank、pinmux_reg、pinmux_val、gpio_initial_high）读写，无 module_param
 */

/* 内核头文件 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>   /* gpiolib 接口：gpio_request/gpio_direction_output/gpio_set_value/gpio_free */
#include <linux/io.h>     /* ioremap/iounmap 等 I/O 内存映射 */

#include "hello_gpio.h"

/* 每个 GPIO bank 的 I/O 映射大小（与设备树 reg 中 length 一致） */
#define HELLO_GPIO_IOMEM_SIZE  0x1000

/* CVitek/SG200x pinctrl 物理基地址与大小（设备树 pinctrl@3001000） */
#define HELLO_PINMUX_PHYS_BASE  0x03001000
#define HELLO_PINMUX_SIZE       0x1000
/* FUNCSEL 为低 3 位，值 3 表示 GPIO（见 cv1822_pinlist_swconfig.h 中 XGPIOA_*） */
#define HELLO_PINMUX_MASK       0x7
#define HELLO_PINMUX_VAL_GPIO   3

/* 由 procfs 读写，init_gpio_enable() 使用当前值 */
static int gpio_line = 0;
static int gpio_iomem_bank = 0;
static int pinmux_reg = 0;
static int pinmux_val = HELLO_PINMUX_VAL_GPIO;
static int gpio_initial_high = 1;

int hello_gpio_get_line(void)          { return gpio_line; }
int hello_gpio_get_iomem_bank(void)   { return gpio_iomem_bank; }
int hello_gpio_get_pinmux_reg(void)    { return pinmux_reg; }
int hello_gpio_get_pinmux_val(void)    { return pinmux_val; }
int hello_gpio_get_initial_high(void) { return gpio_initial_high ? 1 : 0; }

void hello_gpio_set_line(int val)          { gpio_line = val; }
void hello_gpio_set_iomem_bank(int val)    { gpio_iomem_bank = val; }
void hello_gpio_set_pinmux_reg(int val)    { pinmux_reg = val; }
void hello_gpio_set_pinmux_val(int val)    { pinmux_val = val & HELLO_PINMUX_MASK; }
void hello_gpio_set_initial_high(int val)  { gpio_initial_high = val ? 1 : 0; }

/* 运行时状态：是否已通过 gpio_request 占用该 GPIO */
static bool gpio_requested;
/* enable 时 ioremap 得到的虚拟地址，供其他模块访问该 bank 寄存器 */
static void __iomem *gpio_iomem;
/* 是否已对指定 bank 做过 ioremap（避免重复映射） */
static bool gpio_iomem_mapped;

/* sg200x GPIO 控制器物理基地址（来自设备树 reg），每块 0x1000 */
static const unsigned long gpio_bases[HELLO_GPIO_NR_BANKS] = {
	0x03020000, /* gpio0 porta */
	0x03021000, /* gpio1 portb */
	0x03022000, /* gpio2 portc */
	0x03023000, /* gpio3 portd */
	0x05021000, /* gpio4 porte */
};

/**
 * hello_gpio_get_base - 根据 bank 索引返回该 GPIO 控制器的物理基地址
 * @index: bank 索引 (0..HELLO_GPIO_NR_BANKS-1)
 * 返回: 物理地址，越界时返回 0
 */
unsigned long hello_gpio_get_base(unsigned int index)
{
	if (index >= HELLO_GPIO_NR_BANKS)
		return 0;
	return gpio_bases[index];
}

/**
 * hello_gpio_get_iomem - 返回当前已 ioremap 的 GPIO bank 虚拟地址
 * 返回: 若已映射则返回 __iomem 指针，否则 NULL
 */
void __iomem *hello_gpio_get_iomem(void)
{
	return gpio_iomem_mapped ? gpio_iomem : NULL;
}

/* 将指定 pad 的 pinmux 设为给定功能（仅当 pinmux_reg != 0 时在 enable 中调用） */
static void hello_apply_pinmux(void)
{
	void __iomem *base;
	unsigned int val;

	if (pinmux_reg <= 0 || pinmux_reg >= HELLO_PINMUX_SIZE)
		return;
	base = ioremap(HELLO_PINMUX_PHYS_BASE, HELLO_PINMUX_SIZE);
	if (!base) {
		pr_emerg("hello_drv: pinmux ioremap(0x%x) failed\n", (unsigned int)HELLO_PINMUX_PHYS_BASE);
		return;
	}
	val = readl(base + pinmux_reg);
	val = (val & ~HELLO_PINMUX_MASK) | (pinmux_val & HELLO_PINMUX_MASK);
	writel(val, base + pinmux_reg);
	iounmap(base);
	pr_emerg("hello_drv: pinmux reg 0x%x = 0x%x (func=%d)\n",
		 (unsigned int)pinmux_reg, (unsigned int)val, pinmux_val);
}

/**
 * hello_gpio_set_level - 将当前控制的 GPIO 设为高或低
 * @line: GPIO 全局编号（需与模块参数 gpio_line 一致才生效）
 * @high: 1=高电平，0=低电平
 */
void hello_gpio_set_level(int line, int high)
{
	if (!gpio_requested || line != gpio_line)
		return;
	gpio_set_value(line, high ? 1 : 0);
}

/**
 * init_gpio_enable - 使能 GPIO：可选 pinmux、ioremap 指定 bank、申请 GPIO 并设为输出
 * 行为：若 pinmux_reg>0 则先设 pinmux；若 gpio_iomem_bank>=0 则映射 bank；申请 gpio_line 并置初值
 */
void init_gpio_enable(void)
{
	int ret;
	unsigned long phys;

	/* 校验模块参数 gpio_line 是否在有效范围内 */
	if (!gpio_is_valid(gpio_line)) {
		pr_emerg("hello_drv: init_gpio enable failed, invalid gpio_line=%d\n", gpio_line);
		return;
	}
	/* 若指定了 pinmux 寄存器偏移，则将该 pad 切到指定功能（通常为 GPIO） */
	if (pinmux_reg > 0)
		hello_apply_pinmux();
	/* 若指定了 bank 且尚未映射，则 ioremap 该 GPIO 控制器物理基地址 */
	if (gpio_iomem_bank >= 0 && gpio_iomem_bank < HELLO_GPIO_NR_BANKS && !gpio_iomem_mapped) {
		phys = hello_gpio_get_base(gpio_iomem_bank);
		gpio_iomem = ioremap(phys, HELLO_GPIO_IOMEM_SIZE);
		if (gpio_iomem) {
			gpio_iomem_mapped = true;
			pr_emerg("hello_drv: ioremap gpio bank %d phys=0x%lx -> %p\n",
				 gpio_iomem_bank, phys, gpio_iomem);
		} else {
			pr_emerg("hello_drv: ioremap(0x%lx, %u) failed\n", phys, HELLO_GPIO_IOMEM_SIZE);
		}
	}
	/* 首次 enable 时申请该 GPIO 线，避免重复 request */
	if (!gpio_requested) {
		ret = gpio_request(gpio_line, "hello_drv");
		if (ret) {
			pr_emerg("hello_drv: gpio_request(%d) failed %d\n", gpio_line, ret);
			return;
		}
		gpio_requested = true;
	}
	/* 配置为输出并设为初始电平（高/低） */
	ret = gpio_direction_output(gpio_line, gpio_initial_high ? 1 : 0);
	if (ret)
		pr_emerg("hello_drv: gpio_direction_output(%d) failed %d\n", gpio_line, ret);
	else
		pr_emerg("hello_drv: init_gpio enable, gpio%d=%s\n",
			 gpio_line, gpio_initial_high ? "high" : "low");
}

/**
 * init_gpio_disable - 关闭 GPIO：先 iounmap（若曾 ioremap），再输出低并释放 GPIO
 * 行为：与 init_gpio_enable 对称，确保资源释放顺序正确
 */
void init_gpio_disable(void)
{
	/* 若曾对 bank 做过 ioremap，先解除映射 */
	if (gpio_iomem_mapped && gpio_iomem) {
		iounmap(gpio_iomem);
		gpio_iomem = NULL;
		gpio_iomem_mapped = false;
		pr_emerg("hello_drv: iounmap gpio bank %d\n", gpio_iomem_bank);
	}
	if (!gpio_requested)
		return;
	/* 输出低电平后再释放，避免释放后引脚状态未定义 */
	gpio_set_value(gpio_line, 0);
	gpio_free(gpio_line);
	gpio_requested = false;
	pr_emerg("hello_drv: init_gpio disable, gpio%d released\n", gpio_line);
}

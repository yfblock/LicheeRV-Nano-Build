/*
 * hello_procfs.c - /proc/hello_drv 下各文件的读写接口
 */
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#include "hello_i2c.h"
#include "hello_gpio.h"
#include "hello_procfs.h"

#define HELLO_PROC_BUF_SIZE 256

struct hello_proc_ent {
	const char *name;
	char buf[HELLO_PROC_BUF_SIZE];
};

static struct proc_dir_entry *hello_proc_dir;
static struct proc_dir_entry *proc_init_snsr;
static struct proc_dir_entry *proc_init_gpio;
static struct proc_dir_entry *proc_init_i2c;
static struct proc_dir_entry *proc_getchnframe;
static struct proc_dir_entry *proc_i2c_bus;
static struct proc_dir_entry *proc_i2c_addr;
static struct proc_dir_entry *proc_i2c_do_send;
static struct proc_dir_entry *proc_gpio_line;
static struct proc_dir_entry *proc_gpio_bank;
static struct proc_dir_entry *proc_pinmux_reg;
static struct proc_dir_entry *proc_pinmux_val;
static struct proc_dir_entry *proc_gpio_initial_high;

static struct hello_proc_ent ent_init_snsr        = { "init_snsr",        "" };
static struct hello_proc_ent ent_init_gpio        = { "init_gpio",        "" };
static struct hello_proc_ent ent_init_i2c         = { "init_i2c",         "" };
static struct hello_proc_ent ent_getchnframe      = { "getchnframe",      "" };
static struct hello_proc_ent ent_i2c_bus          = { "i2c_bus",          "" };
static struct hello_proc_ent ent_i2c_addr         = { "i2c_addr",         "" };
static struct hello_proc_ent ent_i2c_do_send      = { "i2c_do_send",      "" };
static struct hello_proc_ent ent_gpio_line        = { "gpio_line",        "" };
static struct hello_proc_ent ent_gpio_bank        = { "gpio_bank",        "" };
static struct hello_proc_ent ent_pinmux_reg       = { "pinmux_reg",       "" };
static struct hello_proc_ent ent_pinmux_val       = { "pinmux_val",       "" };
static struct hello_proc_ent ent_gpio_initial_high = { "gpio_initial_high", "" };

static int hello_proc_show(struct seq_file *m, void *v)
{
	struct hello_proc_ent *ent = m->private;

	if (!ent)
		return 0;

	/* I2C 参数：只显示当前数值 */
	if (strcmp(ent->name, "i2c_bus") == 0) {
		seq_printf(m, "%d\n", hello_i2c_get_bus());
		return 0;
	}
	if (strcmp(ent->name, "i2c_addr") == 0) {
		seq_printf(m, "%d\n", hello_i2c_get_addr());
		return 0;
	}
	if (strcmp(ent->name, "i2c_do_send") == 0) {
		seq_printf(m, "%d\n", hello_i2c_get_do_send());
		return 0;
	}

	/* GPIO 参数：只显示当前数值 */
	if (strcmp(ent->name, "gpio_line") == 0) {
		seq_printf(m, "%d\n", hello_gpio_get_line());
		return 0;
	}
	if (strcmp(ent->name, "gpio_bank") == 0) {
		seq_printf(m, "%d\n", hello_gpio_get_iomem_bank());
		return 0;
	}
	if (strcmp(ent->name, "pinmux_reg") == 0) {
		seq_printf(m, "%d\n", hello_gpio_get_pinmux_reg());
		return 0;
	}
	if (strcmp(ent->name, "pinmux_val") == 0) {
		seq_printf(m, "%d\n", hello_gpio_get_pinmux_val());
		return 0;
	}
	if (strcmp(ent->name, "gpio_initial_high") == 0) {
		seq_printf(m, "%d\n", hello_gpio_get_initial_high());
		return 0;
	}

	if (ent->buf[0])
		seq_printf(m, "%s: written: %s\n", ent->name, ent->buf);
	else
		seq_printf(m, "%s: TODO\n", ent->name);
	return 0;
}

static int hello_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hello_proc_show, PDE_DATA(inode));
}

static ssize_t hello_proc_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct seq_file *m = file->private_data;
	struct hello_proc_ent *ent;
	size_t n;

	if (!m || !m->private)
		return -EINVAL;
	ent = m->private;

	if (count == 0)
		return 0;

	n = count;
	if (n >= HELLO_PROC_BUF_SIZE)
		n = HELLO_PROC_BUF_SIZE - 1;

	if (copy_from_user(ent->buf, buf, n))
		return -EFAULT;
	ent->buf[n] = '\0';

	/* I2C 参数：写十进制或 0x 十六进制 */
	if (strcmp(ent->name, "i2c_bus") == 0) {
		int val;
		if (kstrtoint(ent->buf, 0, &val) == 0) {
			hello_i2c_set_bus(val);
			return count;
		}
		return -EINVAL;
	}
	if (strcmp(ent->name, "i2c_addr") == 0) {
		int val;
		if (kstrtoint(ent->buf, 0, &val) == 0 && val >= 0 && val <= 0x7f) {
			hello_i2c_set_addr(val);
			return count;
		}
		return -EINVAL;
	}
	if (strcmp(ent->name, "i2c_do_send") == 0) {
		int val;
		if (kstrtoint(ent->buf, 0, &val) == 0) {
			hello_i2c_set_do_send(val);
			return count;
		}
		return -EINVAL;
	}

	/* GPIO 参数：写十进制或 0x 十六进制 */
	if (strcmp(ent->name, "gpio_line") == 0) {
		int val;
		if (kstrtoint(ent->buf, 0, &val) == 0) {
			hello_gpio_set_line(val);
			return count;
		}
		return -EINVAL;
	}
	if (strcmp(ent->name, "gpio_bank") == 0) {
		int val;
		if (kstrtoint(ent->buf, 0, &val) == 0) {
			hello_gpio_set_iomem_bank(val);
			return count;
		}
		return -EINVAL;
	}
	if (strcmp(ent->name, "pinmux_reg") == 0) {
		int val;
		if (kstrtoint(ent->buf, 0, &val) == 0) {
			hello_gpio_set_pinmux_reg(val);
			return count;
		}
		return -EINVAL;
	}
	if (strcmp(ent->name, "pinmux_val") == 0) {
		int val;
		if (kstrtoint(ent->buf, 0, &val) == 0) {
			hello_gpio_set_pinmux_val(val);
			return count;
		}
		return -EINVAL;
	}
	if (strcmp(ent->name, "gpio_initial_high") == 0) {
		int val;
		if (kstrtoint(ent->buf, 0, &val) == 0) {
			hello_gpio_set_initial_high(val);
			return count;
		}
		return -EINVAL;
	}

	if (strcmp(ent->name, "init_i2c") == 0 || strcmp(ent->name, "init_gpio") == 0) {
		char ch = ent->buf[0];
		if (ch == '1') {
			if (strcmp(ent->name, "init_i2c") == 0)
				init_i2c_enable();
			else
				init_gpio_enable();
			strncpy(ent->buf, "1 (enabled)", HELLO_PROC_BUF_SIZE - 1);
			ent->buf[HELLO_PROC_BUF_SIZE - 1] = '\0';
		} else if (ch == '0') {
			if (strcmp(ent->name, "init_i2c") == 0)
				init_i2c_disable();
			else
				init_gpio_disable();
			strncpy(ent->buf, "0 (disabled)", HELLO_PROC_BUF_SIZE - 1);
			ent->buf[HELLO_PROC_BUF_SIZE - 1] = '\0';
		} else {
			return -EINVAL;
		}
	}

	return count;
}

static const struct proc_ops hello_proc_ops = {
	.proc_open	= hello_proc_open,
	.proc_read	= seq_read,
	.proc_write	= hello_proc_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

void hello_create_procfs(void)
{
	if (hello_proc_dir)
		return;

	hello_proc_dir = proc_mkdir("hello_drv", NULL);
	if (!hello_proc_dir) {
		remove_proc_entry("hello_drv", NULL);
		hello_proc_dir = proc_mkdir("hello_drv", NULL);
	}
	if (!hello_proc_dir) {
		pr_emerg("hello_drv: failed to create /proc/hello_drv\n");
		return;
	}

	proc_init_snsr = proc_create_data("init_snsr", 0644, hello_proc_dir,
					 &hello_proc_ops, &ent_init_snsr);
	proc_init_gpio = proc_create_data("init_gpio", 0644, hello_proc_dir,
					  &hello_proc_ops, &ent_init_gpio);
	proc_init_i2c = proc_create_data("init_i2c", 0644, hello_proc_dir,
					 &hello_proc_ops, &ent_init_i2c);
	proc_getchnframe = proc_create_data("getchnframe", 0644, hello_proc_dir,
					    &hello_proc_ops, &ent_getchnframe);
	proc_i2c_bus = proc_create_data("i2c_bus", 0644, hello_proc_dir,
					&hello_proc_ops, &ent_i2c_bus);
	proc_i2c_addr = proc_create_data("i2c_addr", 0644, hello_proc_dir,
					 &hello_proc_ops, &ent_i2c_addr);
	proc_i2c_do_send = proc_create_data("i2c_do_send", 0644, hello_proc_dir,
					    &hello_proc_ops, &ent_i2c_do_send);
	proc_gpio_line = proc_create_data("gpio_line", 0644, hello_proc_dir,
					 &hello_proc_ops, &ent_gpio_line);
	proc_gpio_bank = proc_create_data("gpio_bank", 0644, hello_proc_dir,
					  &hello_proc_ops, &ent_gpio_bank);
	proc_pinmux_reg = proc_create_data("pinmux_reg", 0644, hello_proc_dir,
					   &hello_proc_ops, &ent_pinmux_reg);
	proc_pinmux_val = proc_create_data("pinmux_val", 0644, hello_proc_dir,
					   &hello_proc_ops, &ent_pinmux_val);
	proc_gpio_initial_high = proc_create_data("gpio_initial_high", 0644, hello_proc_dir,
						  &hello_proc_ops, &ent_gpio_initial_high);
}

void hello_remove_procfs(void)
{
	if (!hello_proc_dir)
		return;
	remove_proc_entry("init_snsr", hello_proc_dir);
	remove_proc_entry("init_gpio", hello_proc_dir);
	remove_proc_entry("init_i2c", hello_proc_dir);
	remove_proc_entry("getchnframe", hello_proc_dir);
	remove_proc_entry("i2c_bus", hello_proc_dir);
	remove_proc_entry("i2c_addr", hello_proc_dir);
	remove_proc_entry("i2c_do_send", hello_proc_dir);
	remove_proc_entry("gpio_line", hello_proc_dir);
	remove_proc_entry("gpio_bank", hello_proc_dir);
	remove_proc_entry("pinmux_reg", hello_proc_dir);
	remove_proc_entry("pinmux_val", hello_proc_dir);
	remove_proc_entry("gpio_initial_high", hello_proc_dir);
	remove_proc_entry("hello_drv", NULL);
	hello_proc_dir = NULL;
}

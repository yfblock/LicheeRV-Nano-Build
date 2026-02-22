/*
 * rust_kernel.c - Rust 调用的内核打印桥接
 *
 * 使用 pr_emerg 确保输出不受 console_loglevel 限制，
 * 在 LicheeRV 等设备上 dmesg 可见（printk 级别常为 0）
 */
#include <linux/kernel.h>

void rust_kernel_pr_info(const char *s)
{
	pr_emerg("%s", s);
}

void rust_kernel_pr_debug(const char *s)
{
	pr_emerg("%s", s);
}

void rust_kernel_pr_err(const char *s)
{
	pr_emerg("%s", s);
}

void rust_kernel_pr_warn(const char *s)
{
	pr_emerg("%s", s);
}

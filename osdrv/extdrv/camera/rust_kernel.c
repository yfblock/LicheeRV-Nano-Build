/*
 * rust_kernel.c - Rust 调用的内核打印桥接（sg200x-vi 的 pr_info/print/println/panic 等）
 */
#include <linux/kernel.h>

void rust_kernel_pr_info(const char *s)
{
	pr_info("%s", s);
}

void rust_kernel_pr_debug(const char *s)
{
	pr_debug("%s", s);
}

void rust_kernel_pr_err(const char *s)
{
	pr_err("%s", s);
}

void rust_kernel_pr_warn(const char *s)
{
	pr_warn("%s", s);
}

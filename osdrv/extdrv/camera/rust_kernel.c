/*
 * rust_kernel.c - Rust 调用的内核打印桥接（sg200x-vi 的 pr_info/print/println/panic 等）
 * 以及供 #[global_allocator] 使用的 kmalloc/kfree 桥接。
 */
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>

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

/* ioremap 在 RISC-V 内核中是宏，没有可链接符号；用此桥接函数供 Rust 调用 */
void __iomem *rust_ioremap(phys_addr_t phys_addr, size_t size)
{
	return ioremap(phys_addr, size);
}

void rust_iounmap(void __iomem *addr)
{
	iounmap(addr);
}

/* 供 Rust #[global_allocator] 使用：内核堆分配/释放 */
void *rust_kmalloc(size_t size)
{
	return kmalloc(size, GFP_KERNEL);
}

void rust_kfree(void *ptr)
{
	kfree(ptr);
}

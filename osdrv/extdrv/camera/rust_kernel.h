/*
 * rust_kernel.h - 供其他 C 文件使用的 Rust (sg200x-vi) FFI 声明
 * 包含本头文件即可调用 Rust 导出的函数，无需重复 extern。
 */
#ifndef __CAMERA_RUST_KERNEL_H__
#define __CAMERA_RUST_KERNEL_H__

#include <linux/types.h>

/* ----- Rust 导出：版本与日志 ----- */
extern u32 sg200x_vi_version(void);
extern void sg200x_vi_log_init(void);
extern int mipi_init_gpio(void);

/* ----- Rust 导出：MIPI 初始化/退出 ----- */
extern int mipi_init(void);
extern int mipi_exit(void);

#endif /* __CAMERA_RUST_KERNEL_H__ */

/*
 * camera 合并模块单一入口：只在此处使用 module_init/module_exit，
 * 依次调用 VI 与 MIPI-RX 的注册/反注册，避免多个 init_module/cleanup_module 重复定义。
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include <vi_core.h>

/* CIF 的 init/exit 在 mipi-rx/chip/mars/cif.c 中实现 */
extern int cvi_cif_init(void);
extern void cvi_cif_exit(void);

/* Rust FFI：sg200x-vi 静态库导出（见 sg200x-vi/src/lib.rs），必须参与链接 */
extern u64 sg200x_vi_add(u64 a, u64 b);
extern u32 sg200x_vi_version(void);

static int __init camera_init(void)
{
	int r;

	pr_info("camera: sg200x_vi version 0x%08x, 1+2=%llu\n",
		(unsigned int)sg200x_vi_version(),
		(unsigned long long)sg200x_vi_add(1, 2));

	r = vi_core_register();
	if (r)
		return r;
	r = cvi_cif_init();
	if (r) {
		vi_core_unregister();
		return r;
	}
	return 0;
}

static void __exit camera_cleanup(void)
{
	cvi_cif_exit();
	vi_core_unregister();
}

module_init(camera_init);
module_exit(camera_cleanup);

MODULE_DESCRIPTION("Cvitek camera (VI + MIPI RX)");
MODULE_LICENSE("GPL");

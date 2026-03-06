#![no_std]
//! sg200x-vi：Rust 静态库，通过 FFI 供 C 内核模块 (camera.ko) 调用。
//! 使用 pr_info!/print!/println! 会输出到内核 pr_info。
//! 也可使用 [log] crate：C 在模块 init 时调用 `sg200x_vi_log_init()` 后，Rust 中可用
//! `log::trace!` / `log::debug!` / `log::info!` / `log::warn!` / `log::error!`，
//! 分别对应内核的 pr_debug、pr_debug、pr_info、pr_warn、pr_err。
//!
//! **RISC-V 重定位**：本 crate 经 LLVM 编译后会生成重定位类型 **R_RISCV_32_PCREL (57)**（如 .eh_frame、PC 相对数据等）。
//! 内核模块加载器必须支持该类型，否则 insmod 会报 "Unknown relocation type 57"。需在
//! `arch/riscv/kernel/module.c` 中实现 `apply_r_riscv_32_pcrel_rela` 并加入 `reloc_handlers_rela`。

pub mod lang_items;
pub mod regs;
mod kernel_print;
mod kernel_logger;

pub use kernel_logger::init_kernel_logger;

/// 供 C 调用的版本号（示例）；仅返回值，由 C 端 pr_info 打印以减体积
#[unsafe(no_mangle)]
pub extern "C" fn sg200x_vi_version() -> u32 {
    0x0001_0000 /* 1.0 */
}

/// 供 C 在模块 init 时调用：初始化 log 前端，使 log::debug!/info!/warn!/error! 输出到 pr_debug/pr_info/pr_warn/pr_err。
/// 仅需调用一次。
#[unsafe(no_mangle)]
pub extern "C" fn sg200x_vi_log_init() {
    use log::LevelFilter;
    init_kernel_logger(LevelFilter::Debug);
    log::info!("sg200x-vi: log initialized");
}

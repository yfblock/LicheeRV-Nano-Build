#![no_std]
//! sg200x-vi：Rust 静态库，通过 FFI 供 C 内核模块 (camera.ko) 调用。
//! 使用 pr_info!/print!/println! 会输出到内核 pr_info。
//!
//! **RISC-V 重定位**：本 crate 经 LLVM 编译后会生成重定位类型 **R_RISCV_32_PCREL (57)**（如 .eh_frame、PC 相对数据等）。
//! 内核模块加载器必须支持该类型，否则 insmod 会报 "Unknown relocation type 57"。需在
//! `arch/riscv/kernel/module.c` 中实现 `apply_r_riscv_32_pcrel_rela` 并加入 `reloc_handlers_rela`。

pub mod lang_items;
mod kernel_print;

/// 供 C 调用的加法（示例）
#[unsafe(no_mangle)]
pub extern "C" fn sg200x_vi_add(a: u64, b: u64) -> u64 {
    a + b
}

/// 供 C 调用的版本号（示例）；仅返回值，由 C 端 pr_info 打印以减体积
#[unsafe(no_mangle)]
pub extern "C" fn sg200x_vi_version() -> u32 {
    0x0001_0000 /* 1.0 */
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        assert_eq!(sg200x_vi_add(2, 2), 4);
        assert_eq!(sg200x_vi_version(), 0x0001_0000);
    }
}

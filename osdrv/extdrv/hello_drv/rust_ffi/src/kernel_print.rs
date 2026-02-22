//! 仅字面量宏，直接调 C 的 pr_*，避免 core::fmt 以减小体积

// C 提供的底层实现（由 rust_kernel.c 定义）
extern "C" {
    fn rust_kernel_pr_info(s: *const u8);
    fn rust_kernel_pr_debug(s: *const u8);
    fn rust_kernel_pr_err(s: *const u8);
    fn rust_kernel_pr_warn(s: *const u8);
}

#[macro_export]
macro_rules! pr_info {
    ($s:literal) => {{
        const _B: &[u8] = concat!($s, "\n\0").as_bytes();
        $crate::kernel_print::call_pr_info(_B.as_ptr());
    }};
}

#[macro_export]
macro_rules! pr_debug {
    ($s:literal) => {{
        const _B: &[u8] = concat!($s, "\n\0").as_bytes();
        $crate::kernel_print::call_pr_debug(_B.as_ptr());
    }};
}

#[macro_export]
macro_rules! pr_err {
    ($s:literal) => {{
        const _B: &[u8] = concat!($s, "\n\0").as_bytes();
        $crate::kernel_print::call_pr_err(_B.as_ptr());
    }};
}

#[macro_export]
macro_rules! pr_warn {
    ($s:literal) => {{
        const _B: &[u8] = concat!($s, "\n\0").as_bytes();
        $crate::kernel_print::call_pr_warn(_B.as_ptr());
    }};
}

pub fn call_pr_info(ptr: *const u8) {
    unsafe { rust_kernel_pr_info(ptr) }
}

pub fn call_pr_debug(ptr: *const u8) {
    unsafe { rust_kernel_pr_debug(ptr) }
}

pub fn call_pr_err(ptr: *const u8) {
    unsafe { rust_kernel_pr_err(ptr) }
}

pub fn call_pr_warn(ptr: *const u8) {
    unsafe { rust_kernel_pr_warn(ptr) }
}

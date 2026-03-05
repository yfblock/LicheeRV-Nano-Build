//! 内核打印：pr_info 等宏，以及 print!/println! 转为 pr_info 输出（由 C 的 rust_kernel.c 提供实现）
#[allow(dead_code)]

unsafe extern "C" {
    fn rust_kernel_pr_info(s: *const u8);
    fn rust_kernel_pr_err(s: *const u8);
    fn rust_kernel_pr_debug(s: *const u8);
    fn rust_kernel_pr_warn(s: *const u8);
}

pub(super) fn call_pr_info(ptr: *const u8) {
    unsafe { rust_kernel_pr_info(ptr) }
}

#[allow(dead_code)]
fn call_pr_err(ptr: *const u8) {
    unsafe { rust_kernel_pr_err(ptr) }
}

#[allow(dead_code)]
fn call_pr_debug(ptr: *const u8) {
    unsafe { rust_kernel_pr_debug(ptr) }
}

#[allow(dead_code)]
fn call_pr_warn(ptr: *const u8) {
    unsafe { rust_kernel_pr_warn(ptr) }
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

/// 将 print! 转为 pr_info 输出（仅支持字面量）
#[macro_export]
macro_rules! print {
    ($s:literal) => {
        $crate::pr_info!($s)
    };
}

/// 将 println! 转为 pr_info 输出（仅支持字面量，与 std 的 println! 一致）
#[macro_export]
macro_rules! println {
    ($s:literal) => {
        $crate::pr_info!($s)
    };
}

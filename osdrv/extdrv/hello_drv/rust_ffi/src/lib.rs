//! rust_ffi - Rust 静态库，通过 FFI 供 C 内核模块调用
//! 体积优化：无 log crate，仅用字面量 pr_info!/pr_debug!

#![no_std]

mod kernel_print;

#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! {
    loop {}
}

/// 供 C 调用的初始化，返回 0 表示成功
#[no_mangle]
pub extern "C" fn rust_hello_init() -> i32 {
    pr_info!("rust_ffi: Hello from Rust");
    pr_debug!("rust_ffi: debug message");
    0
}

/// 供 C 调用的退出
#[no_mangle]
pub extern "C" fn rust_hello_exit() {
    pr_info!("rust_ffi: Goodbye from Rust");
}

/// 供 C 调用的简单计算示例
#[no_mangle]
pub extern "C" fn rust_add(a: i32, b: i32) -> i32 {
    pr_debug!("rust_ffi: rust_add");
    a + b
}

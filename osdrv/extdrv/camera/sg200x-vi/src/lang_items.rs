//! no_std 语言项：极简 panic 处理（仅固定字符串，不拉入 core::fmt 以减小体积）

unsafe extern "C" {
    fn rust_kernel_pr_err(s: *const u8);
}

#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! {
    const MSG: &[u8] = b"sg200x_vi: panic\n\0";
    unsafe { rust_kernel_pr_err(MSG.as_ptr()) }
    loop {}
}

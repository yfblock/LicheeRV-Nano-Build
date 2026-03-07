pub fn ioremap(start: *const u8, size: usize) -> *mut u8 {
    unsafe extern "C" {
        // ioremap 在 RISC-V 内核中是宏，无导出符号，通过 rust_kernel.c 中的桥接函数调用
        fn rust_ioremap(phys_addr: *const u8, size: usize) -> *mut u8;
    }
    unsafe { rust_ioremap(start, size) }
}

pub fn iounmap(addr: *mut u8) {
    unsafe extern "C" {
        fn rust_iounmap(addr: *mut u8);
    }
    unsafe { rust_iounmap(addr) }
}

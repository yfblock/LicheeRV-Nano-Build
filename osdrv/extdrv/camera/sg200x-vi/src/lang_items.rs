//! no_std 语言项：panic 处理 + 通过 FFI 使用内核 kmalloc/kfree 的 #[global_allocator]

use core::alloc::{GlobalAlloc, Layout};

unsafe extern "C" {
    fn rust_kernel_pr_err(s: *const u8);
    fn rust_kmalloc(size: usize) -> *mut u8;
    fn rust_kfree(ptr: *mut u8);
}

#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! {
    const MSG: &[u8] = b"sg200x_vi: panic\n\0";
    unsafe { rust_kernel_pr_err(MSG.as_ptr()) }
    loop {}
}

/// 全局分配器：通过 FFI 调用 Linux 内核的 kmalloc/kfree。
/// 大对齐时多分配一段并在返回指针前 8 字节存原始指针，便于 dealloc 时 kfree 正确地址。
struct KernelAlloc;

unsafe impl GlobalAlloc for KernelAlloc {
    #[inline]
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if layout.size() == 0 {
            return core::ptr::null_mut();
        }
        let align = layout.align().max(1);
        let extra = align - 1;
        let total = 8usize
            .saturating_add(layout.size())
            .saturating_add(extra);
        let base = unsafe { rust_kmalloc(total) };
        if base.is_null() {
            return core::ptr::null_mut();
        }
        let base_usize = base as usize;
        let aligned = (base_usize + 8).wrapping_add(align - 1) & !(align - 1);
        let aligned_ptr = aligned as *mut u8;
        unsafe { *(aligned_ptr as *mut usize).sub(1) = base_usize };
        aligned_ptr
    }

    #[inline]
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        if ptr.is_null() {
            return;
        }
        let base = unsafe { *(ptr as *const usize).sub(1) as *mut u8 };
        unsafe { rust_kfree(base) };
    }
}

#[global_allocator]
static ALLOCATOR: KernelAlloc = KernelAlloc;

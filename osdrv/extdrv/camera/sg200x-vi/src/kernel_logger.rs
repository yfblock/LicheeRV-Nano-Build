//! 将 log crate 的 Level 映射到内核 pr_debug / pr_info / pr_warn / pr_err

use core::fmt;
use log::{Level, LevelFilter, Log, Metadata, Record};

unsafe extern "C" {
    fn rust_kernel_pr_debug(s: *const u8);
    fn rust_kernel_pr_info(s: *const u8);
    fn rust_kernel_pr_warn(s: *const u8);
    fn rust_kernel_pr_err(s: *const u8);
}

/// 固定大小缓冲区，实现 core::fmt::Write，用于在 no_std 下格式化 log 内容
struct LogBuf {
    buf: [u8; 384],
    pos: usize,
}

impl LogBuf {
    const fn new() -> Self {
        Self {
            buf: [0u8; 384],
            pos: 0,
        }
    }

    fn as_c_str(&mut self) -> *const u8 {
        if self.pos >= self.buf.len() {
            self.pos = self.buf.len().saturating_sub(1);
        }
        self.buf[self.pos] = 0;
        self.buf.as_ptr()
    }
}

impl fmt::Write for LogBuf {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        let b = s.as_bytes();
        let rest = self.buf.len().saturating_sub(self.pos);
        let n = b.len().min(rest.saturating_sub(1)); // 留一个给 '\0'
        if n > 0 {
            self.buf[self.pos..][..n].copy_from_slice(&b[..n]);
            self.pos += n;
        }
        if n < b.len() {
            // 截断，末尾加省略
            const ELLIPSIS: &[u8] = b"...";
            let e = ELLIPSIS.len().min(self.buf.len().saturating_sub(self.pos).saturating_sub(1));
            if e > 0 {
                self.buf[self.pos..][..e].copy_from_slice(&ELLIPSIS[..e]);
                self.pos += e;
            }
        }
        Ok(())
    }
}

/// 内核 logger：log Level -> pr_*
struct KernelLogger;

impl Log for KernelLogger {
    fn enabled(&self, _metadata: &Metadata) -> bool {
        true
    }

    fn log(&self, record: &Record) {
        let mut log_buf = LogBuf::new();
        let _ = fmt::write(&mut log_buf, *record.args());
        let ptr = log_buf.as_c_str();

        match record.level() {
            Level::Trace | Level::Debug => unsafe { rust_kernel_pr_debug(ptr) },
            Level::Info => unsafe { rust_kernel_pr_info(ptr) },
            Level::Warn => unsafe { rust_kernel_pr_warn(ptr) },
            Level::Error => unsafe { rust_kernel_pr_err(ptr) },
        }
    }

    fn flush(&self) {}
}

static KERNEL_LOGGER: KernelLogger = KernelLogger;

/// 初始化 log 前端，使 `log::debug!()` / `info!()` / `warn!()` / `error!()` 等输出到内核 pr_*。
/// 应只调用一次（例如在模块 init 时）。
///
/// * `max_level`：最大级别，例如 `LevelFilter::Debug` 会输出 debug 及以上。
pub fn init_kernel_logger(max_level: LevelFilter) {
    let _ = log::set_logger(&KERNEL_LOGGER);
    log::set_max_level(max_level);
}

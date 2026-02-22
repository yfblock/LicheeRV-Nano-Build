# hello_drv - C 内核模块 + Rust FFI（体积优化）
# 中间文件与 .ko 均在 build/ 下，源码目录不产生 .o/.ko 等
#
# 用法: make [all] | clean

PWD := $(shell pwd)
BUILD_DIR := $(PWD)/build
RUST_TARGET ?= riscv64imac-unknown-none-elf
KERNEL_DIR ?= $(PWD)/../../../linux_5.10/build/sg2002_licheervnano_sd
ARCH ?= riscv

RUST_LD ?= $(shell rustup show home 2>/dev/null)/toolchains/stable-x86_64-unknown-linux-gnu/lib/rustlib/x86_64-unknown-linux-gnu/bin/gcc-ld/ld.lld
LLVM_OBJCOPY ?= $(shell rustup show home 2>/dev/null)/toolchains/stable-x86_64-unknown-linux-gnu/lib/rustlib/x86_64-unknown-linux-gnu/bin/llvm-objcopy

.PHONY: all clean

all: $(BUILD_DIR)/hello_drv.ko

# 准备 build 目录：拷贝 C 源、头文件、Makefile 模板，再调内核构建
$(BUILD_DIR)/hello_drv.ko: $(BUILD_DIR)/hello_main.c $(BUILD_DIR)/hello_procfs.c $(BUILD_DIR)/hello_devfs.c $(BUILD_DIR)/hello_i2c.c $(BUILD_DIR)/hello_gpio.c $(BUILD_DIR)/hello_procfs.h $(BUILD_DIR)/hello_devfs.h $(BUILD_DIR)/hello_i2c.h $(BUILD_DIR)/hello_gpio.h $(BUILD_DIR)/Makefile
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) modules

$(BUILD_DIR)/Makefile: Makefile.build
	@mkdir -p $(BUILD_DIR)
	cp -f Makefile.build $(BUILD_DIR)/Makefile

$(BUILD_DIR)/hello_main.c: hello_main.c
	@mkdir -p $(BUILD_DIR)
	cp -f hello_main.c $(BUILD_DIR)/

$(BUILD_DIR)/hello_procfs.c: hello_procfs.c
	@mkdir -p $(BUILD_DIR)
	cp -f hello_procfs.c $(BUILD_DIR)/

$(BUILD_DIR)/hello_devfs.c: hello_devfs.c
	@mkdir -p $(BUILD_DIR)
	cp -f hello_devfs.c $(BUILD_DIR)/

$(BUILD_DIR)/hello_devfs.h: hello_devfs.h
	@mkdir -p $(BUILD_DIR)
	cp -f hello_devfs.h $(BUILD_DIR)/

$(BUILD_DIR)/hello_i2c.c: hello_i2c.c
	@mkdir -p $(BUILD_DIR)
	cp -f hello_i2c.c $(BUILD_DIR)/

$(BUILD_DIR)/hello_gpio.c: hello_gpio.c
	@mkdir -p $(BUILD_DIR)
	cp -f hello_gpio.c $(BUILD_DIR)/

$(BUILD_DIR)/hello_procfs.h: hello_procfs.h
	@mkdir -p $(BUILD_DIR)
	cp -f hello_procfs.h $(BUILD_DIR)/

$(BUILD_DIR)/hello_i2c.h: hello_i2c.h
	@mkdir -p $(BUILD_DIR)
	cp -f hello_i2c.h $(BUILD_DIR)/

$(BUILD_DIR)/hello_gpio.h: hello_gpio.h
	@mkdir -p $(BUILD_DIR)
	cp -f hello_gpio.h $(BUILD_DIR)/

$(BUILD_DIR)/rust_kernel.c: rust_kernel.c
	@mkdir -p $(BUILD_DIR)
	cp -f rust_kernel.c $(BUILD_DIR)/

# Rust 静态库 -> build/rust_ffi.o
$(BUILD_DIR)/rust_ffi.o:
	@mkdir -p $(BUILD_DIR)
	cd rust_ffi && RUSTFLAGS="-C debuginfo=0" cargo build --release --target $(RUST_TARGET)
	$(RUST_LD) -r -o $(BUILD_DIR)/rust_ffi.o -m elf64lriscv --whole-archive rust_ffi/target/$(RUST_TARGET)/release/librust_ffi.a
	@echo 'cmd_$@ := $(RUST_LD) -r ...' > $(BUILD_DIR)/.rust_ffi.o.cmd

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) clean 2>/dev/null || true
	rm -rf $(BUILD_DIR)
	cd rust_ffi && cargo clean 2>/dev/null || true

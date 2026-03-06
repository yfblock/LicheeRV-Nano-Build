# camera - 合并 vi 与 mipi-rx 的单一内核模块，中间文件与 camera.ko 均在源码目录
# 使用：cd osdrv/extdrv/camera && make [all] | clean
# 顶层若编本模块，请用 M=本目录（源码目录），不要用 M=.../camera/build

PWD := $(shell pwd)
KERNEL_DIR ?= $(PWD)/../../../linux_5.10/build/sg2002_licheervnano_sd
ARCH ?= riscv
RUST_TARGET ?= riscv64imac-unknown-none-elf
RUST_LD ?= $(shell rustup show home 2>/dev/null)/toolchains/stable-x86_64-unknown-linux-gnu/lib/rustlib/x86_64-unknown-linux-gnu/bin/gcc-ld/ld.lld
OBJCOPY ?= $(shell command -v llvm-objcopy 2>/dev/null || command -v objcopy 2>/dev/null)

.PHONY: all clean

all: camera.ko

# ---------- 本 Makefile 被内核 include 时使用的 Kbuild（M=本目录时 $(src)=本目录） ----------
obj-m := camera.o
camera-objs := camera_init.o \
		rust_kernel.o \
		sg200x_vi.o \
		vi/common/vi_core.o \
		vi/chip/mars/vi.o \
		vi/chip/mars/vi_ext.o \
		vi/chip/mars/vi_sdk_layer.o \
		vi/chip/mars/vi_isp_buf_ctrl.o \
		vi/chip/mars/vi_raw_dump.o \
		vi/chip/mars/vip/vi_drv.o \
		vi/chip/mars/vip/vi_subsys_ctrl.o \
		vi/chip/mars/vip/vi_fe_ip_ctrl.o \
		vi/chip/mars/vip/vi_be_ip_ctrl.o \
		vi/chip/mars/vip/vi_raw_ip_ctrl.o \
		vi/chip/mars/vip/vi_rgb_ip_ctrl.o \
		vi/chip/mars/vip/vi_yuv_ip_ctrl.o \
		vi/chip/mars/vip/vi_tun_ip_ctrl.o \
		mipi-rx/cif.o \
		mipi-rx/drv/cif_drv.o

ccflags-y += -I$(src)/vi/chip/mars -I$(src)/vi/common
ccflags-y += -I$(src)/mipi-rx -I$(src)/mipi-rx/drv -I$(src)/mipi-rx/drv/inc
# interdrv 头文件（linux/cvi_defines.h 等）：$(src) 与 $(srctree)/../osdrv 双路径兼容
ccflags-y += -I$(src)/../../interdrv/v2/include/common/uapi
ccflags-y += -I$(src)/../../interdrv/v2/include/chip/mars/uapi
ccflags-y += -I$(srctree)/../osdrv/interdrv/v2/include/common/uapi
ccflags-y += -I$(srctree)/../osdrv/interdrv/v2/include/chip/mars/uapi
ccflags-y += -I$(srctree)/../osdrv/interdrv/v2/include/common/kapi
ccflags-y += -I$(srctree)/../osdrv/interdrv/v2/include
ccflags-y += -I$(srctree)/../osdrv/interdrv/v2/base
ccflags-y += -I$(srctree)/../osdrv/interdrv/v2/base/chip/mars
ccflags-y += -I$(srctree)/../osdrv/interdrv/v2/sys/common
ccflags-y += -I$(srctree)/../osdrv/interdrv/v2/sys/common/uapi
ccflags-y += -I$(src)/../../interdrv/v2/include/common/kapi
ccflags-y += -I$(src)/../../interdrv/v2/include
ccflags-y += -I$(src)/../../interdrv/v2/base
ccflags-y += -I$(src)/../../interdrv/v2/base/chip/mars
ccflags-y += -I$(src)/../../interdrv/v2/sys/common
ccflags-y += -I$(src)/../../interdrv/v2/sys/common/uapi
ccflags-y += -I$(srctree)/drivers/tee
ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek
ccflags-y += -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare -Wno-error=unused-variable

# 依赖 base/sys 导出的符号（modpost 用）；本目录 make 时由下方规则传入绝对路径
KBUILD_EXTRA_SYMBOLS ?= $(src)/../../interdrv/v2/base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../../interdrv/v2/sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../../interdrv/v2/Module.symvers

# ---------- 本目录 make all 时：先编 sg200x_vi.o，再调内核在源码目录编出 camera.ko ----------
# 显式传入 Module.symvers 绝对路径，避免 -C build 时 modpost 解析失败
camera.ko: sg200x_vi.o
	$(MAKE) ARCH=$(ARCH) LD=$(RUST_LD) -C $(KERNEL_DIR) M=$(PWD) modules \
		KBUILD_EXTRA_SYMBOLS="$(PWD)/../../interdrv/v2/base/Module.symvers $(PWD)/../../interdrv/v2/sys/Module.symvers $(PWD)/../../interdrv/v2/Module.symvers"
	@STRIP="$(strip $(STRIP))"; [ -z "$$STRIP" ] && STRIP=$$(command -v strip 2>/dev/null); \
	[ -n "$$STRIP" ] && [ -x "$$STRIP" ] && $$STRIP -g $(PWD)/camera.ko 2>/dev/null || true

RUSTFLAGS_SIZE := -C debuginfo=0 -C link-dead-code=false -C overflow-checks=off
RUST_DEPS := sg200x-vi/Cargo.toml $(wildcard sg200x-vi/src/*.rs) \
	$(wildcard sg200x-vi/.cargo/config.toml) $(wildcard sg200x-vi/rust-toolchain.toml)

sg200x_vi.o: $(RUST_DEPS)
	cd sg200x-vi && RUSTFLAGS="$(RUSTFLAGS_SIZE)" cargo build --release --target $(RUST_TARGET)
	$(RUST_LD) -r -o $(PWD)/sg200x_vi.o -m elf64lriscv --whole-archive sg200x-vi/target/$(RUST_TARGET)/release/libsg200x_vi.a
	@OC="$(OBJCOPY)"; [ -z "$$OC" ] && OC=$$(command -v llvm-objcopy 2>/dev/null || command -v objcopy 2>/dev/null); \
	if [ -n "$$OC" ] && [ -x "$$OC" ]; then \
		TMP="$(PWD)/.sg200x_vi.o.stripped"; IN="$(PWD)/sg200x_vi.o"; \
		for sec in .llvmbc .llvmcmd .eh_frame .eh_frame_hdr .comment \
			.rela.debug_frame .rela.debug_info .rela.debug_aranges .rela.debug_ranges \
			.rela.debug_line .rela.debug_loclists .rela.debug_rnglists \
			.debug_frame .debug_info .debug_abbrev .debug_aranges .debug_ranges \
			.debug_str .debug_line .debug_loclists .debug_rnglists .debug_line_str; do \
			$$OC --remove-section=$$sec "$$IN" "$$TMP" 2>/dev/null && mv "$$TMP" "$$IN"; \
		done; \
	else \
		echo "WARNING: OBJCOPY not set, sg200x_vi.o kept with debug/LLVM sections"; \
	fi

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) clean 2>/dev/null || true
	rm -f $(PWD)/sg200x_vi.o $(PWD)/camera.ko $(PWD)/.sg200x_vi.o.cmd
	cd sg200x-vi && cargo clean 2>/dev/null || true

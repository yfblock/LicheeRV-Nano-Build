# camera - 合并 test-vi 与 test-mipi-rx 的单一内核模块
# 子目录：vi/、mipi-rx/、sg200x-vi/（Rust FFI 静态库），所有生成文件在 build/ 下。
#
# 使用：cd osdrv/extdrv/camera && make [all] | clean
# 必须包含 sg200x-vi（Rust）；模块链接使用 LD=$(RUST_LD) 以兼容 Rust 生成的 .o。

PWD := $(shell pwd)
BUILD_DIR := $(PWD)/build
KERNEL_DIR ?= $(PWD)/../../../linux_5.10/build/sg2002_licheervnano_sd
ARCH ?= riscv
RUST_TARGET ?= riscv64imac-unknown-none-elf
RUST_LD ?= $(shell rustup show home 2>/dev/null)/toolchains/stable-x86_64-unknown-linux-gnu/lib/rustlib/x86_64-unknown-linux-gnu/bin/gcc-ld/ld.lld
# 移除 .llvmbc/.llvmcmd 用：优先 PATH 中的 llvm-objcopy 或 objcopy，可设 OBJCOPY= 覆盖
OBJCOPY ?= $(shell command -v llvm-objcopy 2>/dev/null || command -v objcopy 2>/dev/null)

.PHONY: all clean

all: $(BUILD_DIR)/camera.ko

# 构建 camera.ko 所需：build/Makefile + build/vi/* + build/mipi-rx/*
VI_SRCS := vi/common/vi_core.c vi/common/vi_core.h vi/common/vi_ext.h vi/common/vi_common.h \
	vi/chip/mars/vi.c vi/chip/mars/vi.h vi/chip/mars/vi_ext.c vi/chip/mars/vi_sdk_layer.c vi/chip/mars/vi_sdk_layer.h \
	vi/chip/mars/vi_isp_buf_ctrl.c vi/chip/mars/vi_isp_buf_ctrl.h \
	vi/chip/mars/vi_raw_dump.c vi/chip/mars/vi_raw_dump.h \
	vi/chip/mars/vi_defines.h vi/chip/mars/vi_interfaces.h \
	vi/chip/mars/vip/vi_drv.c vi/chip/mars/vip/vi_drv.h \
	vi/chip/mars/vip/vi_subsys_ctrl.c vi/chip/mars/vip/vi_fe_ip_ctrl.c \
	vi/chip/mars/vip/vi_be_ip_ctrl.c vi/chip/mars/vip/vi_raw_ip_ctrl.c \
	vi/chip/mars/vip/vi_rgb_ip_ctrl.c vi/chip/mars/vip/vi_yuv_ip_ctrl.c vi/chip/mars/vip/vi_tun_ip_ctrl.c

MIPI_SRCS := mipi-rx/chip/mars/cif.c mipi-rx/chip/mars/cif.h \
	mipi-rx/chip/mars/drv/cif_drv.c mipi-rx/chip/mars/drv/cif_drv.h mipi-rx/chip/mars/drv/reg.h \
	mipi-rx/chip/mars/drv/inc/cif_reg.h mipi-rx/chip/mars/drv/inc/reg_fields_csi_wrap.h \
	mipi-rx/chip/mars/drv/inc/reg_fields_csi_mac.h mipi-rx/chip/mars/drv/inc/reg_blocks_csi_wrap.h \
	mipi-rx/chip/mars/drv/inc/reg_blocks_csi_mac.h

$(BUILD_DIR)/camera.ko: $(BUILD_DIR)/Makefile $(BUILD_DIR)/camera_init.c $(BUILD_DIR)/rust_kernel.c $(addprefix $(BUILD_DIR)/,$(VI_SRCS)) $(addprefix $(BUILD_DIR)/,$(MIPI_SRCS)) $(BUILD_DIR)/sg200x_vi.o
	$(MAKE) ARCH=$(ARCH) LD=$(RUST_LD) -C $(KERNEL_DIR) M=$(BUILD_DIR) modules
	@STRIP="$(strip $(STRIP))"; [ -z "$$STRIP" ] && STRIP=$$(command -v strip 2>/dev/null); \
	[ -n "$$STRIP" ] && [ -x "$$STRIP" ] && $$STRIP -g $(BUILD_DIR)/camera.ko 2>/dev/null || true

# Rust FFI：sg200x-vi 静态库 -> build/sg200x_vi.o（ld -r 转为可链接的 .o）
# sg200x_vi.o strip：一次性移除 LLVM/调试/异常帧等非必要段，显著减体积
# 体积：无 debug、无死代码、无溢出检查
RUSTFLAGS_SIZE := -C debuginfo=0 -C link-dead-code=false -C overflow-checks=off

$(BUILD_DIR)/sg200x_vi.o:
	@mkdir -p $(BUILD_DIR)
	cd sg200x-vi && RUSTFLAGS="$(RUSTFLAGS_SIZE)" cargo build --release --target $(RUST_TARGET)
	$(RUST_LD) -r -o $(BUILD_DIR)/sg200x_vi.o -m elf64lriscv --whole-archive sg200x-vi/target/$(RUST_TARGET)/release/libsg200x_vi.a
	@OC="$(OBJCOPY)"; [ -z "$$OC" ] && OC="$$(command -v llvm-objcopy 2>/dev/null || command -v objcopy 2>/dev/null)"; \
	if [ -n "$$OC" ] && [ -x "$$OC" ]; then \
		TMP="$(BUILD_DIR)/sg200x_vi.o.stripped"; IN="$(BUILD_DIR)/sg200x_vi.o"; \
		for sec in .llvmbc .llvmcmd .eh_frame .eh_frame_hdr .comment \
			.rela.debug_frame .rela.debug_info .rela.debug_aranges .rela.debug_ranges \
			.rela.debug_line .rela.debug_loclists .rela.debug_rnglists \
			.debug_frame .debug_info .debug_abbrev .debug_aranges .debug_ranges \
			.debug_str .debug_line .debug_loclists .debug_rnglists .debug_line_str; do \
			$$OC --remove-section=$$sec "$$IN" "$$TMP" 2>/dev/null && mv "$$TMP" "$$IN"; \
		done; \
	else \
		echo "WARNING: OBJCOPY not set and llvm-objcopy/objcopy not in PATH"; \
	fi
	@echo 'cmd_$@ := $(RUST_LD) -r ...' > $(BUILD_DIR)/.sg200x_vi.o.cmd

$(BUILD_DIR)/Makefile: Makefile.build
	@mkdir -p $(BUILD_DIR)
	cp -f Makefile.build $(BUILD_DIR)/Makefile

# 从 vi/ 同步到 build/vi/
$(BUILD_DIR)/vi/%: vi/%
	@mkdir -p $(dir $@)
	cp -f $< $@

# 从 mipi-rx/ 同步到 build/mipi-rx/
$(BUILD_DIR)/mipi-rx/%: mipi-rx/%
	@mkdir -p $(dir $@)
	cp -f $< $@

# 顶层入口源文件
$(BUILD_DIR)/camera_init.c: camera_init.c
	@mkdir -p $(BUILD_DIR)
	cp -f $< $@

$(BUILD_DIR)/rust_kernel.c: rust_kernel.c
	@mkdir -p $(BUILD_DIR)
	cp -f $< $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) clean 2>/dev/null || true
	rm -rf $(BUILD_DIR)
	cd sg200x-vi && cargo clean 2>/dev/null || true

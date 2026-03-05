# test-vi - 从 interdrv/v2/vi 复制出的独立 VI 模块
# 源码放在本目录（common/、chip/mars/...），所有生成文件放在 build/ 下。
#
# 使用方法（在仓库根目录已准备好内核构建目录时）：
#   cd osdrv/extdrv/test-vi
#   make [all] | clean

PWD := $(shell pwd)
BUILD_DIR := $(PWD)/build
KERNEL_DIR ?= $(PWD)/../../../linux_5.10/build/sg2002_licheervnano_sd
ARCH ?= riscv

.PHONY: all clean

all: $(BUILD_DIR)/test_vi.ko

# 调用内核 kbuild，在 build/ 目录下构建模块
$(BUILD_DIR)/test_vi.ko: $(BUILD_DIR)/Makefile \
	$(BUILD_DIR)/common/vi_core.c \
	$(BUILD_DIR)/common/vi_core.h \
	$(BUILD_DIR)/common/vi_ext.h \
	$(BUILD_DIR)/common/vi_common.h \
	$(BUILD_DIR)/chip/mars/vi.c \
	$(BUILD_DIR)/chip/mars/vi.h \
	$(BUILD_DIR)/chip/mars/vi_ext.c \
	$(BUILD_DIR)/chip/mars/vi_sdk_layer.c \
	$(BUILD_DIR)/chip/mars/vi_sdk_layer.h \
	$(BUILD_DIR)/chip/mars/vi_isp_buf_ctrl.c \
	$(BUILD_DIR)/chip/mars/vi_isp_buf_ctrl.h \
	$(BUILD_DIR)/chip/mars/vi_raw_dump.c \
	$(BUILD_DIR)/chip/mars/vi_raw_dump.h \
	$(BUILD_DIR)/chip/mars/vi_defines.h \
	$(BUILD_DIR)/chip/mars/vi_interfaces.h \
	$(BUILD_DIR)/chip/mars/vip/vi_drv.c \
	$(BUILD_DIR)/chip/mars/vip/vi_drv.h \
	$(BUILD_DIR)/chip/mars/vip/vi_subsys_ctrl.c \
	$(BUILD_DIR)/chip/mars/vip/vi_fe_ip_ctrl.c \
	$(BUILD_DIR)/chip/mars/vip/vi_be_ip_ctrl.c \
	$(BUILD_DIR)/chip/mars/vip/vi_raw_ip_ctrl.c \
	$(BUILD_DIR)/chip/mars/vip/vi_rgb_ip_ctrl.c \
	$(BUILD_DIR)/chip/mars/vip/vi_yuv_ip_ctrl.c \
	$(BUILD_DIR)/chip/mars/vip/vi_tun_ip_ctrl.c
	bear -- $(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) modules

# 将 Kbuild 模板复制到 build/ 供内核使用
$(BUILD_DIR)/Makefile: Makefile.build
	@mkdir -p $(BUILD_DIR)
	cp -f Makefile.build $(BUILD_DIR)/Makefile

# 通用规则：把源文件/头文件同步到 build/，保持目录层级
$(BUILD_DIR)/%: %
	@mkdir -p $(dir $@)
	cp -f $< $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) clean 2>/dev/null || true
	rm -rf $(BUILD_DIR)


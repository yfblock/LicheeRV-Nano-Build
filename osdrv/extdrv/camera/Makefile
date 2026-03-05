# camera - 合并 VI 与 MIPI RX 的单一内核模块，生成 camera.ko
# 源码：vi/、mipi-rx/；生成文件统一放在 build/ 下。
#
# 使用：cd osdrv/extdrv/camera && make [all] | clean

PWD := $(shell pwd)
BUILD_DIR := $(PWD)/build
KERNEL_DIR ?= $(PWD)/../../../linux_5.10/build/sg2002_licheervnano_sd
ARCH ?= riscv

.PHONY: all clean

all: $(BUILD_DIR)/camera.ko

# 构建依赖：Kbuild Makefile + 全部源码（复制到 build/vi、build/mipi-rx）
VI_SRCS := \
	vi/common/vi_core.c vi/common/vi_core.h vi/common/vi_ext.h vi/common/vi_common.h \
	vi/chip/mars/vi.c vi/chip/mars/vi.h vi/chip/mars/vi_ext.c \
	vi/chip/mars/vi_sdk_layer.c vi/chip/mars/vi_sdk_layer.h \
	vi/chip/mars/vi_isp_buf_ctrl.c vi/chip/mars/vi_isp_buf_ctrl.h \
	vi/chip/mars/vi_raw_dump.c vi/chip/mars/vi_raw_dump.h \
	vi/chip/mars/vi_defines.h vi/chip/mars/vi_interfaces.h \
	vi/chip/mars/vip/vi_drv.c vi/chip/mars/vip/vi_drv.h \
	vi/chip/mars/vip/vi_subsys_ctrl.c vi/chip/mars/vip/vi_fe_ip_ctrl.c \
	vi/chip/mars/vip/vi_be_ip_ctrl.c vi/chip/mars/vip/vi_raw_ip_ctrl.c \
	vi/chip/mars/vip/vi_rgb_ip_ctrl.c vi/chip/mars/vip/vi_yuv_ip_ctrl.c \
	vi/chip/mars/vip/vi_tun_ip_ctrl.c

MIPI_SRCS := \
	mipi-rx/cif.c mipi-rx/cif.h \
	mipi-rx/drv/cif_drv.c mipi-rx/drv/cif_drv.h \
	mipi-rx/drv/reg.h \
	mipi-rx/drv/inc/cif_reg.h \
	mipi-rx/drv/inc/reg_fields_csi_wrap.h \
	mipi-rx/drv/inc/reg_fields_csi_mac.h \
	mipi-rx/drv/inc/reg_blocks_csi_wrap.h \
	mipi-rx/drv/inc/reg_blocks_csi_mac.h

$(BUILD_DIR)/camera.ko: $(BUILD_DIR)/Makefile $(addprefix $(BUILD_DIR)/,$(VI_SRCS) $(MIPI_SRCS))
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) modules

# 将 Kbuild 模板复制到 build/
$(BUILD_DIR)/Makefile: Makefile.build
	@mkdir -p $(BUILD_DIR)
	cp -f Makefile.build $(BUILD_DIR)/Makefile

# 源文件同步到 build/vi、build/mipi-rx，保持目录层级
$(BUILD_DIR)/vi/%: vi/%
	@mkdir -p $(dir $@)
	cp -f $< $@

$(BUILD_DIR)/mipi-rx/%: mipi-rx/%
	@mkdir -p $(dir $@)
	cp -f $< $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) clean 2>/dev/null || true
	rm -rf $(BUILD_DIR)

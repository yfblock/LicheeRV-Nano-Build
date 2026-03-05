# camera - 合并 test-vi 与 test-mipi-rx 的单一内核模块
# 子目录：vi/、mipi-rx/（无各自 Makefile），所有生成文件在 build/ 下。
#
# 使用：cd osdrv/extdrv/camera && make [all] | clean

PWD := $(shell pwd)
BUILD_DIR := $(PWD)/build
KERNEL_DIR ?= $(PWD)/../../../linux_5.10/build/sg2002_licheervnano_sd
ARCH ?= riscv

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

$(BUILD_DIR)/camera.ko: $(BUILD_DIR)/Makefile $(BUILD_DIR)/camera_init.c $(addprefix $(BUILD_DIR)/,$(VI_SRCS)) $(addprefix $(BUILD_DIR)/,$(MIPI_SRCS))
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) modules

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

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) clean 2>/dev/null || true
	rm -rf $(BUILD_DIR)

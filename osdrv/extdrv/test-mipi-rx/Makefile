# test-mipi-rx - 从 interdrv/v2/cif 复制出的独立 MIPI RX 模块
# 源码放在本目录（chip/mars/...），所有生成文件放在 build/ 下。
#
# 使用方法（在仓库根目录已准备好内核构建目录时）：
#   cd osdrv/extdrv/test-mipi-rx
#   make [all] | clean

PWD := $(shell pwd)
BUILD_DIR := $(PWD)/build
KERNEL_DIR ?= $(PWD)/../../../linux_5.10/build/sg2002_licheervnano_sd
ARCH ?= riscv

.PHONY: all clean

all: $(BUILD_DIR)/test_mipi_rx.ko

# 调用内核 kbuild，在 build/ 目录下构建模块
$(BUILD_DIR)/test_mipi_rx.ko: $(BUILD_DIR)/Makefile $(BUILD_DIR)/chip/mars/cif.c $(BUILD_DIR)/chip/mars/cif.h $(BUILD_DIR)/chip/mars/drv/cif_drv.c $(BUILD_DIR)/chip/mars/drv/cif_drv.h $(BUILD_DIR)/chip/mars/drv/reg.h $(BUILD_DIR)/chip/mars/drv/inc/cif_reg.h $(BUILD_DIR)/chip/mars/drv/inc/reg_fields_csi_wrap.h $(BUILD_DIR)/chip/mars/drv/inc/reg_fields_csi_mac.h $(BUILD_DIR)/chip/mars/drv/inc/reg_blocks_csi_wrap.h $(BUILD_DIR)/chip/mars/drv/inc/reg_blocks_csi_mac.h
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) modules

# 将 Kbuild 模板复制到 build/ 供内核使用
$(BUILD_DIR)/Makefile: Makefile.build
	@mkdir -p $(BUILD_DIR)
	cp -f Makefile.build $(BUILD_DIR)/Makefile

# 通用规则：把源文件同步到 build/，保持 chip/mars/... 目录层级
$(BUILD_DIR)/%: %
	@mkdir -p $(dir $@)
	cp -f $< $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(BUILD_DIR) clean 2>/dev/null || true
	rm -rf $(BUILD_DIR)


# esp_drv - 内核模块，生成 esp_drv.ko
# 使用：cd osdrv/extdrv/esp_drv && make [all] | clean
#
# 若 insmod 报 Unknown symbol (get_device/spi_busnum_to_master 等)：
# 1) 必须用与设备上运行的内核同一套源码、同一配置编译本模块（KERNEL_DIR 指向该内核的 build 目录）
# 2) 若 SPI 为模块，先加载：modprobe spi_dw_mmio，再 insmod esp_drv.ko（或使用 modprobe esp_drv，会按 MODULE_SOFTDEP 先加载 spi_dw_mmio）

PWD := $(shell pwd)
KERNEL_DIR ?= $(PWD)/../../../linux_5.10/build/sg2002_licheervnano_sd
ARCH ?= riscv

# Kbuild：多文件组成同一模块（模块名不能与任何组件 .o 同名）
obj-m := esp_drv.o
esp_drv-objs := esp_main.o esp_spi.o esp_net.o

.PHONY: all clean

all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) clean 2>/dev/null || true
	rm -f esp_drv.ko Module.symvers modules.order .*.cmd *.mod *.mod.c *.o esp_spi.o

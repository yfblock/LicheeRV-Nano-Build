/*
 * hello_devfs.c - /dev/hello_drv 字符设备（devfs）+ ioctl
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <asm/ioctl.h>

// #define HELLO_DEVFS_NAME "hello_drv"
#define HELLO_DEVFS_NAME "cvi-mipi-rx"

/* ioctl 魔数与命令（与 hello_devfs_ioctl.h 一致，用户态可共用） */
#define HELLO_IOCTL_MAGIC  'H'
#define HELLO_IOCTL_GET_VERSION _IOR(HELLO_IOCTL_MAGIC, 0, unsigned int)
#define HELLO_IOCTL_SET_VALUE   _IOW(HELLO_IOCTL_MAGIC, 1, unsigned int)
#define HELLO_IOCTL_GET_VALUE   _IOR(HELLO_IOCTL_MAGIC, 2, unsigned int)

/*
 * cvi-mipi-rx 兼容：ioctl 魔数/命令与 osdrv/interdrv/v2/include/chip/mars/uapi/linux/cif_uapi.h
 * 一致；真实实现在 osdrv/interdrv/v2/cif/chip/mars/cif.c 的 cif_ioctl -> _cif_ioctl()。
 */
#define CVI_MIPI_IOC_MAGIC      'm'
/* 0x01: arg = 用户态 struct combo_dev_attr_s。CIF 中 copy_from_user 后调用 cif_set_dev_attr(dev, &attr)：设置 MIPI/LVDS/BT 等输入模式与参数。与 cif_uapi.h 的 _IOW('m', 0x01, struct combo_dev_attr_s) 同值。 */
#define CVI_MIPI_SET_DEV_ATTR   1086352641
/* 0x05: arg = 用户态 unsigned int devno。CIF 中调用 cif_reset_snsr_gpio(dev, devno, 1)：
 *       拉高/拉低 sensor 复位 GPIO（link->snsr_rst_pin），极性由 snsr_rst_pol 决定，用于断言 sensor 复位。 */
#define CVI_MIPI_RESET_SENSOR   _IOW(CVI_MIPI_IOC_MAGIC, 0x05, unsigned int)
/* 0x06: arg = 用户态 unsigned int devno。CIF 中调用 cif_reset_snsr_gpio(dev, devno, 0)：解除 sensor 复位 GPIO。 */
#define CVI_MIPI_UNRESET_SENSOR _IOW(CVI_MIPI_IOC_MAGIC, 0x06, unsigned int)
/* 0x07: arg = 用户态 unsigned int devno。CIF 中调用 cif_reset_mipi(dev, devno)：
 *       屏蔽 CSI 中断 → 拉 MIPI PHY/APB reset → 软复位 VIP CSI MAC(devno 0/1/2) → cif_reset_param(link)。 */
#define CVI_MIPI_RESET_MIPI     _IOW(CVI_MIPI_IOC_MAGIC, 0x07, unsigned int)
/* 0x10: arg = 用户态 unsigned int devno。CIF 中调用 cif_enable_snsr_clk(dev, devno, 1)：开启 sensor MCLK。 */
#define CVI_MIPI_ENABLE_SENSOR_CLOCK  _IOW(CVI_MIPI_IOC_MAGIC, 0x10, unsigned int)

static unsigned int hello_ioctl_value;

static dev_t hello_devt;
static struct cdev hello_cdev;
static struct class *hello_class;
static struct device *hello_device;

static int hello_devfs_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int hello_devfs_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t hello_devfs_read(struct file *file, char __user *buf,
			       size_t count, loff_t *ppos)
{
	const char *msg = "hello_drv\n";
	size_t len = strlen(msg);

	if (*ppos >= len)
		return 0;
	if (count > len - *ppos)
		count = len - *ppos;
	if (copy_to_user(buf, msg + *ppos, count))
		return -EFAULT;
	*ppos += count;
	return count;
}

static ssize_t hello_devfs_write(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	/* 忽略写入，仅消耗计数 */
	return count;
}

static long hello_devfs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int val;
	void __user *uptr = (void __user *)arg;

	pr_info("hello_devfs_ioctl: cmd = %d, arg = %ld\n", cmd, arg);
	switch (cmd) {
	case HELLO_IOCTL_GET_VERSION:
		val = 0x00010000; /* 1.0 */
		if (copy_to_user(uptr, &val, sizeof(val)))
			return -EFAULT;
		return 0;
	case HELLO_IOCTL_SET_VALUE:
		if (copy_from_user(&val, uptr, sizeof(val)))
			return -EFAULT;
		hello_ioctl_value = val;
		return 0;
	case HELLO_IOCTL_GET_VALUE:
		val = hello_ioctl_value;
		if (copy_to_user(uptr, &val, sizeof(val)))
			return -EFAULT;
		return 0;
	case CVI_MIPI_RESET_SENSOR:
		/* 对应 cif.c _cif_ioctl() -> cif_reset_snsr_gpio(dev, devno, 1)：断言 sensor 复位 GPIO */
		if (copy_from_user(&val, uptr, sizeof(val)))
			return -EFAULT;
		pr_info("hello_devfs_ioctl: CVI_MIPI_RESET_SENSOR devno = %u\n", val);
		/* 桩实现：不驱动 GPIO，仅记录 */
		return 0;
	case CVI_MIPI_UNRESET_SENSOR:
		/* 对应 cif.c _cif_ioctl() -> cif_reset_snsr_gpio(dev, devno, 0)：解除 sensor 复位 */
		if (copy_from_user(&val, uptr, sizeof(val)))
			return -EFAULT;
		pr_info("hello_devfs_ioctl: CVI_MIPI_UNRESET_SENSOR devno = %u\n", val);
		return 0;
	case CVI_MIPI_RESET_MIPI:
		/* 对应 cif.c _cif_ioctl() -> cif_reset_mipi(dev, devno)：PHY+APB+CSI MAC 复位并重置 link 参数 */
		if (copy_from_user(&val, uptr, sizeof(val)))
			return -EFAULT;
		pr_info("hello_devfs_ioctl: CVI_MIPI_RESET_MIPI devno = %u\n", val);
		/* 桩实现：不操作 VIP/PHY，仅记录 */
		return 0;
	case CVI_MIPI_ENABLE_SENSOR_CLOCK:
		/* 对应 cif.c _cif_ioctl() -> cif_enable_snsr_clk(dev, devno, 1)：开启 sensor 时钟 */
		if (copy_from_user(&val, uptr, sizeof(val)))
			return -EFAULT;
		pr_info("hello_devfs_ioctl: CVI_MIPI_ENABLE_SENSOR_CLOCK devno = %u\n", val);
		return 0;
	case CVI_MIPI_SET_DEV_ATTR: {
		/* 对应 cif.c _cif_ioctl() -> copy_from_user(combo_dev_attr_s) -> cif_set_dev_attr(dev, &attr) */
		unsigned int sz = _IOC_SIZE(cmd);
		static char attr_buf[4096];

		if (sz == 0 || sz > sizeof(attr_buf))
			return -EINVAL;
		if (copy_from_user(attr_buf, uptr, sz))
			return -EFAULT;
		pr_info("hello_devfs_ioctl: CVI_MIPI_SET_DEV_ATTR len = %u\n", sz);
		return 0;
	}
	default:
		return -ENOTTY;
	}
}

static const struct file_operations hello_devfs_fops = {
	.owner		= THIS_MODULE,
	.open		= hello_devfs_open,
	.release	= hello_devfs_release,
	.read		= hello_devfs_read,
	.write		= hello_devfs_write,
	.unlocked_ioctl = hello_devfs_ioctl,
};

void hello_devfs_create(void)
{
	int ret;

	ret = alloc_chrdev_region(&hello_devt, 0, 1, HELLO_DEVFS_NAME);
	if (ret) {
		pr_emerg("hello_drv: alloc_chrdev_region failed %d\n", ret);
		return;
	}

	cdev_init(&hello_cdev, &hello_devfs_fops);
	hello_cdev.owner = THIS_MODULE;
	ret = cdev_add(&hello_cdev, hello_devt, 1);
	if (ret) {
		pr_emerg("hello_drv: cdev_add failed %d\n", ret);
		goto err_cdev;
	}

	hello_class = class_create(THIS_MODULE, HELLO_DEVFS_NAME);
	if (IS_ERR(hello_class)) {
		ret = PTR_ERR(hello_class);
		pr_emerg("hello_drv: class_create failed %d\n", ret);
		goto err_class;
	}

	hello_device = device_create(hello_class, NULL, hello_devt, NULL, HELLO_DEVFS_NAME);
	if (IS_ERR(hello_device)) {
		ret = PTR_ERR(hello_device);
		pr_emerg("hello_drv: device_create failed %d\n", ret);
		goto err_device;
	}

	pr_emerg("hello_drv: /dev/%s created (major %u minor %u)\n",
		 HELLO_DEVFS_NAME, MAJOR(hello_devt), MINOR(hello_devt));
	return;

err_device:
	class_destroy(hello_class);
	hello_class = NULL;
err_class:
	cdev_del(&hello_cdev);
err_cdev:
	unregister_chrdev_region(hello_devt, 1);
}

void hello_devfs_destroy(void)
{
	if (hello_device && !IS_ERR(hello_device)) {
		device_destroy(hello_class, hello_devt);
		hello_device = NULL;
	}
	if (hello_class) {
		class_destroy(hello_class);
		hello_class = NULL;
	}
	cdev_del(&hello_cdev);
	unregister_chrdev_region(hello_devt, 1);
	pr_emerg("hello_drv: /dev/%s removed\n", HELLO_DEVFS_NAME);
}

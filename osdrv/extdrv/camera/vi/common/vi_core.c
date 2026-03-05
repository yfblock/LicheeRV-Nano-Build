#include <vi_core.h>
#include <base_cb.h>

/* 与 vi_interfaces.h 中 extern 声明对应，唯一定义放在本文件 */
const char * const clk_sys_name[] = {
	"clk_sys_0", "clk_sys_1", "clk_sys_2", "clk_sys_3"
};
const char * const clk_isp_name[] = {
	"clk_axi", "clk_csi_be", "clk_raw", "clk_isp_top"
};
const char * const clk_mac_name[] = {
	"clk_csi_mac0", "clk_csi_mac1", "clk_csi_mac2"
};

#define CVI_VI_IRQ_NAME            "isp"
#define CVI_VI_CLASS_NAME          "cvi-vi"
#define CVI_VI_DEV_NAME            "cvi-vi"

static long vi_core_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	return vi_ioctl(filp, cmd, arg);
}

static int vi_core_open(struct inode *inode, struct file *filp)
{
	return vi_open(inode, filp);
}

static int vi_core_release(struct inode *inode, struct file *filp)
{
	return vi_release(inode, filp);
}

static int vi_core_mmap(struct file *filp, struct vm_area_struct *vm)
{
	return vi_mmap(filp, vm);
}

static unsigned int vi_core_poll(struct file *filp, struct poll_table_struct *wait)
{
	return vi_poll(filp, wait);
}

const struct file_operations vi_fops = {
	.owner = THIS_MODULE,
	.open = vi_core_open,
	.unlocked_ioctl = vi_core_ioctl,
	.release = vi_core_release,
	.mmap = vi_core_mmap,
	.poll = vi_core_poll,
};

static int vi_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_VI);
}

static irqreturn_t vi_core_isr(int irq, void *priv)
{
	struct cvi_vi_dev *vdev = priv;

	vi_irq_handler(vdev);

	return IRQ_HANDLED;
}

static int vi_core_register_cdev(struct cvi_vi_dev *dev)
{
	struct device *dev_t;
	int err = 0;

	dev->vi_class = class_create(THIS_MODULE, CVI_VI_CLASS_NAME);
	if (IS_ERR(dev->vi_class)) {
		dev_err(dev->dev, "create class failed\n");
		return PTR_ERR(dev->vi_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&dev->cdev_id, 0, 1, CVI_VI_DEV_NAME)) < 0) {
		err = -EBUSY;
		dev_err(dev->dev, "allocate chrdev failed\n");
		return err;
	}

	/* initialize the device structure and register the device with the kernel */
	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &vi_fops);

	if ((cdev_add(&dev->cdev, dev->cdev_id, 1)) < 0) {
		err = -EBUSY;
		dev_err(dev->dev, "add chrdev failed\n");
		return err;
	}

	dev_t = device_create(dev->vi_class, dev->dev, dev->cdev_id, NULL, "%s", CVI_VI_DEV_NAME);
	if (IS_ERR(dev_t)) {
		dev_err(dev->dev, "device create failed error code(%ld)\n", PTR_ERR(dev_t));
		err = PTR_ERR(dev_t);
		return err;
	}

	return err;
}

static int vi_core_clk_init(struct platform_device *pdev)
{
	struct cvi_vi_dev *dev;
	u8 i = 0;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_vi drvdata\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(clk_sys_name); ++i) {
		dev->clk_sys[i] = devm_clk_get(&pdev->dev, clk_sys_name[i]);
		if (IS_ERR(dev->clk_sys[i])) {
			dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_sys_name[i]);
			return PTR_ERR(dev->clk_sys[i]);
		}
	}

	for (i = 0; i < ARRAY_SIZE(clk_isp_name); ++i) {
		dev->clk_isp[i] = devm_clk_get(&pdev->dev, clk_isp_name[i]);
		if (IS_ERR(dev->clk_isp[i])) {
			dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_isp_name[i]);
			return PTR_ERR(dev->clk_isp[i]);
		}
	}

	for (i = 0; i < ARRAY_SIZE(clk_mac_name); ++i) {
		dev->clk_mac[i] = devm_clk_get(&pdev->dev, clk_mac_name[i]);
		if (IS_ERR(dev->clk_mac[i])) {
			dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_mac_name[i]);
			return PTR_ERR(dev->clk_mac[i]);
		}
	}

	return 0;
}

static int vi_core_probe(struct platform_device *pdev)
{
	struct cvi_vi_dev *dev;
	struct resource *res;
	int ret = 0;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, dev);

	/* IP register base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->reg_base = devm_ioremap_resource(&pdev->dev, res);
	vi_pr(VI_INFO, "res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			res->start, res->end, dev->reg_base);
	if (IS_ERR(dev->reg_base)) {
		ret = PTR_ERR(dev->reg_base);
		return ret;
	}

	/* Interrupt */
	dev->irq_num = platform_get_irq_byname(pdev, CVI_VI_IRQ_NAME);
	if (dev->irq_num < 0) {
		dev_err(&pdev->dev, "No IRQ resource for %s\n", CVI_VI_IRQ_NAME);
		return -ENODEV;
	}
	vi_pr(VI_INFO, "irq(%d) for %s get from platform driver.\n",
			dev->irq_num, CVI_VI_IRQ_NAME);
	ret = vi_core_clk_init(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to init clk, err %d\n", ret);
		goto err_clk_init;
	}
	ret = vi_core_register_cdev(dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register dev, err %d\n", ret);
		goto err_dev_register;
	}

	ret = vi_create_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create instance, err %d\n", ret);
		goto err_create_instance;
	}

	ret = devm_request_irq(&pdev->dev, dev->irq_num, vi_core_isr, 0,
				pdev->name, dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq_num(%d) ret(%d)\n",
				dev->irq_num, ret);
		ret = -EINVAL;
		goto err_req_irq;
	}

	vi_pr(VI_INFO, "isp registered as %s\n", CVI_VI_DEV_NAME);

err_create_instance:

err_dev_register:

err_clk_init:

err_req_irq:
	return ret;
}

static int vi_core_remove(struct platform_device *pdev)
{
	int ret = 0;

	struct cvi_vi_dev *dev = dev_get_drvdata(&pdev->dev);

	ret = vi_destroy_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	ret = vi_core_rm_cb();
	if (ret) {
		dev_err(&pdev->dev, "Failed to rm vi cb, err %d\n", ret);
	}

	device_destroy(dev->vi_class, dev->cdev_id);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->cdev_id, 1);
	class_destroy(dev->vi_class);

	dev_set_drvdata(&pdev->dev, NULL);

err_destroy_instance:
	vi_pr(VI_INFO, "%s -\n", __func__);

	return ret;
}

static const struct of_device_id vi_core_match[] = {
	{
		.compatible = "cvitek,vi",
		.data       = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, vi_core_match);

static struct platform_driver vi_core_driver = {
	.probe = vi_core_probe,
	.remove = vi_core_remove,
	.driver = {
		.name = CVI_VI_DEV_NAME,
		.of_match_table = vi_core_match,
	},
};

/* 供 camera 合并模块单一入口调用，避免与 mipi-rx 的 module_init 冲突 */
int vi_core_register(void)
{
	return platform_driver_register(&vi_core_driver);
}
void vi_core_unregister(void)
{
	platform_driver_unregister(&vi_core_driver);
}

MODULE_AUTHOR("CVITEK Inc.");
MODULE_DESCRIPTION("Cvitek video input driver");
MODULE_LICENSE("GPL");

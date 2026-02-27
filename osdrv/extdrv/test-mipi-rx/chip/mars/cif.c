#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/iommu.h>
#include <linux/irq.h>
#include <linux/reset.h>
#include <generated/compile.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/cvi_defines.h>
#include "pinctrl-mars.h"
#include <linux/ctype.h>
#include <linux/version.h>

#include "linux/cif_uapi.h"
#include "linux/vi_snsr.h"
#include "drv/cif_drv.h"
#include "cif.h"
#include <vip_common.h>
#include <base_cb.h>
#include <cif_cb.h>

/* 仅保留 MIPI RX 接口，关闭其他输入模式相关代码（DVP/BT/SUBLVDS/HISPI 等） */
#define MIPI_IF
/* DVP_IF / BT601_IF / BT656_IF 等在 test-mipi-rx 中不编译 */

#define MIPI_RX_DEV_NAME "cvi-mipi-rx"
#define MAX_CIF_PROC_BUF 32

enum {
	LANE_SKEW_CROSS_CLK,
	LANE_SKEW_CROSS_DATA_NEAR,
	LANE_SKEW_CROSS_DATA_FAR,
	LANE_SKEW_CLK,
	LANE_SKEW_DATA,
	LANE_SKEW_NUM,
};

static int mclk1 = CAMPLL_FREQ_NONE;
module_param(mclk1, int, 0644);
MODULE_PARM_DESC(mclk1, "cam1 mclk");

static int lane_phase[LANE_SKEW_NUM] = {0x00, 0x03, 0x08, 0x00, 0x03};
static int count;
module_param_array(lane_phase, int, &count, 0664);

static int bypass_mac_clk;
module_param(bypass_mac_clk, int, 0644);
MODULE_PARM_DESC(bypass_mac_clk, "byass mac clk");

static unsigned int max_mac_clk = 594;
module_param(max_mac_clk, uint, 0644);
MODULE_PARM_DESC(max_mac_clk, "max mac clk");

static int cif_set_output_clk_edge(struct cvi_cif_dev *dev,
				   struct clk_edge_s *clk_edge);

const struct sync_code_s default_sync_code = {
	.norm_bk_sav = 0xAB0,
	.norm_bk_eav = 0xB60,
	.norm_sav = 0x800,
	.norm_eav = 0x9D0,
	.n0_bk_sav = 0x2B0,
	.n0_bk_eav = 0x360,
	.n1_bk_sav = 0x6B0,
	.n1_bk_eav = 0x760,
};

static struct cvi_cif_dev *file_cif_dev(struct file *file)
{
	return container_of(file->private_data, struct cvi_cif_dev, miscdev);
}

const char *_to_string_mac_clk(enum rx_mac_clk_e mac_clk)
{
	switch (mac_clk) {
	case RX_MAC_CLK_200M:
		return "200MHZ";
	case RX_MAC_CLK_300M:
		return "300MHZ";
	case RX_MAC_CLK_400M:
		return "400MHZ";
	case RX_MAC_CLK_500M:
		return "500MHZ";
	case RX_MAC_CLK_600M:
		return "600MHZ";
	default:
		return "unknown";
	}
}

const char *_to_string_cmd(unsigned int cmd)
{
	switch (cmd) {
	case CVI_MIPI_SET_DEV_ATTR:
		return "CVI_MIPI_SET_DEV_ATTR";
	case CVI_MIPI_SET_HS_MODE:
		return "CVI_MIPI_SET_HS_MODE";
	case CVI_MIPI_SET_OUTPUT_CLK_EDGE:
		return "CVI_MIPI_SET_OUTPUT_CLK_EDGE";
	case CVI_MIPI_RESET_MIPI:
		return "CVI_MIPI_RESET_MIPI";
	case CVI_MIPI_SET_CROP_TOP:
		return "CVI_MIPI_SET_CROP_TOP";
	case CVI_MIPI_SET_WDR_MANUAL:
		return "CVI_MIPI_SET_WDR_MANUAL";
	case CVI_MIPI_SET_LVDS_FP_VS:
		return "CVI_MIPI_SET_LVDS_FP_VS";
	case CVI_MIPI_RESET_SENSOR:
		return "CVI_MIPI_RESET_SENSOR";
	case CVI_MIPI_UNRESET_SENSOR:
		return "CVI_MIPI_UNRESET_SENSOR";
	case CVI_MIPI_ENABLE_SENSOR_CLOCK:
		return "CVI_MIPI_ENABLE_SENSOR_CLOCK";
	case CVI_MIPI_DISABLE_SENSOR_CLOCK:
		return "CVI_MIPI_DISABLE_SENSOR_CLOCK";
	case CVI_MIPI_RESET_LVDS:
		return "CVI_MIPI_RESET_LVDS";
	case CVI_MIPI_GET_CIF_ATTR:
		return "CVI_MIPI_GET_CIF_ATTR";
	case CVI_MIPI_SET_MAX_MAC_CLOCK:
		return "CVI_MIPI_SET_MAX_MAC_CLOCK";
	case CVI_MIPI_SET_CROP_WINDOW:
		return "CVI_MIPI_SET_CROP_WINDOW";
	default:
		return "unknown";
	}
	return "unknown";
}

const char *_to_string_raw_data_type(enum raw_data_type_e raw_data_type)
{
	switch (raw_data_type) {
	case RAW_DATA_8BIT:
		return "RAW8";
	case RAW_DATA_10BIT:
		return "RAW10";
	case RAW_DATA_12BIT:
		return "RAW12";
	case YUV422_8BIT:
		return "YUV422_8BIT";
	case YUV422_10BIT:
		return "YUV422_10BIT";
	default:
		return "unknown";
	}
}

#define LANE_IS_PORT1(x)	((x > 2) ? 1 : 0)
#define IS_SAME_PORT(x, y)	(!((LANE_IS_PORT1(x))^(LANE_IS_PORT1(y))))

#define PAD_CTRL_PU		BIT(2)
#define PAD_CTRL_PD		BIT(3)
#define PSD_CTRL_OFFSET(n)	((5 - n) * 8)

#ifdef MIPI_IF
static int _cif_set_attr_mipi(struct cvi_cif_dev *dev,
			      struct cif_ctx *ctx,
			      struct mipi_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_csi *csi = &param->cfg.csi;
	uint8_t tbl = 0x1F;
	int i, j = 0, clk_port = 0;
	uint32_t value;

	param->type = CIF_TYPE_CSI;
	pr_info("cif_set_attr_mipi raw_data_type=%s", _to_string_raw_data_type(attr->raw_data_type));
	pr_info("cif_set_attr_mipi decode_type=%x", csi->decode_type);
	pr_info("cif_set_attr_mipi fmt=%x", csi->fmt);
	pr_info("cif_set_attr_mipi lane_num=%d", csi->lane_num);
	pr_info("cif_set_attr_mipi lane_id[0]=%d", attr->lane_id[0]);
	pr_info("cif_set_attr_mipi lane_id[1]=%d", attr->lane_id[1]);
	pr_info("cif_set_attr_mipi lane_id[2]=%d", attr->lane_id[2]);
	pr_info("cif_set_attr_mipi lane_id[3]=%d", attr->lane_id[3]);
	/* config the bit mode. */
	// switch (attr->raw_data_type) {
	// case RAW_DATA_8BIT:
	// 	csi->fmt = CSI_RAW_8;
	// 	csi->decode_type = 0x2A;
	// 	break;
	// case RAW_DATA_10BIT:
	// 	csi->fmt = CSI_RAW_10;
	// 	csi->decode_type = 0x2B;
	// 	break;
	// case RAW_DATA_12BIT:
	// 	csi->fmt = CSI_RAW_12;
	// 	csi->decode_type = 0x2C;
	// 	break;
	// case YUV422_8BIT:
	// 	csi->fmt = CSI_YUV422_8B;
	// 	csi->decode_type = 0x1E;
	// 	break;
	// case YUV422_10BIT:
	// 	csi->fmt = CSI_YUV422_10B;
	// 	csi->decode_type = 0x1F;
	// 	break;
	// default:
	// 	return -EINVAL;
	// }
	csi->fmt = CSI_YUV422_10B;
	csi->decode_type = 0x1F;

	/* config the lane id */
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;
		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) != clk_port)
				clk_port = -1;
		}
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	csi->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	pr_info("cif_set_attr_mipi lane_num=%d", csi->lane_num);
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
	// lane_num == 2
	pr_info("cif_set_attr_mipi lane_id[0]=%d %d", attr->lane_id[0], LANE_IS_PORT1(attr->lane_id[0]));
	if (LANE_IS_PORT1(attr->lane_id[0]))
		cif_set_clk_dir(ctx, CIF_CLK_P12P0);
	else
		cif_set_clk_dir(ctx, CIF_CLK_P02P1);

	/* if clk and data are in the same port.*/
	for (i = 0; i < (csi->lane_num + 1); i++) {
		if (!i)
			cif_set_lane_deskew(ctx, attr->lane_id[i],
					lane_phase[LANE_SKEW_CROSS_CLK]);
		else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
			cif_set_lane_deskew(ctx, attr->lane_id[i],
					lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
		else
			cif_set_lane_deskew(ctx, attr->lane_id[i],
					lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	}
	/* config vc mapping */
	for (i = 0; i < MIPI_DEMUX_NUM; i++) {
		csi->vc_mapping[i] = i;
	}
	param->hdr_en = 0;
	cif_streaming(ctx, 1, param->hdr_en);

	return 0;
}
#endif //MIPI_IF

#define MAC0_CLK_CTRL1_OFFSET		4U
#define MAC1_CLK_CTRL1_OFFSET		8U
#define MAC2_CLK_CTRL1_OFFSET		30U
#define MAC_CLK_CTRL1_MASK		0x3U

static int _cif_set_mac_clk(struct cvi_cif_dev *cdev, uint32_t devno,
		enum rx_mac_clk_e mac_clk)
{
	struct cvi_link *link = &cdev->link[devno];
	u32 data, mask;
	// u32 clk_val = _cif_mac_enum_to_value(mac_clk);
	u32 clk_val = 198;

	pr_info("cif_set_mac_clk devno=%u mac_clk=%s(%u)", devno, _to_string_mac_clk(mac_clk), clk_val);
	pr_info("cdev->max_mac_clk=%u bypass_mac_clk=%u", cdev->max_mac_clk, bypass_mac_clk);
	link->mac_clk = mac_clk;

	/* select the source to vip_sys2 */
	mask = (MAC_CLK_CTRL1_MASK << MAC0_CLK_CTRL1_OFFSET);
	data = 0x2 << MAC0_CLK_CTRL1_OFFSET;
	vip_sys_reg_write_mask(0x1C, mask, data);

	dev_dbg(link->dev, "use div0 clk_disppll as vip_sys2 source\n");
	clk_set_parent(cdev->vip_sys2.clk_o, cdev->clk_disppll.clk_o);

	{
		/* target = source * (1 + ratio) / 32, ratio <= 0x1F */
		u32 tmp = clk_val * 32 / cdev->max_mac_clk;
		/* roundup */
		if ((clk_val * 32) % cdev->max_mac_clk)
			tmp++;
		tmp--;

		VIP_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC0, tmp);
		VIP_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC0, 1);
		VIP_UPDATE_CLK_RATIO(SEL_CSI_MAC0);

		//clk_set_rate(clk_get_parent(cdev->vip_sys2.clk_o), cdev->max_mac_clk * 1000000UL);
		dev_dbg(link->dev, "ratio %d, set rate %dM\n", tmp,
			(tmp + 1) * cdev->max_mac_clk / 32);
		udelay(5);
	}
	return 0;
}

static inline int cif_set_mac_clk(struct cvi_cif_dev *dev, uint32_t devno,
		enum rx_mac_clk_e mac_clk)
{
	return _cif_set_mac_clk(dev, devno, mac_clk);
}

static int cif_set_dev_attr(struct cvi_cif_dev *dev,
			    struct combo_dev_attr_s *attr)
{
	struct device *_dev = dev->miscdev.this_device;
	struct cif_ctx *ctx;
	struct cif_param *param;
	struct combo_dev_attr_s *rx_attr;
	int rc = 0;

	ctx = &dev->link[attr->devno].cif_ctx;
	param = &dev->link[attr->devno].param;
	rx_attr = &dev->link[attr->devno].attr;

	memset(param, 0, sizeof(*param));
	ctx->cur_config = param;
	memcpy(rx_attr, attr, sizeof(*rx_attr));

	/* Setting for serial interface. */
	{
		struct clk_edge_s clk_edge;

		/* set the default clock edge. */
		clk_edge.devno = attr->devno;
		clk_edge.edge = CLK_DOWN_EDGE;
		cif_set_output_clk_edge(dev, &clk_edge);
	}

	/* set mac clk */
	rc = cif_set_mac_clk(dev, attr->devno, attr->mac_clk);
	if (rc < 0)
		return rc;

	/* decide the mclk， just use mclk1 */	
	mclk1 = attr->mclk.freq;

	rc = _cif_set_attr_mipi(dev, ctx, &rx_attr->mipi_attr);

	if (rc < 0) {
		dev_err(_dev, "set attr fail %d\n", rc);
		return rc;
	}
	dev->link[attr->devno].is_on = 1;
	/* unmask the interrupts */
	cif_unmask_csi_int_sts(ctx, 0x1F);

	return 0;
}

static int cif_set_output_clk_edge(struct cvi_cif_dev *dev,
				   struct clk_edge_s *clk_edge)
{
	struct cif_ctx *ctx = &dev->link[clk_edge->devno].cif_ctx;

	dev->link[clk_edge->devno].clk_edge = clk_edge->edge;

	cif_set_clk_edge(ctx, CIF_PHY_LANE_0,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_1,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_2,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_3,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_4,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	cif_set_clk_edge(ctx, CIF_PHY_LANE_5,
			 (clk_edge->edge == CLK_UP_EDGE)
			 ? CIF_CLK_RISING : CIF_CLK_FALLING);

	return 0;
}

static void cif_reset_param(struct cvi_link *link)
{
	link->is_on = 0;
	link->clk_edge = CLK_UP_EDGE;
	link->msb = OUTPUT_NORM_MSB;
	link->crop_top = 0;
	link->distance_fp = 0;
	memset(&link->param, 0, sizeof(struct cif_param));
	memset(&link->attr, 0, sizeof(struct combo_dev_attr_s));
	memset(&link->sts_csi, 0, sizeof(struct cvi_csi_status));
	memset(&link->sts_lvds, 0, sizeof(struct cvi_lvds_status));
}

static int cif_reset_mipi(struct cvi_cif_dev *dev, uint32_t devno)
{
	union vip_sys_reset mask;
	struct cvi_link *link = &dev->link[devno];

	/* mask the interrupts */
	if (link->is_on)
		cif_mask_csi_int_sts(&link->cif_ctx, 0x1F);

	/* reset phy */
	if (link->phy_reset && link->phy_apb_reset) {
		reset_control_assert(link->phy_reset);
		reset_control_assert(link->phy_apb_reset);
		udelay(5);
		reset_control_deassert(link->phy_reset);
		reset_control_deassert(link->phy_apb_reset);
	}

	/* sw reset mac by vip register */
	mask.b.csi_mac0 = 1;
	vip_toggle_reset(mask);

	/* reset parameters. */
	cif_reset_param(link);

	return 0;
}

static int _cif_enable_snsr_clk(struct device *dev,
				struct cvi_cif_dev *cdev,
				uint32_t devno, uint8_t on)
{
	uint32_t value;

	if (mclk1 > CAMPLL_FREQ_NONE  && mclk1 < CAMPLL_FREQ_NUM) {
		// const struct cam_pll_s *clk = &cam_pll_setting[mclk1];
		// const struct cam_pll_s *clk = &cam_pll_setting_24M;
		const uint32_t clk_rate = 24000000;
		pr_info("clk_cam1=%p, clk_rate=%d", cdev->clk_cam1.clk_o, clk_rate);

		/* camera interface. */
		// PINMUX_CONFIG(CAM_MCLK1, CAM_MCLK1);
		(void)value;

		/* set rate of clk_cam1 */
		clk_set_rate(cdev->clk_cam1.clk_o, clk_rate);

		if (on) {
			if (!cdev->clk_cam1.is_on) {
				clk_prepare_enable(cdev->clk_cam1.clk_o);
				cdev->clk_cam1.is_on = 1;
			}
		} else if (cdev->clk_cam1.is_on) {
			clk_disable_unprepare(cdev->clk_cam1.clk_o);
			cdev->clk_cam1.is_on = 0;
		}
	}
	return 0;
}

static inline int cif_enable_snsr_clk(struct cvi_cif_dev *dev,
				uint32_t devno, uint8_t on)
{
	struct device *_dev = dev->miscdev.this_device;

	return _cif_enable_snsr_clk(_dev, dev, devno, on);
}

static int cif_reset_snsr_gpio(struct cvi_cif_dev *dev,
			       unsigned int devno, uint8_t on)
{
	struct cvi_link *link;
	int reset;

	if (devno >= MAX_LINK_NUM)
		return -EINVAL;

	link = &dev->link[devno];
	reset = (link->snsr_rst_pol == OF_GPIO_ACTIVE_LOW) ? 0 : 1;

	if (!gpio_is_valid(link->snsr_rst_pin))
		return 0;
	if (on)
		gpio_direction_output(link->snsr_rst_pin, reset);
	else
		gpio_direction_output(link->snsr_rst_pin, !reset);

	return 0;
}

static long _cif_ioctl(struct cvi_cif_dev *dev, unsigned int cmd,
		       unsigned long arg, unsigned int from_user)
{
	struct device *_dev = dev->miscdev.this_device;
	uint32_t devno;

	dev_dbg(_dev, "%s\n", _to_string_cmd(cmd));
	pr_info("cmd: %s(%d), arg: %lu\n", _to_string_cmd(cmd), cmd, arg);
	if (arg == 0) {
		dev_err(_dev, "null pointer\n");
		return -EINVAL;
	}

	switch (cmd) {
	case CVI_MIPI_SET_DEV_ATTR:
	{
		struct combo_dev_attr_s attr;

		if (from_user) {
			if (copy_from_user(&attr, (void *)arg, sizeof(attr))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			memcpy(&attr, (void *)arg, sizeof(attr));

		return cif_set_dev_attr(dev, &attr);
	}
	case CVI_MIPI_RESET_MIPI:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;

		return cif_reset_mipi(dev, devno);
	case CVI_MIPI_RESET_SENSOR:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_reset_snsr_gpio(dev, devno, 1);
	case CVI_MIPI_UNRESET_SENSOR:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_reset_snsr_gpio(dev, devno, 0);
	case CVI_MIPI_ENABLE_SENSOR_CLOCK:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_enable_snsr_clk(dev, devno, 1);
	case CVI_MIPI_DISABLE_SENSOR_CLOCK:
		if (from_user) {
			if (copy_from_user(&devno, (void *)arg, sizeof(devno))) {
				dev_err(_dev, "copy_from_user failed.\n");
				return -ENOMEM;
			}
		} else
			devno = *(uint32_t *)arg;
		return cif_enable_snsr_clk(dev, devno, 0);
	default:
		pr_info("unknown cmd=0x%x", cmd);
		return -ENOIOCTLCMD;
	}
	return 0;
}

static long cif_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct cvi_cif_dev *dev = file_cif_dev(file);

	return _cif_ioctl(dev, cmd, arg, 1);
}

static int cif_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int cif_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations cif_fops = {
	.owner = THIS_MODULE,
	.open = cif_open,
	.release = cif_release,
	.unlocked_ioctl = cif_ioctl,
};

static int cif_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	return _cif_ioctl((struct cvi_cif_dev *)dev, cmd, (unsigned long)arg, 0);
}

static int cif_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_CIF);
}

static int cif_register_cb(struct cvi_cif_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_CIF;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= cif_cb;

	return base_reg_module_cb(&reg_cb);
}

static int cif_init_miscdev(struct platform_device *pdev, struct cvi_cif_dev *dev)
{
	int rc, i;

	dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev->miscdev.name = MIPI_RX_DEV_NAME;
	dev->miscdev.fops = &cif_fops;

	rc = misc_register(&dev->miscdev);
	if (rc) {
		dev_err(&pdev->dev, "cif: failed to register misc device.\n");
		return rc;
	}

	/* init cif */
	for (i = 0; i < MAX_LINK_NUM; i++) {
		struct cif_ctx *ctx = &dev->link[i].cif_ctx;

		ctx->mac_phys_regs = cif_get_mac_phys_reg_bases(i);
		ctx->wrap_phys_regs = cif_get_wrap_phys_reg_bases(i);
	}

	/* register cif_cb */
	rc = cif_register_cb(dev);
	if (rc)
		dev_err(&pdev->dev, "cif: failed to register callbacks.\n");

	return rc;
}

static irqreturn_t cif_isr(int irq, void *_link)
{
	struct cvi_link *link = (struct cvi_link *)_link;
	struct cif_ctx *ctx = &link->cif_ctx;

	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_ECC_ERR_MASK))
		link->sts_csi.errcnt_ecc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_CRC_ERR_MASK))
		link->sts_csi.errcnt_crc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_WC_ERR_MASK))
		link->sts_csi.errcnt_wc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_HDR_ERR_MASK))
		link->sts_csi.errcnt_hdr++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_FIFO_FULL_MASK))
		link->sts_csi.fifo_full++;

	if (link->sts_csi.errcnt_ecc > 0xFFFF ||
		link->sts_csi.errcnt_crc > 0xFFFF ||
		link->sts_csi.errcnt_hdr > 0xFFFF ||
		link->sts_csi.fifo_full > 0xFFFF ||
		link->sts_csi.errcnt_wc > 0xFFFF) {

		cif_mask_csi_int_sts(ctx, 0x1F);
		dev_err(link->dev, "mask the interrupt since err cnt is full\n");
		dev_err(link->dev, "ecc = %u, crc = %u, wc = %u, hdr = %u, fifo_full = %u\n",
				link->sts_csi.errcnt_ecc,
				link->sts_csi.errcnt_crc,
				link->sts_csi.errcnt_wc,
				link->sts_csi.errcnt_hdr,
				link->sts_csi.fifo_full);
	}

	cif_clear_csi_int_sts(ctx);

	return IRQ_HANDLED;
}

static char irq_name[MAX_LINK_NUM][20] = {
	"cif-irq0",
	"cif-irq1",
	"cif-irq2"
};

static int _init_resource(struct platform_device *pdev)
{
	struct resource *res = NULL;
	void *reg_base[6];
	struct cvi_cif_dev *dev;
	int i;
	struct cvi_link *link;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_cif drvdata\n");
		return -EINVAL;
	}
	/* cam clk shall not depend on the link so we separate them from link. */
	dev->clk_cam0.clk_o = devm_clk_get(&pdev->dev, "clk_cam0");
	if (!IS_ERR_OR_NULL(dev->clk_cam0.clk_o))
		dev_info(&pdev->dev, "cam0 clk installed\n");
	dev->clk_cam1.clk_o = devm_clk_get(&pdev->dev, "clk_cam1");
	if (!IS_ERR_OR_NULL(dev->clk_cam1.clk_o))
		dev_info(&pdev->dev, "cam1 clk installed\n");
	dev->vip_sys2.clk_o = devm_clk_get(&pdev->dev, "clk_sys_2");
	if (!IS_ERR_OR_NULL(dev->vip_sys2.clk_o))
		dev_info(&pdev->dev, "vip_sys_2 clk installed\n");
	dev->clk_mipimpll.clk_o = devm_clk_get(&pdev->dev, "clk_mipimpll");
	if (!IS_ERR_OR_NULL(dev->clk_mipimpll.clk_o))
		dev_info(&pdev->dev, "clk_mipimpll clk installed %p\n", dev->clk_mipimpll.clk_o);
	dev->clk_disppll.clk_o = devm_clk_get(&pdev->dev, "clk_disppll");
	if (!IS_ERR_OR_NULL(dev->clk_disppll.clk_o))
		dev_info(&pdev->dev, "clk_disppll clk installed %p\n", dev->clk_disppll.clk_o);
	dev->clk_fpll.clk_o = devm_clk_get(&pdev->dev, "clk_fpll");
	if (!IS_ERR_OR_NULL(dev->clk_fpll.clk_o))
		dev_info(&pdev->dev, "clk_fpll clk installed %p\n", dev->clk_fpll.clk_o);

	for (i = 0; i < (MAX_LINK_NUM * 2 - 1); ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res)
			break;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		reg_base[i] = devm_ioremap(&pdev->dev, res->start, res->end - res->start);
#else
		reg_base[i] = devm_ioremap_nocache(&pdev->dev, res->start, res->end - res->start);
#endif

		dev_info(&pdev->dev,
			 "(%d) res-reg: start: 0x%llx, end: 0x%llx.",
			 i, res->start, res->end);
		dev_info(&pdev->dev, " virt-addr(%p)\n", reg_base[i]);
	}
	if (i > 1)
		cif_set_base_addr(0, reg_base[0], reg_base[1]);
	if (i > 2)
		cif_set_base_addr(1, reg_base[2], reg_base[1]);
	if (i > 3)
		cif_set_base_addr(2, reg_base[3], reg_base[1]);
	/* init pad_ctrl. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, i);
	if (!res) {
		dev_info(&pdev->dev, "no pad_ctrl for cif\n");
	} else {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		dev->pad_ctrl = devm_ioremap(&pdev->dev, res->start, res->end - res->start);
#else
		dev->pad_ctrl = devm_ioremap_nocache(&pdev->dev, res->start, res->end - res->start);
#endif
		dev_info(&pdev->dev,
			 "pad-ctrl res-reg: start: 0x%llx, end: 0x%llx.",
			 res->start, res->end);
	}

	/* Init max mac clock. */
	if (max_mac_clk <= 400)
		dev->max_mac_clk = 396;
	else if (max_mac_clk <= 500)
		dev->max_mac_clk = 500;
	else
		dev->max_mac_clk = 594;

	/* Interrupt */
	for (i = 0; i < CIF_MAX_CSI_NUM; ++i) {
		link = &dev->link[i];

		link->irq_num = platform_get_irq(pdev, i);
		if (link->irq_num < 0)
			break;
		if (devm_request_irq(&pdev->dev, link->irq_num, cif_isr, IRQF_SHARED, irq_name[i], link))
			break;
		dev_info(&pdev->dev, "request irq-%d as %s\n",
			 link->irq_num, irq_name[i]);

		/* set the port id */
		link->cif_ctx.mac_num = i;
	}

	/* reset pin */
	for (i = 0; i < MAX_LINK_NUM; ++i) {
		link = &dev->link[i];
		link->dev = &pdev->dev;
		link->mac_clk = RX_MAC_CLK_400M;
		link->snsr_rst_pin = of_get_named_gpio_flags(pdev->dev.of_node,
				"snsr-reset", i, &link->snsr_rst_pol);
		if (link->snsr_rst_pin < 0)
			break;

		if (gpio_request(link->snsr_rst_pin, "snsr-rst-gpio"))
			return 0;

		dev_info(&pdev->dev, "rst_pin = %d, pol = %d\n",
			link->snsr_rst_pin, link->snsr_rst_pol);
	}

	/* sw reset */
	link = &dev->link[0];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "phy0");
	if (link->phy_reset)
		dev_info(&pdev->dev, "phy0 reset installed\n");
	link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb0");
	if (link->phy_apb_reset)
		dev_info(&pdev->dev, "phy0 apb reset installed\n");
	link = &dev->link[1];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "phy1");
	if (link->phy_reset)
		dev_info(&pdev->dev, "phy1 reset installed\n");
	link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb1");
	if (link->phy_apb_reset)
		dev_info(&pdev->dev, "phy1 apb reset installed\n");

	return 0;
}

static int cvi_cif_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_cif_dev *dev;

	/* allocate main cif state structure */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* initialize locks */
	spin_lock_init(&dev->lock);
	mutex_init(&dev->mutex);

	dev_set_drvdata(&pdev->dev, dev);
	platform_set_drvdata(pdev, dev);

	rc = cif_init_miscdev(pdev, dev);
	if (rc < 0)
		return rc;

	rc = _init_resource(pdev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to init res for cif, %d\n", rc);
		return rc;
	}

	return 0;
}

static int cvi_cif_remove(struct platform_device *pdev)
{
	struct cvi_cif_dev *dev;

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	/* rm cif_cb */
	if (cif_rm_cb())
		dev_err(&pdev->dev, "cif: failed to rm cb.\n");

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_cif drvdata");
		return 0;
	}

	misc_deregister(&dev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static const struct of_device_id cvi_cif_dt_match[] = {
	{.compatible = "cvitek,cif"},
	{}
};

static struct platform_driver cvi_cif_pdrv = {
	.probe      = cvi_cif_probe,
	.remove     = cvi_cif_remove,
	.driver     = {
		.name		= "cif",
		.owner		= THIS_MODULE,
		.of_match_table	= cvi_cif_dt_match,
	},
};

static int __init cvi_cif_init(void)
{
	int rc;

	rc = platform_driver_register(&cvi_cif_pdrv);
	return rc;
}

static void __exit cvi_cif_exit(void)
{
	platform_driver_unregister(&cvi_cif_pdrv);
}

MODULE_DESCRIPTION("Cvitek Camera Interface Driver");
MODULE_AUTHOR("Saxen Ko");
MODULE_LICENSE("GPL");
module_init(cvi_cif_init);
module_exit(cvi_cif_exit);

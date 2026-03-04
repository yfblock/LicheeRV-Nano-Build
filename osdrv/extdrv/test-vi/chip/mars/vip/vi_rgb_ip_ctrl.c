#include <vip/vi_drv.h>

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern uint8_t g_w_bit[ISP_PRERAW_VIRT_MAX], g_h_bit[ISP_PRERAW_VIRT_MAX];
extern struct lmap_cfg g_lmp_cfg[ISP_PRERAW_VIRT_MAX];

#define LTM_DARK_TONE_LUT_SIZE   0x100
#define LTM_BRIGHT_TONE_LUT_SIZE 0x200
#define LTM_GLOBAL_LUT_SIZE      0x300

void ispblk_tnr_post_chg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	int w = ctx->isp_pipe_cfg[raw_num].crop.w;
	int h = ctx->isp_pipe_cfg[raw_num].crop.h;
	int grid_size = (1 << ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit);

	union REG_ISP_MMAP_60 reg_60;
	union REG_ISP_MMAP_30 reg_30;
	union REG_ISP_MMAP_D0 reg_d0;
	union REG_ISP_MMAP_D4 reg_d4;
	union REG_ISP_MMAP_D8 reg_d8;

	reg_60.raw = ISP_RD_REG(manr, REG_ISP_MMAP_T, REG_60);
	reg_60.bits.RGBMAP_W_BIT = ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit;
	reg_60.bits.RGBMAP_H_BIT = ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_60, reg_60.raw);

	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_04, WH_SW_MODE, 1);

	reg_30.raw = 0;
	reg_30.bits.IMG_WIDTHM1_SW	= ((((w + grid_size - 1) / grid_size) * 6 + 47) / 48 * 8 * grid_size - 1);
	reg_30.bits.IMG_HEIGHTM1_SW	= h - 1;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_30, reg_30.raw);

	reg_d0.raw = 0;
	reg_d0.bits.CROP_ENABLE_SCALAR		= 1;
	reg_d0.bits.IMG_WIDTH_CROP_SCALAR	= reg_30.bits.IMG_WIDTHM1_SW;
	reg_d0.bits.IMG_HEIGHT_CROP_SCALAR	= reg_30.bits.IMG_HEIGHTM1_SW;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_D0, reg_d0.raw);

	reg_d4.raw = 0;
	reg_d4.bits.CROP_W_STR_SCALAR		= 0;
	reg_d4.bits.CROP_W_END_SCALAR		= w - 1;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_D4, reg_d4.raw);

	reg_d8.raw = 0;
	reg_d8.bits.CROP_H_STR_SCALAR		= 0;
	reg_d8.bits.CROP_H_END_SCALAR		= h - 1;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_D8, reg_d8.raw);

	ispblk_mmap_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL32);
	ispblk_mmap_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL34);
}

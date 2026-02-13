#include <linux/types.h>
#include <linux/delay.h>
#include "reg.h"
#include "inc/cif_reg.h"
#include "cif_drv.h"

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static uintptr_t mac_reg_base[MAX_LINK_NUM];
static uintptr_t wrap_reg_base[MAX_LINK_NUM];

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void cif_set_base_addr(uint32_t link, void *mac_base, void *wrap_base)
{
	uintptr_t *addr = cif_get_mac_phys_reg_bases(link);
	int i = 0;

	for (i = 0; i < CIF_MAC_BLK_ID_MAX; ++i) {
		addr[i] -= mac_reg_base[link];
		addr[i] += (uintptr_t)mac_base;
	}
	mac_reg_base[link] = (uintptr_t)mac_base;

	addr = cif_get_wrap_phys_reg_bases(link);

	for (i = 0; i < CIF_WRAP_BLK_ID_MAX; ++i) {
		addr[i] -= wrap_reg_base[link];
		addr[i] += (uintptr_t)wrap_base;
	}
	wrap_reg_base[link] = (uintptr_t)wrap_base;
}

void cif_init(struct cif_ctx *ctx)
{
}

void cif_uninit(struct cif_ctx *ctx)
{
}

void cif_reset(struct cif_ctx *ctx)
{
}

static void _cif_csi_config(struct cif_ctx *ctx,
			   struct param_csi *param)
{
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];
  pr_info("wrap_2l: %lx \n", wrap_4l);

	/* Config the sensor mode */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, SENSOR_MAC_MODE,
			     1);
	/* invert the HS/VS */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, CSI_VS_INV,
			     1);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, CSI_HS_INV,
			     1);
	/* CSI controller enable */
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_00, CSI_CTRL_ENABLE,
			     1);
	/* Config the lane enable */
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_00, CSI_LANE_MODE,
			     param->lane_num - 1);
	/* Config the VS gen mode */
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_70, CSI_VS_GEN_MODE,
			     param->vs_gen_mode);
  pr_info("mac_num: %d\n", ctx->mac_num);
	if (!ctx->mac_num) {
    pr_info("wrap_4l\n");
		/* [Note] disable auto_ignore and auto sync by default. */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_10, AUTO_IGNORE,
				     0);
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_10, AUTO_SYNC,
				     0);
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				     REG_00, SENSOR_MODE,
				     0);
	} else {
		/* [Note] disable auto_ignore and auto sync by default. */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_10, AUTO_IGNORE,
				     0);
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_10, AUTO_SYNC,
				     0);
		/* DPHY sensor mode select */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				     REG_00, SENSOR_MODE,
				     0);
	}
  pr_info("vc_mapping: %d %d %d %d\n", param->vc_mapping[0], param->vc_mapping[1], param->vc_mapping[2], param->vc_mapping[3]);
	/* Config csi vc mapping */
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_18, CSI_VC_MAP_CH00,
			     param->vc_mapping[0]);
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_18, CSI_VC_MAP_CH01,
			     param->vc_mapping[1]);
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_18, CSI_VC_MAP_CH10,
			     param->vc_mapping[2]);
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_18, CSI_VC_MAP_CH11,
			     param->vc_mapping[3]);
}

void cif_crop_info_line(struct cif_ctx *ctx, uint32_t line_num, uint32_t sw_up)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	/* Config the info line strip for HDR pattern 2 */
	if (line_num) {
		CIF_WR_BITS_GRP2(mac_top, REG_SENSOR_MAC_T,
				     REG_48,
				     SENSOR_MAC_INFO_LINE_NUM,
				     line_num,
				     SENSOR_MAC_RM_INFO_LINE,
				     1);

		if (sw_up) {
			CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
					     REG_00, SW_UP,
					     1);
			CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
					     REG_00, SW_UP,
					     1);
		}
	} else {
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_48,
				     SENSOR_MAC_RM_INFO_LINE,
				     0);
	}
}

void cif_set_crop(struct cif_ctx *ctx, struct cif_crop_s *crop)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	if (!crop->w || !crop->h)
		return;

	CIF_WR_BITS_GRP2(mac_top, REG_SENSOR_MAC_T,
		REG_B4,
		SENSOR_MAC_CROP_START_Y,
		crop->y,
		SENSOR_MAC_CROP_END_Y,
		(crop->y + crop->h));
	CIF_WR_BITS_GRP3(mac_top, REG_SENSOR_MAC_T,
		REG_B0,
		SENSOR_MAC_CROP_START_X,
		crop->x,
		SENSOR_MAC_CROP_END_X,
		(crop->x + crop->w),
		SENSOR_MAC_CROP_EN,
		crop->enable);
}

int cif_swap_yuv(struct cif_ctx *ctx, uint8_t uv_swap, uint8_t yc_swap)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	CIF_WR_BITS_GRP2(mac_top, REG_SENSOR_MAC_T,
		REG_B8,
		SENSOR_MAC_SWAPUV_EN,
		!!uv_swap,
		SENSOR_MAC_SWAPYC_EN,
		!!yc_swap);

	return 0;
}

void cif_set_bt_fmt_out(struct cif_ctx *ctx, enum ttl_bt_fmt_out fmt_out)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];

	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			     REG_10, TTL_BT_FMT_OUT,
			     fmt_out);
}

void cif_config(struct cif_ctx *ctx, struct cif_param *param)
{
	switch (param->type) {
	case CIF_TYPE_CSI:
		_cif_csi_config(ctx, &param->cfg.csi);
		break;
	default:
		break;
	}
}

static void _cif_hdr_csi_enable(struct cif_ctx *ctx,
				 struct param_csi *param,
				 uint32_t on)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	if (param->hdr_mode == CSI_HDR_MODE_VC) {
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_04, CSI_HDR_MODE,
	   		     0);
	} else if (param->hdr_mode == CSI_HDR_MODE_DT) {
		/* Enable dtat type mode. */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_74, CSI_HDR_DT_MODE,
				     1);
		/* Program lef data type. */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_74, CSI_HDR_DT_LEF,
				     param->data_type[0]);
		/* Program sef data type. */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_74, CSI_HDR_DT_SEF,
				     param->data_type[1]);
		/* Program decode data type. */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_74, CSI_HDR_DT_FORMAT,
				     param->decode_type);
	} else if (param->hdr_mode == CSI_HDR_MODE_DOL) {
		/* Enable Sony DOL mode. */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_04, CSI_HDR_MODE,
				     1);
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_04, CSI_ID_RM_ELSE,
				     1);
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_04, CSI_ID_RM_OB,
				     1);
	} else {
		/* [TODO] */
		CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_04, CSI_HDR_MODE,
				     1);
	}
	/* CV181X not support invert the HDR */
	// CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
	//		     REG_00, CSI_HDR_INV,
	//		     1);
	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_04, CSI_HDR_EN,
			     !!on);
}

void cif_hdr_manual_config(struct cif_ctx *ctx,
			   struct cif_param *param,
			   uint32_t sw_up)
{
	uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
  pr_info("cif hdr manual config  ==== \n");

	if (!param->hdr_manual) {
		CIF_WR_BITS_GRP3(mac_top, REG_SENSOR_MAC_T,
				 REG_40,
				 SENSOR_MAC_HDR_EN,
				 0,
				 SENSOR_MAC_HDR_HDR0INV, // to-do
				 0,
				 SENSOR_MAC_HDR_HDR1INV, // to-do
				 0);
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				 REG_40, SENSOR_MAC_HDR_MODE,
				 0);

		if (sw_up) {
			CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
					     REG_00, SW_UP,
					     1);
		}
		return;
	}

	/* Config the HDR mode V size and T2 line shift */
	CIF_WR_BITS_GRP2(mac_top, REG_SENSOR_MAC_T,
			 REG_44,
			 SENSOR_MAC_HDR_VSIZE,
			 param->hdr_vsize,
			 SENSOR_MAC_HDR_SHIFT,
			 param->hdr_shift);

	CIF_WR_BITS_GRP3(mac_top, REG_SENSOR_MAC_T,
			 REG_40,
			 SENSOR_MAC_HDR_EN,
			 1,
			 SENSOR_MAC_HDR_HDR0INV, // to-do
			 0,
			 SENSOR_MAC_HDR_HDR1INV, // to-do
			 0);
	CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
			 REG_40, SENSOR_MAC_HDR_MODE,
			 !!param->hdr_rm_padding);

	if (sw_up) {
		CIF_WR_BITS(mac_top, REG_SENSOR_MAC_T,
				     REG_00, SW_UP,
				     1);
	}
}
EXPORT_SYMBOL_GPL(cif_hdr_manual_config);

void cif_hdr_enable(struct cif_ctx *ctx, struct cif_param *param, uint32_t on)
{
  pr_info("== func == cif_hdr_enable\n");
	if (param->hdr_manual)
		return;

	switch (param->type) {
	case CIF_TYPE_CSI:
		_cif_hdr_csi_enable(ctx, &param->cfg.csi, on);
		break;
	default:
		break;
	}

}

static void cif_stream_enable(struct cif_ctx *ctx,
			      struct cif_param *param, uint32_t on)
{
	// uintptr_t mac_top = ctx->mac_phys_regs[CIF_MAC_BLK_ID_TOP];
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];
	union cif_cfg *cfg = &param->cfg;

	/* configure the phy termination only for serdes format. */
	if (param->type <= CIF_TYPE_HISPI) {
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			    REG_00, MIPIRX_PD_IBIAS, 1);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			    REG_00, MIPIRX_PD_RXLP, 0x3F);
		if (on) {
			CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_00, MIPIRX_PD_IBIAS, 0);
		}
	}

	switch (param->type) {
	case CIF_TYPE_CSI:
		/* clear the laine enable */
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				    REG_0C, DESKEW_LANE_EN, 0);
		} else {
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				    REG_0C, DESKEW_LANE_EN, 0);
		}
		if (on) {
			CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_00, MIPIRX_PD_RXLP, 0);
			udelay(20);
			/* lane enable */
			if (!ctx->mac_num) {
				CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
					    REG_0C, DESKEW_LANE_EN,
					    (1 << cfg->csi.lane_num) - 1);
			} else {
				CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
					    REG_0C, DESKEW_LANE_EN,
					    (1 << cfg->csi.lane_num) - 1);
			}
		}
		break;
	default:
		break;
	}

}

void cif_streaming(struct cif_ctx *ctx, uint32_t on, uint32_t hdr)
{
  pr_info("== cif_stream_enable == mac_num: %d check full: %d \n", ctx->mac_num, cif_check_csi_fifo_full(ctx));
	/* CIF OFF */
	cif_stream_enable(ctx, ctx->cur_config, 0);

	if (on) {
		cif_config(ctx, ctx->cur_config);
		cif_hdr_enable(ctx, ctx->cur_config, hdr);
		/* CIF ON */
		cif_stream_enable(ctx, ctx->cur_config, on);
	}
}

void cif_set_lane_id(struct cif_ctx *ctx, enum lane_id_e lane,
			uint32_t select, uint32_t pn_swap)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];

	switch (lane) {
	case CIF_LANE_CLK:
		if (!ctx->mac_num) {
			/* PHYA clock select */

			CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				      REG_04, MIPIRX_SEL_CLK_CHANNEL,
				      CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
						REG_04, MIPIRX_SEL_CLK_CHANNEL) | 1 << select);
			/* PHYD clock select */
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_CK_SEL,
				      select);
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_CK_PNSWAP,
				      pn_swap);
		} else {
			/* Enable dual mode */
			CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				      REG_30, SENSOR_PHY_MODE,
				      1);
			/* PHYA clock select */
			CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				      REG_04, MIPIRX_SEL_CLK_CHANNEL,
				      CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
						REG_04, MIPIRX_SEL_CLK_CHANNEL) | 1 << select);
			/* PHYD clock select */
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_08, CSI_LANE_CK_SEL,
				      select % 3);
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_08, CSI_LANE_CK_PNSWAP,
				      pn_swap);
		}
		break;
	case CIF_LANE_0:
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_04, CSI_LANE_D0_SEL,
				      select);
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_D0_PNSWAP,
				      pn_swap);
		} else {
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_04, CSI_LANE_D0_SEL,
				      select % 3);
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_08, CSI_LANE_D0_PNSWAP,
				      pn_swap);
		}
		break;
	case CIF_LANE_1:
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_04, CSI_LANE_D1_SEL,
				      select);
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_D1_PNSWAP,
				      pn_swap);
		} else {
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_04, CSI_LANE_D1_SEL,
				      select % 3);
			CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
				      REG_08, CSI_LANE_D1_PNSWAP,
				      pn_swap);
		}
		break;
	case CIF_LANE_2:
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_04, CSI_LANE_D2_SEL,
				      select);
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_D2_PNSWAP,
				      pn_swap);
		}
		break;
	case CIF_LANE_3:
		if (!ctx->mac_num) {
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_04, CSI_LANE_D3_SEL,
				      select);
			CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
				      REG_08, CSI_LANE_D3_PNSWAP,
				      pn_swap);
		}
		break;
	default:
		break;
	}
}

// to-do: check with RD
void cif_set_clk_edge(struct cif_ctx *ctx,
		      enum phy_lane_id_e lane, enum cif_clk_edge_e edge)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	switch (lane) {
	case CIF_PHY_LANE_0:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D0_CLK_INV,
			      edge);
		break;
	case CIF_PHY_LANE_1:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D1_CLK_INV,
			      edge);
		break;
	case CIF_PHY_LANE_2:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D2_CLK_INV,
			      edge);
		break;
	case CIF_PHY_LANE_3:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D3_CLK_INV,
			      edge);
		break;
	case CIF_PHY_LANE_4:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D4_CLK_INV,
			      edge);
		break;
	case CIF_PHY_LANE_5:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, AD_D5_CLK_INV,
			      edge);
		break;
	default:
		break;
	}
}

void cif_set_clk_dir(struct cif_ctx *ctx, enum cif_clk_dir_e dir)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
  pr_info("set_clk_dir: %d\n",dir);

	switch (dir) {
	case CIF_CLK_P02P1:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P1TOP0,
			      0);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P0TOP1,
			      1);
		break;
	case CIF_CLK_P12P0:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P0TOP1,
			      0);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P1TOP0,
			      1);
		break;
	case CIF_CLK_FREERUN:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P0TOP1,
			      0);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_04, MIPIRX_SEL_CLK_P1TOP0,
			      0);
		break;
	default:
		break;
	}
}

void cif_set_hs_settle(struct cif_ctx *ctx, uint8_t hs_settle)
{
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];

	if (!ctx->mac_num) {
		/* disable auto_ignore and auto sync. */
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
			      REG_10, AUTO_IGNORE,
			      0);
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
			      REG_10, AUTO_SYNC,
			      0);
		CIF_WR_BITS(wrap_4l, REG_SENSOR_PHY_4L_T,
			      REG_10, T_HS_SETTLE,
			      hs_settle);
	} else {
		/* disable auto_ignore and auto sync. */
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
			      REG_10, AUTO_IGNORE,
			      0);
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
			      REG_10, AUTO_SYNC,
			      0);
		CIF_WR_BITS(wrap_2l, REG_SENSOR_PHY_2L_T,
			      REG_10, T_HS_SETTLE,
			      hs_settle);

	}
}

uint8_t cif_get_lane_data(struct cif_ctx *ctx, enum phy_lane_id_e lane)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];
	uint8_t value;

	switch (lane) {
	case CIF_PHY_LANE_0:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_70, AD_D0_DATA);
		break;
	case CIF_PHY_LANE_1:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_70, AD_D1_DATA);
		break;
	case CIF_PHY_LANE_2:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_70, AD_D2_DATA);
		break;
	case CIF_PHY_LANE_3:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_70, AD_D3_DATA);
		break;
	case CIF_PHY_LANE_4:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_74, AD_D4_DATA);
		break;
	case CIF_PHY_LANE_5:
		value = CIF_RD_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
				    REG_74, AD_D5_DATA);
		break;
	default:
		value = 0;
		break;
	}

	return value;
}

void cif_set_lane_deskew(struct cif_ctx *ctx,
		      enum phy_lane_id_e lane, uint8_t phase)
{
	uintptr_t wrap_top = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_TOP];

	switch (lane) {
	case CIF_PHY_LANE_0:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_84, DESKEW_CODE0,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE0,
			      !!phase);
		break;
	case CIF_PHY_LANE_1:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_84, DESKEW_CODE1,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE1,
			      !!phase);
		break;
	case CIF_PHY_LANE_2:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_84, DESKEW_CODE2,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE2,
			      !!phase);
		break;
	case CIF_PHY_LANE_3:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_84, DESKEW_CODE3,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE3,
			      !!phase);
		break;
	case CIF_PHY_LANE_4:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_88, DESKEW_CODE4,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE4,
			      !!phase);
		break;
	case CIF_PHY_LANE_5:
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_88, DESKEW_CODE5,
			      phase);
		CIF_WR_BITS(wrap_top, REG_SENSOR_PHY_TOP_T,
			      REG_80, FORCE_DESKEW_CODE5,
			      !!phase);
		break;
	default:
		break;
	}
}

int cif_check_csi_int_sts(struct cif_ctx *ctx, uint32_t mask)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];
	uint32_t reg = CIF_RD_REG(mac_csi, REG_CSI_CTRL_TOP_T, REG_60);

	return !!(reg & mask);
}

void cif_clear_csi_int_sts(struct cif_ctx *ctx)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_04, CSI_INTR_CLR,
			     0xFF);
}

void cif_mask_csi_int_sts(struct cif_ctx *ctx, uint32_t mask)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_04, CSI_INTR_MASK,
			     0xFF);
}

void cif_unmask_csi_int_sts(struct cif_ctx *ctx, uint32_t mask)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	CIF_WR_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
			     REG_04, CSI_INTR_MASK,
			     0x00);
}

int cif_check_csi_fifo_full(struct cif_ctx *ctx)
{
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];

	return !!CIF_RD_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				      REG_40, CSI_FIFO_FULL);
}

int cif_get_csi_decode_fmt(struct cif_ctx *ctx)
{
	int i;
	uintptr_t mac_csi = ctx->mac_phys_regs[CIF_MAC_BLK_ID_CSI];
	uint32_t value = CIF_RD_BITS(mac_csi, REG_CSI_CTRL_TOP_T,
				     REG_40, CSI_DECODE_FORMAT);

	for (i = 0; i < DEC_FMT_NUM; i++) {
		if (value & (1 << i))
			return i;
	}

	return i;
}

int cif_get_csi_phy_state(struct cif_ctx *ctx, union mipi_phy_state *state)
{
	uintptr_t wrap_4l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_4L];
	uintptr_t wrap_2l = ctx->wrap_phys_regs[CIF_WRAP_BLK_ID_2L];

	if (!ctx->mac_num) {
		state->raw = CIF_RD_REG(wrap_4l, REG_SENSOR_PHY_4L_T, DBG_90) |
			     CIF_RD_REG(wrap_4l, REG_SENSOR_PHY_4L_T, DBG_94) << 16;
	} else {
		state->raw = CIF_RD_REG(wrap_2l, REG_SENSOR_PHY_2L_T, DBG_90) |
			     CIF_RD_REG(wrap_2l, REG_SENSOR_PHY_2L_T, DBG_94) << 16;
	}

	return 0;
}

uintptr_t *cif_get_mac_phys_reg_bases(uint32_t link)
{
	static uintptr_t m_cif_mac_phys_base_list[MAX_LINK_NUM]
		[CIF_MAC_BLK_ID_MAX];

	m_cif_mac_phys_base_list[link][CIF_MAC_BLK_ID_TOP] =
					(CIF_MAC_BLK_BA_TOP);
	m_cif_mac_phys_base_list[link][CIF_MAC_BLK_ID_SLVDS] =
					(CIF_MAC_BLK_BA_SLVDS);
	m_cif_mac_phys_base_list[link][CIF_MAC_BLK_ID_CSI] =
					(CIF_MAC_BLK_BA_CSI);
	m_cif_mac_phys_base_list[link][CIF_MAC_BLK_ID_MAC] =
					(CIF_MAC_BLK_BA_MAC);

	return m_cif_mac_phys_base_list[link];
}

uintptr_t *cif_get_wrap_phys_reg_bases(uint32_t link)
{
	static uintptr_t m_cif_wrap_phys_base_list[MAX_LINK_NUM]
		[CIF_WRAP_BLK_ID_MAX];

	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_TOP] =
					(CIF_WRAP_BLK_BA_TOP);
	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_4L] =
					(CIF_WRAP_BLK_BA_4L);
	m_cif_wrap_phys_base_list[link][CIF_WRAP_BLK_ID_2L] =
					(CIF_WRAP_BLK_BA_2L);

	return m_cif_wrap_phys_base_list[link];
}

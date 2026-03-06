//! MIPI CSI CIF 寄存器与驱动逻辑（由 mipi-rx/drv/cif_drv.c 翻译，使用 tock-registers register_structs! 风格）

use tock_registers::interfaces::{Readable, ReadWriteable};
use tock_registers::register_bitfields;
use tock_registers::register_structs;
use tock_registers::registers::{ReadOnly, ReadWrite};

// ============== 寄存器位域（与 C 头 reg_fields_csi_mac / reg_fields_csi_wrap 对应） ==============

register_bitfields![u32,
    pub SensorMac00 [
        SENSOR_MAC_MODE OFFSET(0) NUMBITS(3) [],
        CSI_CTRL_ENABLE OFFSET(4) NUMBITS(1) [],
        CSI_VS_INV OFFSET(5) NUMBITS(1) [],
        CSI_HS_INV OFFSET(6) NUMBITS(1) [],
        SW_UP OFFSET(17) NUMBITS(1) [],
    ],
    pub SensorMac40 [
        SENSOR_MAC_HDR_EN OFFSET(0) NUMBITS(1) [],
        SENSOR_MAC_HDR_HDR0INV OFFSET(4) NUMBITS(1) [],
        SENSOR_MAC_HDR_HDR1INV OFFSET(5) NUMBITS(1) [],
        SENSOR_MAC_HDR_MODE OFFSET(8) NUMBITS(1) [],
    ],
    pub SensorMac44 [
        SENSOR_MAC_HDR_SHIFT OFFSET(0) NUMBITS(13) [],
        SENSOR_MAC_HDR_VSIZE OFFSET(16) NUMBITS(13) [],
    ],
    pub SensorMac48 [
        SENSOR_MAC_INFO_LINE_NUM OFFSET(0) NUMBITS(13) [],
        SENSOR_MAC_RM_INFO_LINE OFFSET(16) NUMBITS(1) [],
    ],
    pub SensorMacB0 [
        SENSOR_MAC_CROP_START_X OFFSET(0) NUMBITS(13) [],
        SENSOR_MAC_CROP_END_X OFFSET(16) NUMBITS(13) [],
        SENSOR_MAC_CROP_EN OFFSET(30) NUMBITS(1) [],
    ],
    pub SensorMacB4 [
        SENSOR_MAC_CROP_START_Y OFFSET(0) NUMBITS(13) [],
        SENSOR_MAC_CROP_END_Y OFFSET(16) NUMBITS(13) [],
    ],
    pub SensorMacB8 [
        SENSOR_MAC_SWAPUV_EN OFFSET(0) NUMBITS(1) [],
        SENSOR_MAC_SWAPYC_EN OFFSET(1) NUMBITS(1) [],
    ],
];

register_bitfields![u32,
    pub CsiCtrl00 [ CSI_LANE_MODE OFFSET(0) NUMBITS(3) [], ],
    pub CsiCtrl04 [
        CSI_INTR_MASK OFFSET(0) NUMBITS(8) [],
        CSI_INTR_CLR OFFSET(8) NUMBITS(8) [],
        CSI_HDR_EN OFFSET(16) NUMBITS(1) [],
        CSI_HDR_MODE OFFSET(17) NUMBITS(1) [],
        CSI_ID_RM_ELSE OFFSET(18) NUMBITS(1) [],
        CSI_ID_RM_OB OFFSET(19) NUMBITS(1) [],
    ],
    pub CsiCtrl18 [
        CSI_VC_MAP_CH00 OFFSET(0) NUMBITS(4) [],
        CSI_VC_MAP_CH01 OFFSET(4) NUMBITS(4) [],
        CSI_VC_MAP_CH10 OFFSET(8) NUMBITS(4) [],
        CSI_VC_MAP_CH11 OFFSET(12) NUMBITS(4) [],
    ],
    pub CsiCtrl40 [
        CSI_FIFO_FULL OFFSET(8) NUMBITS(1) [],
        CSI_DECODE_FORMAT OFFSET(16) NUMBITS(6) [],
    ],
    pub CsiCtrl60 [ CSI_INTR_STATUS OFFSET(0) NUMBITS(8) [], ],
    pub CsiCtrl70 [ CSI_VS_GEN_MODE OFFSET(0) NUMBITS(2) [], ],
    pub CsiCtrl74 [
        CSI_HDR_DT_MODE OFFSET(0) NUMBITS(1) [],
        CSI_HDR_DT_FORMAT OFFSET(4) NUMBITS(6) [],
        CSI_HDR_DT_LEF OFFSET(12) NUMBITS(6) [],
        CSI_HDR_DT_SEF OFFSET(20) NUMBITS(6) [],
    ],
];

register_bitfields![u32,
    pub Phy4l00 [ SENSOR_MODE OFFSET(0) NUMBITS(2) [], ],
    pub Phy4l04 [
        CSI_LANE_D0_SEL OFFSET(0) NUMBITS(3) [],
        CSI_LANE_D1_SEL OFFSET(4) NUMBITS(3) [],
        CSI_LANE_D2_SEL OFFSET(8) NUMBITS(3) [],
        CSI_LANE_D3_SEL OFFSET(12) NUMBITS(3) [],
    ],
    pub Phy4l08 [
        CSI_LANE_CK_SEL OFFSET(0) NUMBITS(3) [],
        CSI_LANE_CK_PNSWAP OFFSET(4) NUMBITS(1) [],
        CSI_LANE_D0_PNSWAP OFFSET(8) NUMBITS(1) [],
        CSI_LANE_D1_PNSWAP OFFSET(9) NUMBITS(1) [],
        CSI_LANE_D2_PNSWAP OFFSET(10) NUMBITS(1) [],
        CSI_LANE_D3_PNSWAP OFFSET(11) NUMBITS(1) [],
    ],
    pub Phy4l0C [ DESKEW_LANE_EN OFFSET(0) NUMBITS(4) [], ],
    pub Phy4l10 [
        AUTO_IGNORE OFFSET(16) NUMBITS(1) [],
        AUTO_SYNC OFFSET(17) NUMBITS(1) [],
    ],
    pub Phy2l00 [ SENSOR_MODE OFFSET(0) NUMBITS(2) [], ],
    pub Phy2l04 [
        CSI_LANE_D0_SEL OFFSET(0) NUMBITS(2) [],
        CSI_LANE_D1_SEL OFFSET(4) NUMBITS(2) [],
    ],
    pub Phy2l08 [
        CSI_LANE_CK_SEL OFFSET(0) NUMBITS(2) [],
        CSI_LANE_CK_PNSWAP OFFSET(4) NUMBITS(1) [],
        CSI_LANE_D0_PNSWAP OFFSET(8) NUMBITS(1) [],
        CSI_LANE_D1_PNSWAP OFFSET(9) NUMBITS(1) [],
    ],
    pub Phy2l0C [ DESKEW_LANE_EN OFFSET(0) NUMBITS(2) [], ],
    pub Phy2l10 [
        AUTO_IGNORE OFFSET(16) NUMBITS(1) [],
        AUTO_SYNC OFFSET(17) NUMBITS(1) [],
    ],
];

register_bitfields![u32,
    pub PhyTop00 [
        MIPIRX_PD_IBIAS OFFSET(12) NUMBITS(1) [],
        MIPIRX_PD_RXLP OFFSET(14) NUMBITS(6) [],
    ],
    pub PhyTop04 [
        MIPIRX_SEL_CLK_CHANNEL OFFSET(16) NUMBITS(6) [],
        MIPIRX_SEL_CLK_P0TOP1 OFFSET(6) NUMBITS(1) [],
        MIPIRX_SEL_CLK_P1TOP0 OFFSET(7) NUMBITS(1) [],
    ],
    pub PhyTop70 [
        AD_D0_DATA OFFSET(0) NUMBITS(8) [],
        AD_D1_DATA OFFSET(8) NUMBITS(8) [],
        AD_D2_DATA OFFSET(16) NUMBITS(8) [],
        AD_D3_DATA OFFSET(24) NUMBITS(8) [],
    ],
    pub PhyTop74 [
        AD_D4_DATA OFFSET(0) NUMBITS(8) [],
        AD_D5_DATA OFFSET(8) NUMBITS(8) [],
    ],
    pub PhyTop80 [
        AD_D0_CLK_INV OFFSET(0) NUMBITS(1) [],
        AD_D1_CLK_INV OFFSET(1) NUMBITS(1) [],
        AD_D2_CLK_INV OFFSET(2) NUMBITS(1) [],
        AD_D3_CLK_INV OFFSET(3) NUMBITS(1) [],
        AD_D4_CLK_INV OFFSET(4) NUMBITS(1) [],
        AD_D5_CLK_INV OFFSET(5) NUMBITS(1) [],
        FORCE_DESKEW_CODE0 OFFSET(16) NUMBITS(1) [],
        FORCE_DESKEW_CODE1 OFFSET(17) NUMBITS(1) [],
        FORCE_DESKEW_CODE2 OFFSET(18) NUMBITS(1) [],
        FORCE_DESKEW_CODE3 OFFSET(19) NUMBITS(1) [],
        FORCE_DESKEW_CODE4 OFFSET(20) NUMBITS(1) [],
        FORCE_DESKEW_CODE5 OFFSET(21) NUMBITS(1) [],
    ],
    pub PhyTop84 [
        DESKEW_CODE0 OFFSET(0) NUMBITS(8) [],
        DESKEW_CODE1 OFFSET(8) NUMBITS(8) [],
        DESKEW_CODE2 OFFSET(16) NUMBITS(8) [],
        DESKEW_CODE3 OFFSET(24) NUMBITS(8) [],
    ],
    pub PhyTop88 [
        DESKEW_CODE4 OFFSET(0) NUMBITS(8) [],
        DESKEW_CODE5 OFFSET(8) NUMBITS(8) [],
    ],
];

// ============== 寄存器块结构（与 C reg_blocks 布局一致，仅包含使用的寄存器） ==============

register_structs! {
    /// SENSOR_MAC 块（REG_SENSOR_MAC_T 中使用到的部分）
    pub SensorMacRegs {
        (0x00 => reg_00: ReadWrite<u32, SensorMac00::Register>),
        (0x04 => _reserved_04),
        (0x40 => reg_40: ReadWrite<u32, SensorMac40::Register>),
        (0x44 => reg_44: ReadWrite<u32, SensorMac44::Register>),
        (0x48 => reg_48: ReadWrite<u32, SensorMac48::Register>),
        (0x4C => _reserved_4c),
        (0xB0 => reg_b0: ReadWrite<u32, SensorMacB0::Register>),
        (0xB4 => reg_b4: ReadWrite<u32, SensorMacB4::Register>),
        (0xB8 => reg_b8: ReadWrite<u32, SensorMacB8::Register>),
        (0xBC => @END),
    }
}

register_structs! {
    /// CSI_CTRL_TOP 块（REG_CSI_CTRL_TOP_T 中使用到的部分）
    pub CsiCtrlTopRegs {
        (0x00 => reg_00: ReadWrite<u32, CsiCtrl00::Register>),
        (0x04 => reg_04: ReadWrite<u32, CsiCtrl04::Register>),
        (0x08 => _reserved_08),
        (0x18 => reg_18: ReadWrite<u32, CsiCtrl18::Register>),
        (0x1C => _reserved_1c),
        (0x40 => reg_40: ReadWrite<u32, CsiCtrl40::Register>),
        (0x44 => _reserved_44),
        (0x60 => reg_60: ReadOnly<u32, CsiCtrl60::Register>),
        (0x64 => _reserved_64),
        (0x70 => reg_70: ReadWrite<u32, CsiCtrl70::Register>),
        (0x74 => reg_74: ReadWrite<u32, CsiCtrl74::Register>),
        (0x78 => @END),
    }
}

register_structs! {
    /// SENSOR_PHY_4L 块（REG_SENSOR_PHY_4L_T 前 0x14 字节）
    pub SensorPhy4lRegs {
        (0x00 => reg_00: ReadWrite<u32, Phy4l00::Register>),
        (0x04 => reg_04: ReadWrite<u32, Phy4l04::Register>),
        (0x08 => reg_08: ReadWrite<u32, Phy4l08::Register>),
        (0x0C => reg_0c: ReadWrite<u32, Phy4l0C::Register>),
        (0x10 => reg_10: ReadWrite<u32, Phy4l10::Register>),
        (0x14 => @END),
    }
}

register_structs! {
    /// SENSOR_PHY_2L 块（REG_SENSOR_PHY_2L_T 前 0x14 字节）
    pub SensorPhy2lRegs {
        (0x00 => reg_00: ReadWrite<u32, Phy2l00::Register>),
        (0x04 => reg_04: ReadWrite<u32, Phy2l04::Register>),
        (0x08 => reg_08: ReadWrite<u32, Phy2l08::Register>),
        (0x0C => reg_0c: ReadWrite<u32, Phy2l0C::Register>),
        (0x10 => reg_10: ReadWrite<u32, Phy2l10::Register>),
        (0x14 => @END),
    }
}

register_structs! {
    /// SENSOR_PHY_TOP 块（REG_SENSOR_PHY_TOP_T 中使用到的部分）
    pub SensorPhyTopRegs {
        (0x00 => reg_00: ReadWrite<u32, PhyTop00::Register>),
        (0x04 => reg_04: ReadWrite<u32, PhyTop04::Register>),
        (0x08 => _reserved_08),
        (0x70 => reg_70: ReadOnly<u32, PhyTop70::Register>),
        (0x74 => reg_74: ReadOnly<u32, PhyTop74::Register>),
        (0x78 => _reserved_78),
        (0x80 => reg_80: ReadWrite<u32, PhyTop80::Register>),
        (0x84 => reg_84: ReadWrite<u32, PhyTop84::Register>),
        (0x88 => reg_88: ReadWrite<u32, PhyTop88::Register>),
        (0x8C => @END),
    }
}

// ============== CSI 参数与上下文 ==============

/// CSI 虚拟通道映射（4 个 VC）
pub type VcMapping = [u8; 4];

/// CSI 配置（对应 C param_csi）
#[derive(Clone, Debug)]
pub struct CsiConfig {
    pub lane_num: u16,
    pub vs_gen_mode: u32,
    pub hdr_mode: CsiHdrMode,
    pub data_type: [u16; 2],
    pub decode_type: u16,
    pub vc_mapping: VcMapping,
}

/// CSI HDR 模式（对应 C enum csi_hdr_mode）
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum CsiHdrMode {
    Vc = 0,
    Id = 1,
    Dt = 2,
    Dol = 3,
}

/// 裁剪区域（对应 C struct cif_crop_s）
#[derive(Clone, Copy, Debug, Default)]
pub struct Crop {
    pub enable: u32,
    pub x: u32,
    pub y: u32,
    pub w: u32,
    pub h: u32,
}

/// CIF 上下文：持有 MAC/Wrap 基址，对应 C struct cif_ctx
#[derive(Clone, Copy)]
pub struct CifContext {
    pub mac_top: usize,
    pub mac_csi: usize,
    pub wrap_top: usize,
    pub wrap_4l: usize,
    pub wrap_2l: usize,
    /// true = 2L PHY，false = 4L PHY（对应 ctx->mac_num != 0）
    pub is_2l: bool,
}

impl CifContext {
    /// 从 C 的 mac_phys_regs / wrap_phys_regs 数组构造
    pub fn from_bases(
        mac_phys_regs: &[usize; 4],
        wrap_phys_regs: &[usize; 3],
        mac_num: u16,
    ) -> Self {
        Self {
            mac_top: mac_phys_regs[0],
            mac_csi: mac_phys_regs[2],
            wrap_top: wrap_phys_regs[0],
            wrap_4l: wrap_phys_regs[1],
            wrap_2l: wrap_phys_regs[2],
            is_2l: mac_num != 0,
        }
    }

    #[inline]
    fn mac_top_regs(&self) -> &SensorMacRegs {
        unsafe { &*(self.mac_top as *const SensorMacRegs) }
    }

    #[inline]
    fn mac_csi_regs(&self) -> &CsiCtrlTopRegs {
        unsafe { &*(self.mac_csi as *const CsiCtrlTopRegs) }
    }

    #[inline]
    fn wrap_top_regs(&self) -> &SensorPhyTopRegs {
        unsafe { &*(self.wrap_top as *const SensorPhyTopRegs) }
    }

    #[inline]
    fn wrap_4l_regs(&self) -> &SensorPhy4lRegs {
        unsafe { &*(self.wrap_4l as *const SensorPhy4lRegs) }
    }

    #[inline]
    fn wrap_2l_regs(&self) -> &SensorPhy2lRegs {
        unsafe { &*(self.wrap_2l as *const SensorPhy2lRegs) }
    }

    /// CSI 配置：sensor mode、HS/VS 反转、lane、VC mapping、DPHY sensor mode
    pub fn config_csi(&self, param: &CsiConfig) {
        let mac = self.mac_top_regs();
        mac.reg_00.modify(
            SensorMac00::SENSOR_MAC_MODE.val(1)
                + SensorMac00::CSI_VS_INV.val(1)
                + SensorMac00::CSI_HS_INV.val(1)
                + SensorMac00::CSI_CTRL_ENABLE.val(1),
        );

        let csi = self.mac_csi_regs();
        csi.reg_00.modify(CsiCtrl00::CSI_LANE_MODE.val((param.lane_num - 1) as u32));
        csi.reg_70.modify(CsiCtrl70::CSI_VS_GEN_MODE.val(param.vs_gen_mode));

        if self.is_2l {
            let wp = self.wrap_2l_regs();
            wp.reg_10.modify(Phy2l10::AUTO_IGNORE.val(0) + Phy2l10::AUTO_SYNC.val(0));
            wp.reg_00.modify(Phy2l00::SENSOR_MODE.val(0));
        } else {
            let wp = self.wrap_4l_regs();
            wp.reg_10.modify(Phy4l10::AUTO_IGNORE.val(0) + Phy4l10::AUTO_SYNC.val(0));
            wp.reg_00.modify(Phy4l00::SENSOR_MODE.val(0));
        }

        csi.reg_18.modify(
            CsiCtrl18::CSI_VC_MAP_CH00.val(param.vc_mapping[0] as u32)
                + CsiCtrl18::CSI_VC_MAP_CH01.val(param.vc_mapping[1] as u32)
                + CsiCtrl18::CSI_VC_MAP_CH10.val(param.vc_mapping[2] as u32)
                + CsiCtrl18::CSI_VC_MAP_CH11.val(param.vc_mapping[3] as u32),
        );

        log::debug!("mipi: config_csi lane={} vs_gen={}", param.lane_num, param.vs_gen_mode);
    }

    /// 裁剪使能/区域（SENSOR_MAC crop）
    pub fn set_crop(&self, crop: &Crop) {
        if crop.w == 0 || crop.h == 0 {
            return;
        }
        let mac = self.mac_top_regs();
        mac.reg_b0.modify(
            SensorMacB0::SENSOR_MAC_CROP_START_X.val(crop.x)
                + SensorMacB0::SENSOR_MAC_CROP_END_X.val(crop.x + crop.w)
                + SensorMacB0::SENSOR_MAC_CROP_EN.val(crop.enable),
        );
        mac.reg_b4.modify(
            SensorMacB4::SENSOR_MAC_CROP_START_Y.val(crop.y)
                + SensorMacB4::SENSOR_MAC_CROP_END_Y.val(crop.y + crop.h),
        );
    }

    /// YUV 交换（UV / YC）
    pub fn swap_yuv(&self, uv_swap: bool, yc_swap: bool) {
        self.mac_top_regs().reg_b8.modify(
            SensorMacB8::SENSOR_MAC_SWAPUV_EN.val(uv_swap as u32)
                + SensorMacB8::SENSOR_MAC_SWAPYC_EN.val(yc_swap as u32),
        );
    }

    /// 使能/关闭 CSI 流：PHY 上下电、lane enable
    pub fn stream_enable(&self, csi: &CsiConfig, on: bool) {
        let top = self.wrap_top_regs();
        if on {
            top.reg_00.modify(
                PhyTop00::MIPIRX_PD_IBIAS.val(0) + PhyTop00::MIPIRX_PD_RXLP.val(0),
            );
        } else {
            top.reg_00.modify(
                PhyTop00::MIPIRX_PD_IBIAS.val(1) + PhyTop00::MIPIRX_PD_RXLP.val(0x3F),
            );
        }

        if self.is_2l {
            let wp = self.wrap_2l_regs();
            wp.reg_0c.modify(Phy2l0C::DESKEW_LANE_EN.val(0));
            if on {
                let lane_en = (1u32 << csi.lane_num) - 1;
                wp.reg_0c.modify(Phy2l0C::DESKEW_LANE_EN.val(lane_en));
            }
        } else {
            let wp = self.wrap_4l_regs();
            wp.reg_0c.modify(Phy4l0C::DESKEW_LANE_EN.val(0));
            if on {
                let lane_en = (1u32 << csi.lane_num) - 1;
                wp.reg_0c.modify(Phy4l0C::DESKEW_LANE_EN.val(lane_en));
            }
        }
    }

    /// HDR CSI 使能（VC/DT/DOL 等）
    pub fn hdr_csi_enable(&self, param: &CsiConfig, on: bool) {
        let csi = self.mac_csi_regs();
        match param.hdr_mode {
            CsiHdrMode::Vc => {
                csi.reg_04.modify(CsiCtrl04::CSI_HDR_MODE.val(0));
            }
            CsiHdrMode::Dt => {
                csi.reg_74.modify(
                    CsiCtrl74::CSI_HDR_DT_MODE.val(1)
                        + CsiCtrl74::CSI_HDR_DT_LEF.val(param.data_type[0] as u32)
                        + CsiCtrl74::CSI_HDR_DT_SEF.val(param.data_type[1] as u32)
                        + CsiCtrl74::CSI_HDR_DT_FORMAT.val(param.decode_type as u32),
                );
            }
            CsiHdrMode::Dol => {
                csi.reg_04.modify(
                    CsiCtrl04::CSI_HDR_MODE.val(1)
                        + CsiCtrl04::CSI_ID_RM_ELSE.val(1)
                        + CsiCtrl04::CSI_ID_RM_OB.val(1),
                );
            }
            CsiHdrMode::Id => {
                csi.reg_04.modify(CsiCtrl04::CSI_HDR_MODE.val(1));
            }
        }
        csi.reg_04.modify(CsiCtrl04::CSI_HDR_EN.val(on as u32));
    }

    /// 手动 HDR 配置（SENSOR_MAC HDR 相关）
    pub fn hdr_manual_config(
        &self,
        hdr_en: bool,
        hdr_vsize: u16,
        hdr_shift: u16,
        hdr_rm_padding: bool,
        sw_up: bool,
    ) {
        let mac = self.mac_top_regs();
        if !hdr_en {
            mac.reg_40.modify(
                SensorMac40::SENSOR_MAC_HDR_EN.val(0)
                    + SensorMac40::SENSOR_MAC_HDR_HDR0INV.val(0)
                    + SensorMac40::SENSOR_MAC_HDR_HDR1INV.val(0)
                    + SensorMac40::SENSOR_MAC_HDR_MODE.val(0),
            );
            if sw_up {
                mac.reg_00.modify(SensorMac00::SW_UP.val(1));
            }
            return;
        }
        mac.reg_44.modify(
            SensorMac44::SENSOR_MAC_HDR_VSIZE.val(hdr_vsize as u32)
                + SensorMac44::SENSOR_MAC_HDR_SHIFT.val(hdr_shift as u32),
        );
        mac.reg_40.modify(
            SensorMac40::SENSOR_MAC_HDR_EN.val(1)
                + SensorMac40::SENSOR_MAC_HDR_HDR0INV.val(0)
                + SensorMac40::SENSOR_MAC_HDR_HDR1INV.val(0)
                + SensorMac40::SENSOR_MAC_HDR_MODE.val(hdr_rm_padding as u32),
        );
        if sw_up {
            mac.reg_00.modify(SensorMac00::SW_UP.val(1));
        }
    }

    /// Info line 裁剪（HDR pattern 2）
    pub fn crop_info_line(&self, line_num: u32, sw_up: bool) {
        let mac = self.mac_top_regs();
        if line_num != 0 {
            mac.reg_48.modify(
                SensorMac48::SENSOR_MAC_INFO_LINE_NUM.val(line_num)
                    + SensorMac48::SENSOR_MAC_RM_INFO_LINE.val(1),
            );
            if sw_up {
                mac.reg_00.modify(SensorMac00::SW_UP.val(1));
            }
        } else {
            mac.reg_48.modify(SensorMac48::SENSOR_MAC_RM_INFO_LINE.val(0));
        }
    }

    /// 清除 CSI 中断状态
    pub fn clear_csi_int_sts(&self) {
        self.mac_csi_regs()
            .reg_04
            .modify(CsiCtrl04::CSI_INTR_CLR.val(0xFF));
    }

    /// 屏蔽 CSI 中断
    pub fn mask_csi_int_sts(&self) {
        self.mac_csi_regs()
            .reg_04
            .modify(CsiCtrl04::CSI_INTR_MASK.val(0xFF));
    }

    /// 解除屏蔽 CSI 中断
    pub fn unmask_csi_int_sts(&self) {
        self.mac_csi_regs()
            .reg_04
            .modify(CsiCtrl04::CSI_INTR_MASK.val(0x00));
    }

    /// 检查 CSI 中断状态（mask 为 CIF_INT_STS_*_MASK）
    pub fn check_csi_int_sts(&self, mask: u32) -> bool {
        (self.mac_csi_regs().reg_60.get() & mask) != 0
    }

    /// CSI FIFO 是否满
    pub fn check_csi_fifo_full(&self) -> bool {
        self.mac_csi_regs()
            .reg_40
            .read(CsiCtrl40::CSI_FIFO_FULL) != 0
    }

    /// 获取 CSI 解码格式（DEC_FMT_* 索引）
    pub fn get_csi_decode_fmt(&self) -> u32 {
        let v = self.mac_csi_regs().reg_40.read(CsiCtrl40::CSI_DECODE_FORMAT);
        for i in 0..6 {
            if (v & (1 << i)) != 0 {
                return i;
            }
        }
        6
    }

    /// 时钟方向（P0->P1 / P1->P0 / Freerun）
    pub fn set_clk_dir(&self, p0_to_p1: bool, p1_to_p0: bool) {
        self.wrap_top_regs().reg_04.modify(
            PhyTop04::MIPIRX_SEL_CLK_P1TOP0.val(p1_to_p0 as u32)
                + PhyTop04::MIPIRX_SEL_CLK_P0TOP1.val(p0_to_p1 as u32),
        );
    }

    /// 某 PHY lane 的 AD 数据（0..=5）
    pub fn get_lane_data(&self, lane: u32) -> u8 {
        let top = self.wrap_top_regs();
        match lane {
            0 => top.reg_70.read(PhyTop70::AD_D0_DATA) as u8,
            1 => top.reg_70.read(PhyTop70::AD_D1_DATA) as u8,
            2 => top.reg_70.read(PhyTop70::AD_D2_DATA) as u8,
            3 => top.reg_70.read(PhyTop70::AD_D3_DATA) as u8,
            4 => top.reg_74.read(PhyTop74::AD_D4_DATA) as u8,
            5 => top.reg_74.read(PhyTop74::AD_D5_DATA) as u8,
            _ => 0,
        }
    }

    /// 设置 lane deskew 相位（lane 0..=5）
    pub fn set_lane_deskew(&self, lane: u32, phase: u8) {
        let top = self.wrap_top_regs();
        let force = (phase != 0) as u32;
        match lane {
            0 | 1 | 2 | 3 => {
                top.reg_84.modify(match lane {
                    0 => PhyTop84::DESKEW_CODE0.val(phase as u32),
                    1 => PhyTop84::DESKEW_CODE1.val(phase as u32),
                    2 => PhyTop84::DESKEW_CODE2.val(phase as u32),
                    _ => PhyTop84::DESKEW_CODE3.val(phase as u32),
                });
            }
            4 | 5 => {
                top.reg_88.modify(if lane == 4 {
                    PhyTop88::DESKEW_CODE4.val(phase as u32)
                } else {
                    PhyTop88::DESKEW_CODE5.val(phase as u32)
                });
            }
            _ => return,
        }
        top.reg_80.modify(match lane {
            0 => PhyTop80::FORCE_DESKEW_CODE0.val(force),
            1 => PhyTop80::FORCE_DESKEW_CODE1.val(force),
            2 => PhyTop80::FORCE_DESKEW_CODE2.val(force),
            3 => PhyTop80::FORCE_DESKEW_CODE3.val(force),
            4 => PhyTop80::FORCE_DESKEW_CODE4.val(force),
            5 => PhyTop80::FORCE_DESKEW_CODE5.val(force),
            _ => return,
        });
    }

    /// 设置 PHY lane 时钟沿（0=rising, 1=falling）
    pub fn set_clk_edge(&self, lane: u32, falling: bool) {
        let v = falling as u32;
        let r = match lane {
            0 => PhyTop80::AD_D0_CLK_INV.val(v),
            1 => PhyTop80::AD_D1_CLK_INV.val(v),
            2 => PhyTop80::AD_D2_CLK_INV.val(v),
            3 => PhyTop80::AD_D3_CLK_INV.val(v),
            4 => PhyTop80::AD_D4_CLK_INV.val(v),
            5 => PhyTop80::AD_D5_CLK_INV.val(v),
            _ => return,
        };
        self.wrap_top_regs().reg_80.modify(r);
    }
}

// 中断状态掩码（与 C 头一致）
pub const CIF_INT_STS_ECC_ERR_MASK: u32 = 1 << 0;
pub const CIF_INT_STS_CRC_ERR_MASK: u32 = 1 << 1;
pub const CIF_INT_STS_HDR_ERR_MASK: u32 = 1 << 2;
pub const CIF_INT_STS_WC_ERR_MASK: u32 = 1 << 3;
pub const CIF_INT_STS_FIFO_FULL_MASK: u32 = 1 << 4;

use crate::{pr_info, utils::ioremap};
use sg200x_bsp::{
    gpio::{self, GpioPin, RTCSYS_GPIO_BASE},
    pinmux::{self, FMUX_BASE, FMUX_PWR_GPIO1, IOBLK_G1_BASE, IOBLK_G7_BASE, IOBLK_G10_BASE, IOBLK_G12_BASE, IOBLK_GRTC_BASE},
};
use tock_registers::interfaces::{Readable, ReadWriteable, Writeable};
/// struct clk handle values returned by devm_clk_get (printed via %p in cif.c probe):
///   cam0         clk installed 00000000a32e40e1
///   cam1         clk installed 000000008b33fa8c
///   vip_sys_2    clk installed 000000006d27470d
///   clk_mipimpll clk installed 00000000b15fefd9
///   clk_disppll  clk installed 000000000830eb5c
pub const CLK_CAM0: u64 = 0xa32e40e1;
pub const CLK_CAM1: u64 = 0x8b33fa8c;
pub const CLK_SYS2: u64 = 0x6d27470d;
pub const CLK_MIPIMPLL: u64 = 0xb15fefd9;
pub const CLK_DISPPLL: u64 = 0x0830eb5c;
pub const CLK_FPLL: u64 = 0x39F476A3;

pub const MAC_PHYS_SIZE: usize = 0x2000;
pub const WRAP_PHYS_SIZE: usize = 0x1000;

pub const MAC0_PTR: *mut u8 = 0xA0C2000 as *mut u8;
pub const MAC1_PTR: *mut u8 = 0xA0C4000 as *mut u8;
pub const MAC2_PTR: *mut u8 = 0xA0C6000 as *mut u8;
pub const WRAP_PTR: *mut u8 = 0xA0D0000 as *mut u8;

pub const IRQ_CIF0_NUM: u32 = 30;
pub const IRQ_CIF1_NUM: u32 = 31;

#[repr(C)]
pub struct MipiInitParams {
    pub mac0_ptr: *mut u8,
    pub mac1_ptr: *mut u8,
    pub mac2_ptr: *mut u8,
    pub wrap_ptr: *mut u8,
}

#[unsafe(no_mangle)]
pub extern "C" fn mipi_init() -> i32 {
    let params: MipiInitParams = MipiInitParams {
        mac0_ptr: ioremap(MAC0_PTR, MAC_PHYS_SIZE),
        mac1_ptr: ioremap(MAC1_PTR, MAC_PHYS_SIZE),
        mac2_ptr: ioremap(MAC2_PTR, MAC_PHYS_SIZE),
        wrap_ptr: ioremap(WRAP_PTR, WRAP_PHYS_SIZE),
    };
    log::info!("[mipi_init] mac0 IOREMAP: {:#p}", params.mac0_ptr);
    log::info!("[mipi_init] mac1 IOREMAP: {:#p}", params.mac1_ptr);
    log::info!("[mipi_init] mac2 IOREMAP: {:#p}", params.mac2_ptr);
    log::info!("[mipi_init] wrap IOREMAP: {:#p}", params.wrap_ptr);
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn mipi_init_gpio() -> i32 {
    let kaddr = ioremap(FMUX_BASE as *const u8, 0x1000) as usize;
    let pinmux = unsafe {
        pinmux::Pinmux::from_base_addresses(
            kaddr,
            kaddr + IOBLK_G1_BASE - FMUX_BASE,
            kaddr + IOBLK_G7_BASE - FMUX_BASE,
            kaddr + IOBLK_G10_BASE - FMUX_BASE,
            kaddr + IOBLK_G12_BASE - FMUX_BASE,
            kaddr + IOBLK_GRTC_BASE - FMUX_BASE,
        )
    };
    pinmux.fmux.pwr_gpio1.write(FMUX_PWR_GPIO1::FSEL::PWR_GPIO_1);
    let rst_gpio = unsafe {
        gpio::Gpio::from_base_address(
            ioremap(RTCSYS_GPIO_BASE as *const u8, 0x1000) as usize,
            gpio::GpioPort::RtcSysGpio,
        )
    };
    let rst_pin = GpioPin::new(&rst_gpio, 1);
    rst_pin.set_direction(gpio::Direction::Output);
    log::info!("========== rst_pin lvl: {}", rst_pin.read());
    // rst_pin.set(true);
    rst_pin.set(false);
    log::info!("========== rst_pin lvl: {}", rst_pin.read());

    // rst_pin.set_direction(, direction);
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn mipi_exit() -> i32 {
    0
}

/*
 * Copyright 2018-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2018 Jared Boone
 *
 * This file is part of HackRF.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "portapack.h"

#include "hackrf_core.h"
#include "platform_scu.h"
#include "gpio_lpc.h"
#include "delay.h"

static void portapack_sleep_milliseconds(const uint32_t milliseconds)
{
	/* NOTE: Naively assumes 204 MHz instruction cycle clock and five instructions per count */
	delay(milliseconds * 40800);
}

// clang-format off
static struct gpio_t gpio_io_stbx = GPIO(5,  0); /* P2_0 */
static struct gpio_t gpio_addr    = GPIO(5,  1); /* P2_1 */
__attribute__((unused))
static struct gpio_t gpio_lcd_te  = GPIO(5,  3); /* P2_3 */
__attribute__((unused))
static struct gpio_t gpio_unused  = GPIO(5,  7); /* P2_8 */
static struct gpio_t gpio_lcd_rdx = GPIO(5,  4); /* P2_4 */
static struct gpio_t gpio_lcd_wrx = GPIO(1, 10); /* P2_9 */
static struct gpio_t gpio_dir     = GPIO(1, 13); /* P2_13 */

// clang-format on

typedef struct portapack_if_t {
	gpio_t gpio_dir;
	gpio_t gpio_lcd_rdx;
	gpio_t gpio_lcd_wrx;
	gpio_t gpio_io_stbx;
	gpio_t gpio_addr;
	gpio_port_t* const gpio_port_data;
	uint8_t io_reg;
} portapack_if_t;

static portapack_if_t portapack_if = {
	.gpio_dir = &gpio_dir,
	.gpio_lcd_rdx = &gpio_lcd_rdx,
	.gpio_lcd_wrx = &gpio_lcd_wrx,
	.gpio_io_stbx = &gpio_io_stbx,
	.gpio_addr = &gpio_addr,
	.gpio_port_data = GPIO_LPC_PORT(3),
	.io_reg = 0x03,
};

/* NOTE: Code below assumes the shift value is "8". */
#define GPIO_DATA_SHIFT (8)
static const uint32_t gpio_data_mask = 0xFFU << GPIO_DATA_SHIFT;

static void portapack_data_mask_set(void)
{
	portapack_if.gpio_port_data->mask = ~gpio_data_mask;
}

static void portapack_data_write_low(const uint32_t value)
{
	portapack_if.gpio_port_data->mpin = (value << GPIO_DATA_SHIFT);
}

static void portapack_data_write_high(const uint32_t value)
{
	/* NOTE: Assumes no other bits in the port are masked. */
	/* NOTE: Assumes that bits 15 through 8 are masked. */
	portapack_if.gpio_port_data->mpin = value;
}

static void portapack_dir_read(void)
{
	portapack_if.gpio_port_data->dir &= ~gpio_data_mask;
	gpio_set(portapack_if.gpio_dir);
}

static void portapack_dir_write(void)
{
	gpio_clear(portapack_if.gpio_dir);
	portapack_if.gpio_port_data->dir |= gpio_data_mask;
	/* TODO: Manipulating DIR[3] makes me queasy. The RFFC5072 DATA pin
	 * is also on port 3, and switches direction periodically...
	 * Time to resort to bit-banding to enforce atomicity? But then, how
	 * to change direction on eight bits efficiently? Or do I care, since
	 * the PortaPack data bus shouldn't change direction too frequently?
	 */
}

__attribute__((unused)) static void portapack_lcd_rd_assert(void)
{
	gpio_clear(portapack_if.gpio_lcd_rdx);
}

static void portapack_lcd_rd_deassert(void)
{
	gpio_set(portapack_if.gpio_lcd_rdx);
}

static void portapack_lcd_wr_assert(void)
{
	gpio_clear(portapack_if.gpio_lcd_wrx);
}

static void portapack_lcd_wr_deassert(void)
{
	gpio_set(portapack_if.gpio_lcd_wrx);
}

static void portapack_io_stb_assert(void)
{
	gpio_clear(portapack_if.gpio_io_stbx);
}

static void portapack_io_stb_deassert(void)
{
	gpio_set(portapack_if.gpio_io_stbx);
}

static void portapack_addr(const bool value)
{
	gpio_write(portapack_if.gpio_addr, value);
}

static void portapack_lcd_command(const uint32_t value)
{
	portapack_data_write_high(0); /* Drive high byte (with zero -- don't care) */
	portapack_dir_write();        /* Turn around data bus, MCU->CPLD */
	portapack_addr(0);            /* Indicate command */
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	portapack_lcd_wr_assert(); /* Latch high byte */

	portapack_data_write_low(value); /* Drive low byte (pass-through) */
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	portapack_lcd_wr_deassert(); /* Complete write operation */

	portapack_addr(1); /* Set up for data phase (most likely after a command) */
}

static void portapack_lcd_write_data(const uint32_t value)
{
	// NOTE: Assumes and DIR=0 and ADDR=1 from command phase.
	portapack_data_write_high(value); /* Drive high byte */
	__asm__("nop");
	portapack_lcd_wr_assert(); /* Latch high byte */

	portapack_data_write_low(value); /* Drive low byte (pass-through) */
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	portapack_lcd_wr_deassert(); /* Complete write operation */
}

static void portapack_io_write(const bool address, const uint_fast16_t value)
{
	portapack_data_write_low(value);
	portapack_dir_write();
	portapack_addr(address);
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	portapack_io_stb_assert();
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	portapack_io_stb_deassert();
}

static void portapack_if_init(void)
{
	portapack_data_mask_set();
	portapack_data_write_high(0);

	portapack_dir_read();
	portapack_lcd_rd_deassert();
	portapack_lcd_wr_deassert();
	portapack_io_stb_deassert();
	portapack_addr(0);

	gpio_output(portapack_if.gpio_dir);
	gpio_output(portapack_if.gpio_lcd_rdx);
	gpio_output(portapack_if.gpio_lcd_wrx);
	gpio_output(portapack_if.gpio_io_stbx);
	gpio_output(portapack_if.gpio_addr);
	/* gpio_input(portapack_if.gpio_rot_a); */
	/* gpio_input(portapack_if.gpio_rot_b); */

	scu_pinmux(SCU_PINMUX_PP_D0, SCU_CONF_FUNCTION0 | SCU_GPIO_PDN);
	scu_pinmux(SCU_PINMUX_PP_D1, SCU_CONF_FUNCTION0 | SCU_GPIO_PDN);
	scu_pinmux(SCU_PINMUX_PP_D2, SCU_CONF_FUNCTION0 | SCU_GPIO_PDN);
	scu_pinmux(SCU_PINMUX_PP_D3, SCU_CONF_FUNCTION0 | SCU_GPIO_PDN);
	scu_pinmux(SCU_PINMUX_PP_D4, SCU_CONF_FUNCTION0 | SCU_GPIO_PDN);
	scu_pinmux(SCU_PINMUX_PP_D5, SCU_CONF_FUNCTION0 | SCU_GPIO_PDN);
	scu_pinmux(SCU_PINMUX_PP_D6, SCU_CONF_FUNCTION0 | SCU_GPIO_PDN);
	scu_pinmux(SCU_PINMUX_PP_D7, SCU_CONF_FUNCTION0 | SCU_GPIO_PDN);

	scu_pinmux(SCU_PINMUX_PP_DIR, SCU_CONF_FUNCTION0 | SCU_GPIO_NOPULL);
	scu_pinmux(SCU_PINMUX_PP_LCD_RDX, SCU_CONF_FUNCTION4 | SCU_GPIO_NOPULL);
	scu_pinmux(SCU_PINMUX_PP_LCD_WRX, SCU_CONF_FUNCTION0 | SCU_GPIO_NOPULL);
	scu_pinmux(SCU_PINMUX_PP_IO_STBX, SCU_CONF_FUNCTION4 | SCU_GPIO_NOPULL);
	scu_pinmux(SCU_PINMUX_PP_ADDR, SCU_CONF_FUNCTION4 | SCU_GPIO_NOPULL);
	/* scu_pinmux(SCU_PINMUX_PP_LCD_TE,   SCU_CONF_FUNCTION4 | SCU_GPIO_NOPULL); */
	/* scu_pinmux(SCU_PINMUX_PP_UNUSED,   SCU_CONF_FUNCTION4 | SCU_GPIO_NOPULL); */
}

static void portapack_lcd_reset_state(const bool active)
{
	portapack_if.io_reg = (portapack_if.io_reg & 0xfe) | (active ? (1 << 0) : 0);
	portapack_io_write(1, portapack_if.io_reg);
}

static void portapack_lcd_data_write_command_and_data(
	const uint_fast8_t command,
	const uint8_t* data,
	const size_t data_count)
{
	portapack_lcd_command(command);
	for (size_t i = 0; i < data_count; i++) {
		portapack_lcd_write_data(data[i]);
	}
}

static void portapack_lcd_sleep_out(void)
{
	const uint8_t cmd_11[] = {};
	portapack_lcd_data_write_command_and_data(0x11, cmd_11, ARRAY_SIZEOF(cmd_11));
	// "It will be necessary to wait 120msec after sending Sleep Out
	// command (when in Sleep In Mode) before Sleep In command can be
	// sent."
	portapack_sleep_milliseconds(120);
}

static void portapack_lcd_display_on(void)
{
	const uint8_t cmd_29[] = {};
	portapack_lcd_data_write_command_and_data(0x29, cmd_29, ARRAY_SIZEOF(cmd_29));
}

static void portapack_lcd_ramwr_start(void)
{
	const uint8_t cmd_2c[] = {};
	portapack_lcd_data_write_command_and_data(0x2c, cmd_2c, ARRAY_SIZEOF(cmd_2c));
}

static void portapack_lcd_set(
	const uint_fast8_t command,
	const uint_fast16_t start,
	const uint_fast16_t end)
{
	const uint8_t data[] = {(start >> 8), (start & 0xff), (end >> 8), (end & 0xff)};
	portapack_lcd_data_write_command_and_data(command, data, ARRAY_SIZEOF(data));
}

static void portapack_lcd_caset(
	const uint_fast16_t start_column,
	const uint_fast16_t end_column)
{
	portapack_lcd_set(0x2a, start_column, end_column);
}

static void portapack_lcd_paset(
	const uint_fast16_t start_page,
	const uint_fast16_t end_page)
{
	portapack_lcd_set(0x2b, start_page, end_page);
}

static void portapack_lcd_start_ram_write(const ui_rect_t rect)
{
	portapack_lcd_caset(rect.point.x, rect.point.x + rect.size.width - 1);
	portapack_lcd_paset(rect.point.y, rect.point.y + rect.size.height - 1);
	portapack_lcd_ramwr_start();
}

static void portapack_lcd_write_pixel(const ui_color_t pixel)
{
	portapack_lcd_write_data(pixel.v);
}

static void portapack_lcd_write_pixels_color(const ui_color_t c, size_t n)
{
	while (n--) {
		portapack_lcd_write_data(c.v);
	}
}

static void portapack_lcd_wake(void)
{
	portapack_lcd_sleep_out();
	portapack_lcd_display_on();
}

static void portapack_lcd_reset(void)
{
	portapack_lcd_reset_state(false);
	portapack_sleep_milliseconds(1);
	portapack_lcd_reset_state(true);
	portapack_sleep_milliseconds(10);
	portapack_lcd_reset_state(false);
	portapack_sleep_milliseconds(120);
}

static void portapack_lcd_init(void)
{
	// LCDs are configured for IM[2:0] = 001
	// 8080-I system, 16-bit parallel bus

	//
	// 0x3a: DBI[2:0] = 101
	// MDT[1:0] = XX (if not in 18-bit mode, right?)

	// Power control B
	// 0
	// PCEQ=1, DRV_ena=0, Power control=3
	const uint8_t cmd_cf[] = {0x00, 0xD9, 0x30};
	portapack_lcd_data_write_command_and_data(0xCF, cmd_cf, ARRAY_SIZEOF(cmd_cf));

	// Power on sequence control
	const uint8_t cmd_ed[] = {0x64, 0x03, 0x12, 0x81};
	portapack_lcd_data_write_command_and_data(0xED, cmd_ed, ARRAY_SIZEOF(cmd_ed));

	// Driver timing control A
	const uint8_t cmd_e8[] = {0x85, 0x10, 0x78};
	portapack_lcd_data_write_command_and_data(0xE8, cmd_e8, ARRAY_SIZEOF(cmd_e8));

	// Power control A
	const uint8_t cmd_cb[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
	portapack_lcd_data_write_command_and_data(0xCB, cmd_cb, ARRAY_SIZEOF(cmd_cb));

	// Pump ratio control
	const uint8_t cmd_f7[] = {0x20};
	portapack_lcd_data_write_command_and_data(0xF7, cmd_f7, ARRAY_SIZEOF(cmd_f7));

	// Driver timing control B
	const uint8_t cmd_ea[] = {0x00, 0x00};
	portapack_lcd_data_write_command_and_data(0xEA, cmd_ea, ARRAY_SIZEOF(cmd_ea));

	const uint8_t cmd_b1[] = {0x00, 0x1B};
	portapack_lcd_data_write_command_and_data(0xB1, cmd_b1, ARRAY_SIZEOF(cmd_b1));

	// Blanking Porch Control
	// VFP = 0b0000010 = 2 (number of HSYNC of vertical front porch)
	// VBP = 0b0000010 = 2 (number of HSYNC of vertical back porch)
	// HFP = 0b0001010 = 10 (number of DOTCLOCK of horizontal front porch)
	// HBP = 0b0010100 = 20 (number of DOTCLOCK of horizontal back porch)
	const uint8_t cmd_b5[] = {0x02, 0x02, 0x0a, 0x14};
	portapack_lcd_data_write_command_and_data(0xB5, cmd_b5, ARRAY_SIZEOF(cmd_b5));

	// Display Function Control
	// PT[1:0] = 0b10
	// PTG[1:0] = 0b10
	// ISC[3:0] = 0b0010 (scan cycle interval of gate driver: 5 frames)
	// SM = 0 (gate driver pin arrangement in combination with GS)
	// SS = 1 (source output scan direction S720 -> S1)
	// GS = 0 (gate output scan direction G1 -> G320)
	// REV = 1 (normally white)
	// NL = 0b100111 (default)
	// PCDIV = 0b000000 (default?)
	const uint8_t cmd_b6[] = {0x0A, 0xA2, 0x27, 0x00};
	portapack_lcd_data_write_command_and_data(0xB6, cmd_b6, ARRAY_SIZEOF(cmd_b6));

	// Power Control 1
	//VRH[5:0]
	const uint8_t cmd_c0[] = {0x1B};
	portapack_lcd_data_write_command_and_data(0xC0, cmd_c0, ARRAY_SIZEOF(cmd_c0));

	// Power Control 2
	//SAP[2:0];BT[3:0]
	const uint8_t cmd_c1[] = {0x12};
	portapack_lcd_data_write_command_and_data(0xC1, cmd_c1, ARRAY_SIZEOF(cmd_c1));

	// VCOM Control 1
	const uint8_t cmd_c5[] = {0x32, 0x3C};
	portapack_lcd_data_write_command_and_data(0xC5, cmd_c5, ARRAY_SIZEOF(cmd_c5));

	// VCOM Control 2
	const uint8_t cmd_c7[] = {0x9B};
	portapack_lcd_data_write_command_and_data(0xC7, cmd_c7, ARRAY_SIZEOF(cmd_c7));

	// Memory Access Control
	// Invert X and Y memory access order, so upper-left of
	// screen is (0,0) when writing to display.
	const uint8_t cmd_36[] = {
		(1 << 7) | // MY=1
		(1 << 6) | // MX=1
		(0 << 5) | // MV=0
		(1 << 4) | // ML=1: reverse vertical refresh to simplify scrolling logic
		(1 << 3)   // BGR=1: For Kingtech LCD, BGR filter.
	};
	portapack_lcd_data_write_command_and_data(0x36, cmd_36, ARRAY_SIZEOF(cmd_36));

	// COLMOD: Pixel Format Set
	// DPI=101 (16 bits/pixel), DBI=101 (16 bits/pixel)
	const uint8_t cmd_3a[] = {0x55};
	portapack_lcd_data_write_command_and_data(0x3A, cmd_3a, ARRAY_SIZEOF(cmd_3a));

	//portapack_lcd_data_write_command_and_data(0xF6, { 0x01, 0x30 });
	// WEMODE=1 (reset column and page number on overflow)
	// MDT[1:0]
	// EPF[1:0]=00 (use channel MSB for LSB)
	// RIM=0 (If COLMOD[6:4]=101 (65k color), 16-bit RGB interface (1 transfer/pixel))
	// RM=0 (system interface/VSYNC interface)
	// DM[1:0]=00 (internal clock operation)
	// ENDIAN=0 (doesn't matter with 16-bit interface)
	const uint8_t cmd_f6[] = {0x01, 0x30, 0x00};
	portapack_lcd_data_write_command_and_data(0xF6, cmd_f6, ARRAY_SIZEOF(cmd_f6));

	// 3Gamma Function Disable
	const uint8_t cmd_f2[] = {0x00};
	portapack_lcd_data_write_command_and_data(0xF2, cmd_f2, ARRAY_SIZEOF(cmd_f2));

	// Gamma curve selected
	const uint8_t cmd_26[] = {0x01};
	portapack_lcd_data_write_command_and_data(0x26, cmd_26, ARRAY_SIZEOF(cmd_26));

	// Set Gamma
	const uint8_t cmd_e0[] = {
		0x0F,
		0x1D,
		0x19,
		0x0E,
		0x10,
		0x07,
		0x4C,
		0x63,
		0x3F,
		0x03,
		0x0D,
		0x00,
		0x26,
		0x24,
		0x04};
	portapack_lcd_data_write_command_and_data(0xE0, cmd_e0, ARRAY_SIZEOF(cmd_e0));

	// Set Gamma
	const uint8_t cmd_e1[] = {
		0x00,
		0x1C,
		0x1F,
		0x02,
		0x0F,
		0x03,
		0x35,
		0x25,
		0x47,
		0x04,
		0x0C,
		0x0B,
		0x29,
		0x2F,
		0x05};
	portapack_lcd_data_write_command_and_data(0xE1, cmd_e1, ARRAY_SIZEOF(cmd_e1));

	portapack_lcd_wake();

	// Turn on Tearing Effect Line (TE) output signal.
	const uint8_t cmd_35[] = {0b00000000};
	portapack_lcd_data_write_command_and_data(0x35, cmd_35, ARRAY_SIZEOF(cmd_35));
}

void portapack_backlight(const bool on)
{
	portapack_if.io_reg = (portapack_if.io_reg & 0x7f) | (on ? (1 << 7) : 0);
	portapack_io_write(1, portapack_if.io_reg);
}

void portapack_reference_oscillator(const bool on)
{
	const uint8_t mask = 1 << 6;
	portapack_if.io_reg = (portapack_if.io_reg & ~mask) | (on ? mask : 0);
	portapack_io_write(1, portapack_if.io_reg);
}

void portapack_fill_rectangle(const ui_rect_t rect, const ui_color_t color)
{
	portapack_lcd_start_ram_write(rect);
	portapack_lcd_write_pixels_color(color, rect.size.width * rect.size.height);
}

void portapack_clear_display(const ui_color_t color)
{
	const ui_rect_t rect_screen = {{0, 0}, {240, 320}};
	portapack_fill_rectangle(rect_screen, color);
}

void portapack_draw_bitmap(
	const ui_point_t point,
	const ui_bitmap_t bitmap,
	const ui_color_t foreground,
	const ui_color_t background)
{
	const ui_rect_t rect = {.point = point, .size = bitmap.size};

	portapack_lcd_start_ram_write(rect);

	const size_t count = bitmap.size.width * bitmap.size.height;
	for (size_t i = 0; i < count; i++) {
		const uint8_t pixel = bitmap.data[i >> 3] & (1U << (i & 0x7));
		portapack_lcd_write_pixel(pixel ? foreground : background);
	}
}

ui_bitmap_t portapack_font_glyph(const ui_font_t* const font, const char c)
{
	if (c >= font->c_start) {
		const uint_fast8_t index = c - font->c_start;
		if (index < font->c_count) {
			const ui_bitmap_t bitmap = {
				.size = font->glyph_size,
				.data = &font->data[index * font->data_stride]};
			return bitmap;
		}
	}

	const ui_bitmap_t bitmap = {
		.size = font->glyph_size,
		.data = font->data,
	};
	return bitmap;
}

static bool jtag_pp_tck(const bool tms_value)
{
	gpio_write(jtag_cpld.gpio->gpio_pp_tms, tms_value);

	// 8 ns TMS/TDI to TCK setup
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");

	gpio_set(jtag_cpld.gpio->gpio_tck);

	// 15 ns TCK to TMS/TDI hold time
	// 20 ns TCK high time
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");

	gpio_clear(jtag_cpld.gpio->gpio_tck);

	// 20 ns TCK low time
	// 25 ns TCK falling edge to TDO valid
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");

	return gpio_read(jtag_cpld.gpio->gpio_pp_tdo);
}

static uint32_t jtag_pp_shift(const uint32_t tms_bits, const size_t count)
{
	uint32_t result = 0;
	size_t bit_in_index = count - 1;
	size_t bit_out_index = 0;
	while (bit_out_index < count) {
		const uint32_t tdo = jtag_pp_tck((tms_bits >> bit_in_index) & 1) & 1;
		result |= (tdo << bit_out_index);

		bit_in_index--;
		bit_out_index++;
	}

	return result;
}

static uint32_t jtag_pp_idcode(void)
{
	cpld_jtag_take(&jtag_cpld);

	/* TODO: Check if PortaPack TMS is floating or driven by an external device. */
	gpio_output(jtag_cpld.gpio->gpio_pp_tms);

	/* Test-Logic/Reset -> Run-Test/Idle -> Select-DR/Scan -> Capture-DR */
	jtag_pp_shift(0b11111010, 8);

	/* Shift-DR */
	const uint32_t idcode = jtag_pp_shift(0, 32);

	/* Exit1-DR -> Update-DR -> Run-Test/Idle -> ... -> Test-Logic/Reset */
	jtag_pp_shift(0b11011111, 8);

	cpld_jtag_release(&jtag_cpld);

	return idcode;
}

static bool portapack_detect(void)
{
	const uint32_t idcode = jtag_pp_idcode();

	/* 0x020A50DD => Altera 5M40ZE64C5N
	   0x00025610 => AGM Microelectronics AG256SL100 */
	return idcode == 0x020A50DD || idcode == 0x00025610;
}

static const portapack_t portapack_instance = {};

static const portapack_t* portapack_pointer = NULL;

const portapack_t* portapack(void)
{
	return portapack_pointer;
}

void portapack_init(void)
{
	if (portapack_detect()) {
		portapack_if_init();
		portapack_lcd_reset();
		portapack_lcd_init();
		portapack_pointer = &portapack_instance;
	} else {
		portapack_pointer = NULL;
	}
}

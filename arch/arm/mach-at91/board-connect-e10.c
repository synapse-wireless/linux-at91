/*
 *  Copyright (C) 2005 SAN People
 *  Copyright (C) 2010 calao-systems
 *  Copyright (C) 2015 Synapse Wireless, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/clk.h>

#include <mach/hardware.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/irq.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/at91sam9_smc.h>
#include <mach/system_rev.h>

#include "at91_aic.h"
#include "board.h"
#include "sam9_smc.h"
#include "generic.h"


static void __init ek_init_early(void)
{
	/* Initialize processor: 12.000 MHz crystal */
	at91_initialize(12000000);
}

/*
 * USB Host port
 */
static struct at91_usbh_data __initdata ek_usbh_data = {
	.ports		        = 1,
	.vbus_pin	        = {-EINVAL, -EINVAL},
	.overcurrent_pin    = {-EINVAL, -EINVAL},
};

/*
 * USB Device port
 */
static struct at91_udc_data __initdata ek_udc_data = {
	.vbus_pin	= AT91_PIN_PC5,
	.pullup_pin	= -EINVAL,		/* pull-up driven by UDC */
};

/*
 * MACB Ethernet device
 */
static struct macb_platform_data __initdata ek_macb_data = {
	.phy_irq_pin	= AT91_PIN_PA31,
	.is_rmii	= 1,
};

/*
 * NAND flash
 */
static struct mtd_partition __initdata ek_nand_partition[] = {
	{
		.name   = "bootstrap",
		.offset = 0,
		.size   = 128 * SZ_1K,
	},
	{
		.name	= "uboot",
		.offset	= MTDPART_OFS_NXTBLK,
		.size	= 384 * SZ_1K,
	},
	{
		.name	= "environment",
		.offset	= MTDPART_OFS_NXTBLK,
		.size	= 256 * SZ_1K,
	},
	{
		.name	= "uImage",
		.offset	= MTDPART_OFS_NXTBLK,
		.size	= 3328 * SZ_1K,
	},
	{
		.name	= "rescue",
		.offset	= MTDPART_OFS_NXTBLK,
		.size	= 9216 * SZ_1K,
	},
	{
		.name	= "rootfs",
		.offset	= MTDPART_OFS_NXTBLK,
		.size	= 211 * SZ_1M,
	},
	{
		.name	= "recovery",
		.offset	= MTDPART_OFS_NXTBLK,
		.size	= MTDPART_SIZ_FULL,
	},
};

/* det_pin is not connected */
static struct atmel_nand_data __initdata ek_nand_data = {
	.ale		= 21,
	.cle		= 22,
	.rdy_pin	= AT91_PIN_PC13,
	.enable_pin	= AT91_PIN_PC14,
	.det_pin	= -EINVAL,
	.ecc_mode	= NAND_ECC_SOFT,
	.on_flash_bbt	= 1,
	.parts		= ek_nand_partition,
	.num_parts	= ARRAY_SIZE(ek_nand_partition),
};

static struct sam9_smc_config __initdata ek_nand_smc_config = {
	.ncs_read_setup		= 0,
	.nrd_setup		= 2,
	.ncs_write_setup	= 0,
	.nwe_setup		= 2,

	.ncs_read_pulse		= 4,
	.nrd_pulse		= 4,
	.ncs_write_pulse	= 4,
	.nwe_pulse		= 4,

	.read_cycle		= 7,
	.write_cycle		= 7,

	.mode			= AT91_SMC_READMODE | AT91_SMC_WRITEMODE |
					  AT91_SMC_EXNWMODE_DISABLE,
	.tdf_cycles		= 3,
};

static void __init ek_add_device_nand(void)
{
	ek_nand_smc_config.mode |= AT91_SMC_DBW_8;

	/* configure chip-select 3 (NAND) */
	sam9_smc_configure(0, 3, &ek_nand_smc_config);

	at91_add_device_nand(&ek_nand_data);
}

/*
 * LEDs
 */
static struct gpio_led ek_leds[] = {
	{	/* only LED B is controlled by Linux */
		.name			= "ledB:red",
		.gpio			= AT91_PIN_PB13,
		.default_trigger	= "none",
	},
	{
		.name			= "ledB:green",
		.gpio			= AT91_PIN_PB12,
		.default_trigger	= "none",
	}
};

/*
 * GPIO Buttons
 */
#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
static struct gpio_keys_button ek_buttons[] = {
	{
		.gpio		= AT91_PIN_PB10,
		.code		= BTN_3,
		.desc		= "mode",
		.active_low	= 1,
		.wakeup		= 1,
	},
};

static struct gpio_keys_platform_data ek_button_data = {
	.buttons	= ek_buttons,
	.nbuttons	= ARRAY_SIZE(ek_buttons),
};

static struct platform_device ek_button_device = {
	.name		= "gpio-keys",
	.id			= -1,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &ek_button_data,
	}
};

static void __init ek_add_device_buttons(void)
{
	at91_set_gpio_input(AT91_PIN_PB10, 1);	/* btn3 */
	at91_set_deglitch(AT91_PIN_PB10, 1);

	platform_device_register(&ek_button_device);
}
#else
static void __init ek_add_device_buttons(void) {}
#endif


static struct i2c_board_info __initdata ek_i2c_devices[] = {
	{
		I2C_BOARD_INFO("m41t81", 0x68),
	},
};


static void __init ek_board_init(void)
{
	/* set serial console to ttyS0 (ie, DBGU) (Rx, Tx)*/
	at91_register_uart(0, 0, 0);

	/* USART0 on ttyS1. (Rx, Tx, CTS, RTS) */
	at91_register_uart(AT91SAM9260_ID_US0, 1,
			ATMEL_UART_CTS | ATMEL_UART_RTS);

	/* USART1 on ttyS2. (Rx, Tx, CTS, RTS) */
	at91_register_uart(AT91SAM9260_ID_US1, 2,
			ATMEL_UART_CTS | ATMEL_UART_RTS);

	/* Setup UARTs */
	at91_add_device_serial();

	/* USB Host */
	at91_add_device_usbh(&ek_usbh_data);
	/* USB Device */
	at91_add_device_udc(&ek_udc_data);
	/* NAND */
	ek_add_device_nand();
	/* Ethernet */
	at91_add_device_eth(&ek_macb_data);
	/* I2C */
	at91_add_device_i2c(ek_i2c_devices, ARRAY_SIZE(ek_i2c_devices));
	/* LEDs */
	at91_gpio_leds(ek_leds, ARRAY_SIZE(ek_leds));
	/* Push Buttons */
	ek_add_device_buttons();
	/* SNAP Module Reset Trigger */
	gpio_request_one(AT91_PIN_PB11,
			GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED,
			"snap_reset");
}

MACHINE_START(CONNECT_E10, "connect-e10")
	/* Maintainer: Doug Goldstein <cardoe@cardoe.com> */
	.init_time      = at91sam926x_pit_init,
	.map_io		= at91_map_io,
	.handle_irq     = at91_aic_handle_irq,
	.init_early     = ek_init_early,
	.init_irq	= at91_init_irq_default,
	.init_machine	= ek_board_init,
MACHINE_END

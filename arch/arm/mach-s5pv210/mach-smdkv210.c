/* linux/arch/arm/mach-s5pv210/mach-smdkv210.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/device.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/pwm_backlight.h>
#include <linux/mmc/host.h>
#include <linux/spi/spi.h>

#include <asm/hardware/vic.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

//#include <video/platform_lcd.h>

#include <net/ax88796.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/irqs.h>
#include <mach/gpio.h>
#include <mach/ohci.h>


#include <plat/regs-serial.h>
#include <plat/regs-srom.h>
#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/adc.h>
#include <plat/ts.h>
#include <plat/ata.h>
#include <plat/iic.h>
#include <plat/keypad.h>
#include <plat/pm.h>
#include <plat/fb.h>
#include <plat/s5p-time.h>
#include <plat/backlight.h>
#include <plat/regs-fb-v4.h>
#include <plat/mfc.h>
#include <plat/ehci.h>
#include <plat/usb-phy.h>
#include <plat/s3c64xx-spi.h>

#include <plat/sdhci.h>
#include "common.h"

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDKV210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDKV210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDKV210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg smdkv210_uartcfgs[] __initdata = {
	[0] = {
		.hwport	= 0,
		.flags	= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon	= SMDKV210_ULCON_DEFAULT,
		.ufcon	= SMDKV210_UFCON_DEFAULT,
	},
	[1] = {
		.hwport	= 1,
		.flags	= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon	= SMDKV210_ULCON_DEFAULT,
		.ufcon	= SMDKV210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport	= 2,
		.flags	= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon	= SMDKV210_ULCON_DEFAULT,
		.ufcon	= SMDKV210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport	= 3,
		.flags	= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon	= SMDKV210_ULCON_DEFAULT,
		.ufcon	= SMDKV210_UFCON_DEFAULT,
	},
};


#if 0
static struct s3c_ide_platdata smdkv210_ide_pdata __initdata = {
	.setup_gpio	= s5pv210_ide_setup_gpio,
};
static uint32_t smdkv210_keymap[] __initdata = {
	/* KEY(row, col, keycode) */
	KEY(0, 3, KEY_1), KEY(0, 4, KEY_2), KEY(0, 5, KEY_3),
	KEY(0, 6, KEY_4), KEY(0, 7, KEY_5),
	KEY(1, 3, KEY_A), KEY(1, 4, KEY_B), KEY(1, 5, KEY_C),
	KEY(1, 6, KEY_D), KEY(1, 7, KEY_E)
};

static struct matrix_keymap_data smdkv210_keymap_data __initdata = {
	.keymap		= smdkv210_keymap,
	.keymap_size	= ARRAY_SIZE(smdkv210_keymap),
};

static struct samsung_keypad_platdata smdkv210_keypad_data __initdata = {
	.keymap_data	= &smdkv210_keymap_data,
	.rows		= 8,
	.cols		= 8,
};
static void smdkv210_lte480wv_set_power(struct plat_lcd_data *pd,
					unsigned int power)
{
	if (power) {
#if !defined(CONFIG_BACKLIGHT_PWM)
		gpio_request_one(S5PV210_GPD0(3), GPIOF_OUT_INIT_HIGH, "GPD0");
		gpio_free(S5PV210_GPD0(3));
#endif

		/* fire nRESET on power up */
		gpio_request_one(S5PV210_GPH0(6), GPIOF_OUT_INIT_HIGH, "GPH0");

		gpio_set_value(S5PV210_GPH0(6), 0);
		mdelay(10);

		gpio_set_value(S5PV210_GPH0(6), 1);
		mdelay(10);

		gpio_free(S5PV210_GPH0(6));
	} else {
#if !defined(CONFIG_BACKLIGHT_PWM)
		gpio_request_one(S5PV210_GPD0(3), GPIOF_OUT_INIT_LOW, "GPD0");
		gpio_free(S5PV210_GPD0(3));
#endif
	}
}

static struct plat_lcd_data smdkv210_lcd_lte480wv_data = {
	.set_power	= smdkv210_lte480wv_set_power,
};

#endif

#if 0
static struct platform_device smdkv210_lcd_lte480wv = {
	.name			= "platform-lcd",
	.dev.parent		= &s3c_device_fb.dev,
	//.dev.platform_data	= &smdkv210_lcd_lte480wv_data,
};

static struct s3c_fb_pd_win smdkv210_fb_win0 = {
	.win_mode = {
		.left_margin	= 13,
		.right_margin	= 8,
		.upper_margin	= 7,
		.lower_margin	= 5,
		.hsync_len	= 3,
		.vsync_len	= 1,
		.xres		= 800,
		.yres		= 480,
	},
	.max_bpp	= 32,
	.default_bpp	= 24,
};

static struct s3c_fb_platdata smdkv210_lcd0_pdata __initdata = {
	.win[0]	= &smdkv210_fb_win0,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC,
	.setup_gpio	= s5pv210_fb_gpio_setup_24bpp,
};
#endif

// ax88796b driver resource
#define BASE_ADDR_AX88796B			(S5PV210_PA_CS1 + 0x000)
#define IRQ_AX88796B					IRQ_EINT(10)
#define GPIO_FOR_IRQ_AX88796B		(S5PV210_GPH1(2))			// EINT10
static struct ax_plat_data ax88796_platdata = {
	.flags		= AXFLG_MAC_FROMDEV,
	.wordlength	= 2,
	.dcr_val	= 0x48,
	.rcr_val	= 0x40,
};
static struct resource ax88796_resources[] = {
	[0] = {
		.start  = BASE_ADDR_AX88796B,
		.end    = BASE_ADDR_AX88796B  + 0xff, 
		.flags  = IORESOURCE_MEM,
	},  
	[1] = {
		.start = IRQ_AX88796B,
		.end   = IRQ_AX88796B,
	  	.flags = IORESOURCE_IRQ,
	},
};

struct platform_device device_ax88796b = {
	.name          = "ax88796b",
	.id    			= -1,
	.num_resources = ARRAY_SIZE(ax88796_resources),
	.resource      = ax88796_resources,
	.dev	= {
		.platform_data = &ax88796_platdata
	}
};

static void ax88796_hw_init(void)
{
	u32  gpio, irq;
	
	gpio = GPIO_FOR_IRQ_AX88796B;
	irq  = IRQ_AX88796B;
	s3c_gpio_cfgpin	( gpio, S3C_GPIO_SFN(0xF)		);
	s3c_gpio_setpull	( gpio, S3C_GPIO_PULL_UP		);
	irq_set_irq_type	( irq , IRQF_TRIGGER_FALLING	);
}

// nand flash driver resource
static struct resource s3c_nand_resource[] = {
    [0] = {
        .start = S5P_PA_NAND,
        .end   = S5P_PA_NAND + S5P_SZ_NAND - 1,
        .flags = IORESOURCE_MEM,
    }
};
struct platform_device s3c_device_nand = {
    .name       		= "s3c-nand",
    .id    		 		= -1,
    .num_resources  	= ARRAY_SIZE(s3c_nand_resource),
    .resource   		= s3c_nand_resource,
};



// i2c driver resource
static struct i2c_board_info i2c_devs0[] __initdata = {
	{
		I2C_BOARD_INFO("ds1307", 0x68),
	},
};


// usb driver
static struct s5pv210_ohci_platdata smdkv210_ohci_pdata;
void __init smdkv210_ohci_init(void)
{
	struct s5pv210_ohci_platdata *pdata = &smdkv210_ohci_pdata;

	s5pv210_ohci_set_platdata(pdata);
}
#if 0
static struct s5p_ehci_platdata smdkv210_ehci_pdata;
void __init smdkv210_ehci_init(void)
{
	struct s5p_ehci_platdata *pdata = &smdkv210_ehci_pdata;

	s5p_ehci_set_platdata(pdata);
}
#endif


// hsmmc0 platdata
static struct s3c_sdhci_platdata smdkv210_hsmmc0_data __initdata = {
	.max_width		= 4,
	.host_caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED,
	.cd_type			= S3C_SDHCI_CD_NONE,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
};



#define SPI_MAX_HZ		12 * 1000 * 1000
static struct s3c64xx_spi_csinfo smdk_spi_csi[] = {
	[0] = {
		.line = S5PV210_GPB(1),
		.set_level = gpio_set_value,
		.fb_delay = 0x0,
	},
	[1] = {
		.line = S5PV210_GPB(5),
		.set_level = gpio_set_value,
		.fb_delay = 0x0,
	},
};
static struct spi_board_info s3c_spi_devs[] __initdata = {
	{
		.modalias	 	= "spidev",
		.mode		 		= SPI_MODE_0,		
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 0,
		.chip_select	= 0,
		.controller_data = &smdk_spi_csi[0],
	},
	{
		.modalias	 	= "spidev",
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 0,
		.chip_select	= 1,
		.controller_data = &smdk_spi_csi[0],
	},
	{
		.modalias	 	= "spidev",
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 0,
		.chip_select	= 2,
		.controller_data = &smdk_spi_csi[0],
	},
	{
		.modalias	 	= "spidev", 		
		.mode		 		= SPI_MODE_0,
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 0,
		.chip_select	= 3,
		.controller_data = &smdk_spi_csi[0],
	},
	{
		.modalias	 	= "spidev", 	
		.mode		 		= SPI_MODE_0,
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 0,
		.chip_select	= 4,
		.controller_data = &smdk_spi_csi[0],
	},
	{
		.modalias	 	= "spidev", 	
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 0,
		.chip_select	= 5,
		.controller_data = &smdk_spi_csi[0],
	},
	{
		.modalias	 	= "spidev", 
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 0,
		.chip_select	= 6,
		.controller_data = &smdk_spi_csi[0],
	},
	{
		.modalias	 	= "spidev", 	
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 0,
		.chip_select	= 7,
		.controller_data = &smdk_spi_csi[0],
	},
	{
		.modalias	 	= "spidev",
		.mode		 		= SPI_MODE_0,		
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 0,
		.controller_data = &smdk_spi_csi[1],
	},
	{
		.modalias	 	= "spidev",
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 1,
		.controller_data = &smdk_spi_csi[1],
	},
	{
		.modalias	 	= "spidev",
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 2,
		.controller_data = &smdk_spi_csi[1],
	},
	{
		.modalias	 	= "spidev", 		
		.mode		 		= SPI_MODE_0,
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 3,
		.controller_data = &smdk_spi_csi[1],
	},
	{
		.modalias	 	= "spidev", 	
		.mode		 		= SPI_MODE_0,
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 4,
		.controller_data = &smdk_spi_csi[1],
	},
	{
		.modalias	 	= "spidev", 	
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 5,
		.controller_data = &smdk_spi_csi[1],
	},
	{
		.modalias	 	= "spidev", 
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 6,
		.controller_data = &smdk_spi_csi[1],
	},
	{
		.modalias	 	= "spidev", 	
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 7,
		.controller_data = &smdk_spi_csi[1],
	},
	{
		.modalias	 	= "spidev", 	
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 8,
		.controller_data = &smdk_spi_csi[1],
	},
	{
		.modalias	 	= "vs1003_sci", 	
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 9,
		.controller_data = &smdk_spi_csi[1],
	},
	{
		.modalias	 	= "vs1003_sdi", 	
		.mode		 		= SPI_MODE_0,	
		.max_speed_hz  = SPI_MAX_HZ,
		.bus_num	 		= 1,
		.chip_select	= 10,
		.controller_data = &smdk_spi_csi[1],
	},
};

static struct platform_device *smdkv210_devices[] __initdata = {
	&s3c_device_adc,
	//	&s3c_device_cfcon,
	// &s3c_device_fb,
	&s3c_device_hsmmc0,
	//	&s3c_device_hsmmc1,
	//	&s3c_device_hsmmc2,
	//	&s3c_device_hsmmc3,
	//&s3c_device_i2c0,
	//	&s3c_device_i2c1,
	//	&s3c_device_i2c2,
	//	&s3c_device_rtc,
	//	&s3c_device_ts,
	//	&s3c_device_wdt,
	//	&s5p_device_fimc0,
	//	&s5p_device_fimc1,
	//	&s5p_device_fimc2,
	//	&s5p_device_fimc_md,
	//	&s5p_device_jpeg,
	//	&s5p_device_mfc,
	//	&s5p_device_mfc_l,
	//	&s5p_device_mfc_r,
	//	&s5pv210_device_ac97,
	//	&s5pv210_device_iis0,
	//	&s5pv210_device_spdif,
	//	&samsung_asoc_dma,
	//	&samsung_asoc_idma,
	//	&samsung_device_keypad,
	//&smdkv210_lcd_lte480wv,
	&s3c_device_nand,
	&device_ax88796b,

	&s5pv210_device_ohci,
	//&s5p_device_ehci,
	&s3c64xx_device_spi0,
	&s3c64xx_device_spi1,
};

#if 0
static struct i2c_board_info smdkv210_i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c08", 0x50), },     /* Samsung S524AD0XD1 */
	{ I2C_BOARD_INFO("wm8580", 0x1b), },
};

static struct i2c_board_info smdkv210_i2c_devs1[] __initdata = {
	/* To Be Updated */
};

static struct i2c_board_info smdkv210_i2c_devs2[] __initdata = {
	/* To Be Updated */
};

/* LCD Backlight data */
static struct samsung_bl_gpio_info smdkv210_bl_gpio_info = {
	.no = S5PV210_GPD0(3),
	.func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smdkv210_bl_data = {
	.pwm_id = 3,
	.pwm_period_ns = 1000,
};

#endif

static struct platform_device *s5pv210_i2c_devices[] __initdata = {
	&s3c_device_i2c0,
};

static void __init smdkv210_map_io(void)
{
	s5pv210_init_io(NULL, 0);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdkv210_uartcfgs, ARRAY_SIZE(smdkv210_uartcfgs));
	s5p_set_timer_source(S5P_PWM2, S5P_PWM4);
}

static void __init smdkv210_reserve(void)
{
	s5p_mfc_reserve_mem(0x43000000, 8 << 20, 0x51000000, 8 << 20);
}

static void __init smdkv210_machine_init(void)
{
	s3c_pm_init();
	ax88796_hw_init();

	//samsung_keypad_set_platdata(&smdkv210_keypad_data);
	//s3c24xx_ts_set_platdata(NULL);

	platform_add_devices(s5pv210_i2c_devices, ARRAY_SIZE(s5pv210_i2c_devices) );
	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	//s3c_ide_set_platdata(&smdkv210_ide_pdata);
	//s3c_fb_set_platdata(&smdkv210_lcd0_pdata);
	//s5p_fimd0_set_platdata(&nuri_fb_pdata);

	smdkv210_ohci_init();
	//smdkv210_ehci_init();

	//s5pv210_default_sdhci0();
	s3c_sdhci0_set_platdata(&smdkv210_hsmmc0_data);

	//samsung_bl_set(&smdkv210_bl_gpio_info, &smdkv210_bl_data);


	s3c64xx_spi0_set_platdata(&s3c64xx_spi0_pdata, 0, 8);
	s3c64xx_spi1_set_platdata(&s3c64xx_spi1_pdata, 0, 11);
	spi_register_board_info(s3c_spi_devs, ARRAY_SIZE(s3c_spi_devs));

	platform_add_devices(smdkv210_devices, ARRAY_SIZE(smdkv210_devices));
}

MACHINE_START(SMDKV210, "SMDKV210")
/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.atag_offset	= 0x100,
	.init_irq		= s5pv210_init_irq,
	.handle_irq		= vic_handle_irq,
	.map_io			= smdkv210_map_io,
	.init_machine	= smdkv210_machine_init,
	.timer			= &s5p_timer,
	.restart			= s5pv210_restart,
	.reserve			= &smdkv210_reserve,
MACHINE_END

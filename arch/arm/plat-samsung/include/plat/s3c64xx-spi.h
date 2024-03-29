/* linux/arch/arm/plat-samsung/include/plat/s3c64xx-spi.h
 *
 * Copyright (C) 2009 Samsung Electronics Ltd.
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S3C64XX_PLAT_SPI_H
#define __S3C64XX_PLAT_SPI_H

struct platform_device;

/**
 * struct s3c64xx_spi_csinfo - ChipSelect description
 * @fb_delay: Slave specific feedback delay.
 *            Refer to FB_CLK_SEL register definition in SPI chapter.
 * @line: Custom 'identity' of the CS line.
 * @set_level: CS line control.
 *
 * This is per SPI-Slave Chipselect information.
 * Allocate and initialize one in machine init code and make the
 * spi_board_info.controller_data point to it.
 */
struct s3c64xx_spi_csinfo {
	u8 fb_delay;
	unsigned line;
	void (*set_level)(unsigned line_id, int lvl);
};

/**
 * struct s3c64xx_spi_info - SPI Controller defining structure
 * @src_clk_nr: Clock source index for the CLK_CFG[SPI_CLKSEL] field.
 * @clk_from_cmu: If the SPI clock/prescalar control block is present
 *     by the platform's clock-management-unit and not in SPI controller.
 * @num_cs: Number of CS this controller emulates.
 * @cfg_gpio: Configure pins for this SPI controller.
 * @fifo_lvl_mask: All tx fifo_lvl fields start at offset-6
 * @rx_lvl_offset: Depends on tx fifo_lvl field and bus number
 * @high_speed: If the controller supports HIGH_SPEED_EN bit
 * @tx_st_done: Depends on tx fifo_lvl field
 */
struct s3c64xx_spi_info {
	int src_clk_nr;
	bool clk_from_cmu;

	int num_cs;

	int (*cfg_gpio)(struct platform_device *pdev);
	void (*select_dev)(int num);
	/* Following two fields are for future compatibility */
	int fifo_lvl_mask;
	int rx_lvl_offset;
	int high_speed;
	int tx_st_done;
};

/**
 * s3c64xx_spi_set_platdata - SPI Controller configure callback by the board
 *				initialization code.
 * @pd: SPI platform data to set.
 * @src_clk_nr: Clock the SPI controller is to use to generate SPI clocks.
 * @num_cs: Number of elements in the 'cs' array.
 *
 * Call this from machine init code for each SPI Controller that
 * has some chips attached to it.
 */
extern void s3c64xx_spi0_set_platdata(struct s3c64xx_spi_info *pd,
				      int src_clk_nr, int num_cs);
extern void s3c64xx_spi1_set_platdata(struct s3c64xx_spi_info *pd,
				      int src_clk_nr, int num_cs);
extern void s3c64xx_spi2_set_platdata(struct s3c64xx_spi_info *pd,
				      int src_clk_nr, int num_cs);

/* defined by architecture to configure gpio */
extern int s3c64xx_spi0_cfg_gpio(struct platform_device *dev);
extern int s3c64xx_spi1_cfg_gpio(struct platform_device *dev);
extern int s3c64xx_spi2_cfg_gpio(struct platform_device *dev);

extern struct s3c64xx_spi_info s3c64xx_spi0_pdata;
extern struct s3c64xx_spi_info s3c64xx_spi1_pdata;
extern struct s3c64xx_spi_info s3c64xx_spi2_pdata;
#endif /* __S3C64XX_PLAT_SPI_H */

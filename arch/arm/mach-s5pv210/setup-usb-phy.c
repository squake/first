/*
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundationr
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <mach/map.h>
#include <mach/regs-sys.h>
#include <mach/regs-clock.h>
#include <plat/cpu.h>
#include <plat/regs-usb-hsotg-phy.h>
#include <plat/usb-phy.h>

static int s5pv210_usb_otgphy_init(struct platform_device *pdev)
{
	struct clk *xusbxti;
	u32 phyclk;
	
	writel(readl(S5PV210_USB_PHY_CON) | S5PV210_USB_PHY0_EN,
			S5PV210_USB_PHY_CON);

	/* set clock frequency for PLL */
	phyclk = readl(S3C_PHYCLK) & ~S3C_PHYCLK_CLKSEL_MASK;

	xusbxti = clk_get(&pdev->dev, "xusbxti");
	if (xusbxti && !IS_ERR(xusbxti)) {
		switch (clk_get_rate(xusbxti)) {
		case 12 * MHZ:
			phyclk |= S3C_PHYCLK_CLKSEL_12M;
			break;
		case 24 * MHZ:
			phyclk |= S3C_PHYCLK_CLKSEL_24M;
			break;
		default:
		case 48 * MHZ:
			/* default reference clock */
			break;
		}
		clk_put(xusbxti);
	}

	/* TODO: select external clock/oscillator */
	writel(phyclk | S3C_PHYCLK_CLK_FORCE, S3C_PHYCLK);

	/* set to normal OTG PHY */
	writel((readl(S3C_PHYPWR) & ~S3C_PHYPWR_NORMAL_MASK), S3C_PHYPWR);
	mdelay(1);

	/* reset OTG PHY and Link */
	writel(S3C_RSTCON_PHY | S3C_RSTCON_HCLK | S3C_RSTCON_PHYCLK,
			S3C_RSTCON);
	udelay(20);	/* at-least 10uS */
	writel(0, S3C_RSTCON);

	return 0;
}

static int s5pv210_usb_otgphy_exit(struct platform_device *pdev)
{
	writel((readl(S3C_PHYPWR) | S3C_PHYPWR_ANALOG_POWERDOWN |
				S3C_PHYPWR_OTG_DISABLE), S3C_PHYPWR);

	writel(readl(S5PV210_USB_PHY_CON) & ~S5PV210_USB_PHY0_EN,
			S5PV210_USB_PHY_CON);

	return 0;
}

static int s5pv210_usb_host_phy_init(void)
{
	struct clk *otg_clk;

	otg_clk = clk_get(NULL, "otg");
	clk_enable(otg_clk);

	if (readl(S5P_USB_PHY_CONTROL) & (0x1<<1)) return -1;
	writel(readl(S5P_USB_PHY_CONTROL) |(0x1<<1), S5P_USB_PHY_CONTROL);
	writel((readl(S3C_PHYPWR) &~(0x1<<7)&~(0x1<<6))|(0x1<<8)|(0x1<<5), S3C_PHYPWR);
	writel((readl(S3C_PHYCLK) &~(0x1<<7))|(0x3<<0), S3C_PHYCLK);
	writel((readl(S3C_RSTCON)) |(0x1<<4)|(0x1<<3), S3C_RSTCON);
	writel(readl(S3C_RSTCON) &~(0x1<<4)&~(0x1<<3), S3C_RSTCON);

	return 0;
}

static int s5pv210_usb_host_phy_off(void)
{
	writel(readl(S3C_PHYPWR) |(0x1<<7)|(0x1<<6), S3C_PHYPWR);
	writel(readl(S5P_USB_PHY_CONTROL) &~(1<<1), S5P_USB_PHY_CONTROL);

	return 0;
}

int s5p_usb_phy_init(struct platform_device *pdev, int type)
{

	if (type == S5P_USB_PHY_DEVICE)
		return s5pv210_usb_otgphy_init(pdev);
	else if (type == S5P_USB_PHY_HOST)
		return s5pv210_usb_host_phy_init();

	return -EINVAL;
}

int s5p_usb_phy_exit(struct platform_device *pdev, int type)
{
	if (type == S5P_USB_PHY_DEVICE)
		return s5pv210_usb_otgphy_exit(pdev);
	else if (type == S5P_USB_PHY_HOST)
		return s5pv210_usb_host_phy_off();

	return -EINVAL;
}

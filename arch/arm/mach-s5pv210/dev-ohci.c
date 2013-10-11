/* linux/arch/arm/mach-exynos/dev-ohci.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS - OHCI support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/dma-mapping.h>
#include <linux/platform_device.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <mach/ohci.h>

#include <plat/devs.h>
#include <plat/usb-phy.h>

static struct resource s5pv210_ohci_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_OHCI, SZ_1M),
	[1] = DEFINE_RES_IRQ(IRQ_UHOST),
};

static u64 s5pv210_ohci_dma_mask = DMA_BIT_MASK(32);

struct platform_device s5pv210_device_ohci = {
	.name				= "s5pv210-ohci",
	.id				= -1,
	.num_resources	= ARRAY_SIZE(s5pv210_ohci_resource),
	.resource		= s5pv210_ohci_resource,
	.dev				= {
		.dma_mask				= &s5pv210_ohci_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

void __init s5pv210_ohci_set_platdata(struct s5pv210_ohci_platdata *pd)
{
	struct s5pv210_ohci_platdata *npd;

	npd = s3c_set_platdata(pd, sizeof(struct s5pv210_ohci_platdata),
			&s5pv210_device_ohci);

	if (!npd->phy_init)
		npd->phy_init = s5p_usb_phy_init;
	if (!npd->phy_exit)
		npd->phy_exit = s5p_usb_phy_exit;
}

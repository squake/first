/* 
 * ============================================================================
 * ax88796b.c: Linux2.6.x Ethernet device driver for ASIX AX88796B chip.
 *
 * This program is free software; you can distrine_block_inputbute it and/or
 * modify it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * This program is based on
 *
 *    ne.c:    A general non-shared-memory NS8390 ethernet driver for linux
 *             Written 1992-94 by Donald Becker.
 *
 *    8390. c: A general NS8390 ethernet driver core for linux.
 *             Written 1992-94 by Donald Becker.
 *
 * ============================================================================
 */
#include "ax88796b.h"

/* NAMING CONSTANT DECLARATIONS */
#define DRV_NAME	"AX88796B"
#define ADP_NAME	"ASIX AX88796B Ethernet Adapter"
#define DRV_VERSION	"2.2.0"
#define PFX		DRV_NAME ": "

#define	PRINTK(flag, args...) if (flag & DEBUG_FLAGS) printk(args)

//#define CONFIG_AX88796B_USE_MEMCPY		1
#define CONFIG_AX88796B_USE_MEMCPY		0			// falinux modify
#define CONFIG_AX88796B_8BIT_WIDE		0
#define CONFIG_AX88796B_EEPROM_READ_WRITE	0


// [FALINUX] 부트로더에서 넘겨 받는 MAC 주소 설정
unsigned char ezboot_mac[6];
static int __init ezboot_mac_setup(char *str)
{
	unsigned char   idx;
	char    str_mac[32], str_byte[3];
	char    *ptr;

	strcpy( str_mac, str );

	ptr = str_mac;
	for (idx=0; idx<6; idx++)
	{
		str_byte[0] =  *ptr++;
		str_byte[1] =  *ptr++;
		str_byte[2] =  0;
		ptr ++;

		ezboot_mac[idx] = simple_strtol( str_byte, NULL, 16 );
	}

	return 1;
}
__setup("mac=",  ezboot_mac_setup);

/* LOCAL VARIABLES DECLARATIONS */
static char version[] =
//KERN_INFO ADP_NAME ":v" DRV_VERSION " " __TIME__ " " __DATE__ "\n"
//KERN_INFO "  http://www.asix.com.tw\n";
KERN_INFO ADP_NAME ":v" DRV_VERSION "\n";

static unsigned int media = 0;
static int mem = 0;
static int irq = 0;
static int weight = 0;

module_param (mem, int, 0);
module_param (irq, int, 0);
module_param (media, int, 0);
module_param (weight, int, 0); 

MODULE_PARM_DESC (mem, "MEMORY base address(es),required");
MODULE_PARM_DESC (irq, "IRQ number(s)");
MODULE_PARM_DESC (media,
	"Media Mode(0=auto, 1=100full, 2=100half, 3=10full, 4=10half)");
MODULE_PARM_DESC (weight, "NAPI weight");

MODULE_DESCRIPTION ("ASIX AX88796B Fast Ethernet driver");
MODULE_LICENSE ("GPL");

static int ax_get_link (struct ax_device *ax_local);

#if (CONFIG_AX88796B_8BIT_WIDE == 1)
static inline u16 READ_FIFO (void *membase)
{
	return (readb (membase) | (((u16)readb (membase)) << 8));
}

static inline void WRITE_FIFO (void *membase, u16 data)
{
	writeb ((u8)data , membase);
	writeb ((u8)(data >> 8) , membase);
}
#else
static inline u16 READ_FIFO (void *membase)
{
	return readw (membase);
}

static inline void WRITE_FIFO (void *membase, u16 data)
{
	writew (data, membase);
}
#endif

static inline struct ax_device *ax_get_priv (struct net_device *ndev)
{
	return (struct ax_device *) netdev_priv (ndev);
}

/*  
 *  ======================================================================
 *   MII interface support
 *  ======================================================================
 */
#define MDIO_SHIFT_CLK		0x01
#define MDIO_DATA_WRITE0	0x00
#define MDIO_DATA_WRITE1	0x08
#define MDIO_DATA_READ		0x04
#define MDIO_MASK		0x0f
#define MDIO_ENB_IN		0x02

/*
 * ----------------------------------------------------------------------------
 * Function Name: mdio_sync
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static void mdio_sync (struct net_device *ndev)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
    int bits;
    for (bits = 0; bits < 32; bits++) {
		writeb (MDIO_DATA_WRITE1, ax_base + AX88796_MII_EEPROM);
		writeb (MDIO_DATA_WRITE1 | MDIO_SHIFT_CLK,
				ax_base + AX88796_MII_EEPROM);
    }
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: mdio_sync
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static int mdio_read (struct net_device *ndev, int phy_id, int loc)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	u_int cmd = (0xf6<<10)|(phy_id<<5)|loc;
	int i, retval = 0;

	mdio_sync (ndev);
	for (i = 13; i >= 0; i--) {
		int dat = (cmd&(1<<i)) ? MDIO_DATA_WRITE1 : MDIO_DATA_WRITE0;
		writeb (dat, ax_base + AX88796_MII_EEPROM);
		writeb (dat | MDIO_SHIFT_CLK, ax_base + AX88796_MII_EEPROM);
	}
	for (i = 19; i > 0; i--) {
		writeb (MDIO_ENB_IN, ax_base + AX88796_MII_EEPROM);
		retval = (retval << 1) | ((readb (ax_base + AX88796_MII_EEPROM)
				& MDIO_DATA_READ) != 0);
		writeb (MDIO_ENB_IN | MDIO_SHIFT_CLK,
				ax_base + AX88796_MII_EEPROM);
	}

	return (retval>>1) & 0xffff;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: mdio_write
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static void mdio_write (struct net_device *ndev, int phy_id, int loc, int value)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	u_int cmd = (0x05<<28)|(phy_id<<23)|(loc<<18)|(1<<17)|value;
	int i;

	mdio_sync (ndev);
	for (i = 31; i >= 0; i--) {
		int dat = (cmd&(1<<i)) ? MDIO_DATA_WRITE1 : MDIO_DATA_WRITE0;
		writeb (dat, ax_base + AX88796_MII_EEPROM);
		writeb (dat | MDIO_SHIFT_CLK, ax_base + AX88796_MII_EEPROM);
	}
	for (i = 1; i >= 0; i--) {
		writeb (MDIO_ENB_IN, ax_base + AX88796_MII_EEPROM);
		writeb (MDIO_ENB_IN | MDIO_SHIFT_CLK,
				ax_base + AX88796_MII_EEPROM);
	}

}


#if (CONFIG_AX88796B_EEPROM_READ_WRITE == 1)
/*  
 *  ======================================================================
 *   EEPROM interface support
 *  ======================================================================
 */
#define EEPROM_SHIFT_CLK			(0x80)
#define EEPROM_DATA_READ1			(0x40)
#define EEPROM_DATA_WRITE0			(0x00)
#define EEPROM_DATA_WRITE1			(0x20)
#define EEPROM_SELECT				(0x10)
#define EEPROM_DIR_IN				(0x02)

#define EEPROM_READ				(0x02)
#define EEPROM_EWEN				(0x00)
#define EEPROM_ERASE				(0x03)
#define EEPROM_WRITE				(0x01)
#define EEPROM_ERALL				(0x00)
#define EEPROM_WRAL				(0x00)
#define EEPROM_EWDS				(0x00)
#define EEPROM_93C46_OPCODE(x)			((x) << 6)
#define EEPROM_93C46_STARTBIT			(1 << 8)

/*
 * ----------------------------------------------------------------------------
 * Function Name: eeprom_write_en
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static void
eeprom_write_en (struct net_device *ndev)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;

	unsigned short cmd = EEPROM_93C46_STARTBIT | 
			     EEPROM_93C46_OPCODE(EEPROM_EWEN) | 0x30;
	unsigned char tmp;
	int i;

	// issue a "SB OP Addr" command
	for (i = 8; i >= 0 ; i--) {
		tmp = (cmd & (1 << i)) == 0 ? 
				EEPROM_DATA_WRITE0 : EEPROM_DATA_WRITE1;
		writeb (tmp | EEPROM_SELECT, ax_base + AX88796_MII_EEPROM);
		writeb (tmp | EEPROM_SELECT | EEPROM_SHIFT_CLK,
				ax_base + AX88796_MII_EEPROM);
	}

	for (i = 15; i >= 0; i--) {
		writeb (EEPROM_DATA_WRITE0 | EEPROM_SELECT,
				ax_base + AX88796_MII_EEPROM);
		writeb (EEPROM_DATA_WRITE0 | EEPROM_SELECT | EEPROM_SHIFT_CLK,
				ax_base + AX88796_MII_EEPROM);
	}

	writeb (0, ax_base + AX88796_MII_EEPROM);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: eeprom_write_dis
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static void
eeprom_write_dis (struct net_device *ndev)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;

	unsigned short cmd = EEPROM_93C46_STARTBIT |
			     EEPROM_93C46_OPCODE (EEPROM_EWDS);
	unsigned char tmp;
	int i;

	// issue a "SB OP Addr" command
	for (i = 8; i >= 0 ; i--) {
		tmp = (cmd & (1 << i)) == 0 ? 
				EEPROM_DATA_WRITE0 : EEPROM_DATA_WRITE1;
		writeb (tmp | EEPROM_SELECT, ax_base + AX88796_MII_EEPROM);
		writeb (tmp | EEPROM_SELECT | EEPROM_SHIFT_CLK,
				ax_base + AX88796_MII_EEPROM);
	}

	for (i = 15; i >= 0; i--) {
		writeb (EEPROM_DATA_WRITE0 | EEPROM_SELECT,
				ax_base + AX88796_MII_EEPROM);
		writeb (EEPROM_DATA_WRITE0 | EEPROM_SELECT | EEPROM_SHIFT_CLK,
				ax_base + AX88796_MII_EEPROM);
	}

	writeb (0, ax_base + AX88796_MII_EEPROM);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: eeprom_write
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static int
eeprom_write (struct net_device *ndev, unsigned char loc, unsigned short nValue)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;

	unsigned short cmd = EEPROM_93C46_STARTBIT |
			     EEPROM_93C46_OPCODE (EEPROM_WRITE) | loc;
	unsigned char tmp;
	int i;
	int ret = 0;

	// issue a "SB OP Addr" command
	for(i = 8; i >= 0 ; i--) {
		tmp = (cmd & (1 << i)) == 0 ? 
				EEPROM_DATA_WRITE0 : EEPROM_DATA_WRITE1;
		writeb (tmp | EEPROM_SELECT, ax_base + AX88796_MII_EEPROM);
		writeb (tmp | EEPROM_SELECT | EEPROM_SHIFT_CLK,
				ax_base + AX88796_MII_EEPROM);
	}

	// writing the data
	for (i = 15; i >= 0; i--) {
		tmp = (nValue & (1 << i)) == 0 ?
				EEPROM_DATA_WRITE0 : EEPROM_DATA_WRITE1;
		writeb (tmp | EEPROM_SELECT, ax_base + AX88796_MII_EEPROM);
		writeb (tmp | EEPROM_SELECT | EEPROM_SHIFT_CLK,
				ax_base + AX88796_MII_EEPROM);
	}

	//
	// check busy
	//

	// Turn, wait two clocks
	writeb (EEPROM_DIR_IN, ax_base + AX88796_MII_EEPROM);
	writeb (EEPROM_SHIFT_CLK | EEPROM_DIR_IN, ax_base + AX88796_MII_EEPROM);
	writeb (EEPROM_DIR_IN, ax_base + AX88796_MII_EEPROM);
	writeb (EEPROM_SHIFT_CLK | EEPROM_DIR_IN, ax_base + AX88796_MII_EEPROM);

	// waiting for busy signal
	i = 0xFFFF;
	while (--i) {
		writeb (EEPROM_SELECT | EEPROM_DIR_IN,
				ax_base + AX88796_MII_EEPROM);
		writeb (EEPROM_SELECT | EEPROM_DIR_IN | EEPROM_SHIFT_CLK,
				ax_base + AX88796_MII_EEPROM);
		tmp = readb (ax_base + AX88796_MII_EEPROM);
		if ((tmp & EEPROM_DATA_READ1) == 0)
			break;
	}
	if (i <= 0) {
		PRINTK (ERROR_MSG, PFX
			"Failed on waiting for EEPROM bus busy signal\n");
		ret = -1;
	} else {
		i = 0xFFFF;
		while (--i) {
			writeb (EEPROM_SELECT | EEPROM_DIR_IN,
					ax_base + AX88796_MII_EEPROM);
			writeb ((EEPROM_SELECT | EEPROM_DIR_IN | 
				 EEPROM_SHIFT_CLK),
				 ax_base + AX88796_MII_EEPROM);
			tmp = readb (ax_base + AX88796_MII_EEPROM);
			if (tmp & EEPROM_DATA_READ1)
				break;
		}

		if (i <= 0) {
			PRINTK (ERROR_MSG, PFX
				"Failed on waiting for EEPROM completion\n");
			ret = -1;
		}
	}

	writeb (0, ax_base + AX88796_MII_EEPROM);

	return ret;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: eeprom_read
 * Purpose: 
 * ----------------------------------------------------------------------------
 */
static unsigned short
eeprom_read (struct net_device *ndev, unsigned char loc)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;

	unsigned short cmd = EEPROM_93C46_STARTBIT |
			     EEPROM_93C46_OPCODE (EEPROM_READ) | loc;
	unsigned char tmp;
	unsigned short retValue = 0;
	int i;

	// issue a "SB OP Addr" command
	for (i = 8; i >= 0 ; i--) {
		tmp = (cmd & (1 << i)) == 0 ? 
		EEPROM_DATA_WRITE0 : EEPROM_DATA_WRITE1;
		writeb (tmp | EEPROM_SELECT, ax_base + AX88796_MII_EEPROM);
		writeb (tmp | EEPROM_SELECT | EEPROM_SHIFT_CLK,
			ax_base + AX88796_MII_EEPROM);
	}

	// Turn
	writeb (EEPROM_DIR_IN | EEPROM_SELECT, ax_base + AX88796_MII_EEPROM);
	writeb (EEPROM_DIR_IN | EEPROM_SELECT | 
		EEPROM_SHIFT_CLK | EEPROM_DATA_WRITE1,
		ax_base + AX88796_MII_EEPROM);

	// retriving the data
	for (i = 15; i >= 0; i--) {
		writeb (EEPROM_SELECT | EEPROM_DIR_IN,
			ax_base + AX88796_MII_EEPROM);
		tmp = readb (ax_base + AX88796_MII_EEPROM);
		if (tmp & EEPROM_DATA_READ1)
			retValue |= (1 << i);
		writeb (EEPROM_SELECT | EEPROM_DIR_IN | EEPROM_SHIFT_CLK,
			ax_base + AX88796_MII_EEPROM);
	}

	writeb (0, ax_base + AX88796_MII_EEPROM);

	return retValue;
}
#endif /* #if (CONFIG_AX88796B_EEPROM_READ_WRITE == 1) */

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_dump_regs
 * Purpose: Dump all MAC registers
 * ----------------------------------------------------------------------------
 */
static void ax88796b_dump_regs (struct ax_device *ax_local)
{
	void __iomem *ax_base = ax_local->membase;
	u8 cr, i, j;

	cr = readb (ax_base + E8390_CMD) & E8390_PAGE_MASK;
	PRINTK (DEBUG_MSG, "       Page 0   Page 1   Page 2   Page 3\n");
	for (i = 0; i < 0x1F; i++) {

		PRINTK (DEBUG_MSG, " 0x%02x   ", i);
		for (j = 0; j < 4; j++) {
			writeb (cr | (j << 6), ax_base + E8390_CMD);

			PRINTK (DEBUG_MSG, "0x%02x     ",
				readb (ax_base + EI_SHIFT (i)));
		}
		PRINTK (DEBUG_MSG, "\n");
	}
	PRINTK (DEBUG_MSG, "\n");

	writeb (cr | E8390_PAGE0, ax_base + E8390_CMD);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_dump_phy_regs
 * Purpose: Dump PHY register from MR0 to MR5
 * ----------------------------------------------------------------------------
 */
static void ax88796b_dump_phy_regs (struct ax_device *ax_local)
{
	int i;

	PRINTK (DEBUG_MSG, "Dump PHY registers:\n");
	for (i = 0; i < 6; i++) {
		PRINTK (DEBUG_MSG, "  MR%d = 0x%04x\n", i,
			mdio_read (ax_local->ndev, 0x10, i));
	}
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_get_drvinfo
 * Purpose: Exported for Ethtool to query the driver version
 * ----------------------------------------------------------------------------
 */
static void ax88796b_get_drvinfo (struct net_device *ndev,
				 struct ethtool_drvinfo *info)
{
	/* Inherit standard device info */
	strncpy (info->driver, DRV_NAME, sizeof info->driver);
	strncpy (info->version, DRV_VERSION, sizeof info->version);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_get_link
 * Purpose: Exported for Ethtool to query the media link status
 * ----------------------------------------------------------------------------
 */
static u32 ax88796b_get_link (struct net_device *ndev)
{
	struct ax_device *ax_local = ax_get_priv (ndev);

	return ax_get_link (ax_local);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_get_wol
 * Purpose: Exported for Ethtool to query wake on lan setting
 * ----------------------------------------------------------------------------
 */
static void
ax88796b_get_wol (struct net_device *ndev, struct ethtool_wolinfo *wolinfo)
{
	struct ax_device *ax_local = ax_get_priv (ndev);

	wolinfo->supported = WAKE_PHY | WAKE_MAGIC;
	wolinfo->wolopts = 0;
	
	if (ax_local->wol & WAKEUP_LSCWE)
		wolinfo->wolopts |= WAKE_PHY;
	if (ax_local->wol & WAKE_MAGIC)
		wolinfo->wolopts |= WAKE_MAGIC;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_set_wol
 * Purpose: Exported for Ethtool to set the wake on lan setting
 * ----------------------------------------------------------------------------
 */
static int
ax88796b_set_wol (struct net_device *ndev, struct ethtool_wolinfo *wolinfo)
{
	struct ax_device *ax_local = ax_get_priv (ndev);

	ax_local->wol = 0;

	if (wolinfo->wolopts & WAKE_PHY)
		ax_local->wol |= WAKEUP_LSCWE;
	if (wolinfo->wolopts & WAKE_MAGIC)
		ax_local->wol |= WAKEUP_MP;

	return 0;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_get_settings
 * Purpose: Exported for Ethtool to query PHY setting
 * ----------------------------------------------------------------------------
 */
static int
ax88796b_get_settings (struct net_device *ndev, struct ethtool_cmd *cmd)
{
	struct ax_device *ax_local = ax_get_priv (ndev);

	return mii_ethtool_gset(&ax_local->mii, cmd);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_set_settings
 * Purpose: Exported for Ethtool to set PHY setting
 * ----------------------------------------------------------------------------
 */
static int
ax88796b_set_settings (struct net_device *ndev, struct ethtool_cmd *cmd)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	int retval;

	retval = mii_ethtool_sset (&ax_local->mii, cmd);

	return retval;

}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_nway_reset
 * Purpose: Exported for Ethtool to restart PHY autonegotiation
 * ----------------------------------------------------------------------------
 */
static int ax88796b_nway_reset (struct net_device *ndev)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	return mii_nway_restart(&ax_local->mii);
}

struct ethtool_ops ax88796b_ethtool_ops = {
	.get_drvinfo		= ax88796b_get_drvinfo,
	.get_link		= ax88796b_get_link,
	.get_wol		= ax88796b_get_wol,
	.set_wol		= ax88796b_set_wol,
	.get_settings		= ax88796b_get_settings,
	.set_settings		= ax88796b_set_settings,
	.nway_reset		= ax88796b_nway_reset,
};

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_load_macaddr
 * Purpose: Load MAC address from EEPROM
 * ----------------------------------------------------------------------------
 */
static void
ax88796b_load_macaddr (struct net_device *ndev, unsigned char *pMac)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	int i;
	struct {unsigned char value, offset; } program_seq[] =
	{
		{E8390_NODMA | E8390_PAGE0 | E8390_STOP, E8390_CMD},
		{ax_local->bus_width, EN0_DCFG},
		{0x00, EN0_RCNTLO},
		{0x00, EN0_RCNTHI},
		{0x00, EN0_IMR},
		{0xFF, EN0_ISR},
		{E8390_RXOFF, EN0_RXCR},
		{E8390_TXOFF, EN0_TXCR},
		{0x0C, EN0_RCNTLO},
		{0x00, EN0_RCNTHI},
		{0x00, EN0_RSARLO},
		{0x00, EN0_RSARHI},
		{E8390_RREAD | E8390_START, E8390_CMD},
	};

	/* Read the 12 bytes of station address PROM. */
	for (i = 0; i < sizeof (program_seq)/sizeof (program_seq[0]); i++)
		writeb (program_seq[i].value, ax_base + program_seq[i].offset);

	while (( readb (ax_base + EN0_SR) & 0x20) == 0);

	for (i = 0; i < 6; i++)
		pMac[i] = (unsigned char) READ_FIFO (ax_base + EN0_DATAPORT);

	writeb (ENISR_RDC, ax_base + EN0_ISR);	/* Ack intr. */

	// [FALINUX] - BootLoader 에서 맥주소를 가져 온다.
	AX88796_GET_MAC_ADDR(pMac);

	/* Support for No EEPROM */
	if (!is_valid_ether_addr (pMac)) {
		PRINTK (DRIVER_MSG, "Use random MAC address\n");
		random_ether_addr(pMac);
	}
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_set_macaddr
 * Purpose: Set MAC addres into hardware
 * ----------------------------------------------------------------------------
 */
static void ax88796b_set_macaddr (struct net_device *ndev)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	u8 i;

	/* Copy the station address into the DS8390 registers. */
	writeb (E8390_NODMA | E8390_PAGE1, ax_base + E8390_CMD);
	writeb (NESM_START_PG + TX_PAGES + 1, ax_base + EN1_CURPAG);
	for (i = 0; i < 6; i++) 
	{
		writeb (ndev->dev_addr[i], ax_base + EN1_PHYS_SHIFT (i));
		if (readb (ax_base + EN1_PHYS_SHIFT (i))!=ndev->dev_addr[i])
			PRINTK (ERROR_MSG, PFX
				"Hw. address read/write mismap %d\n", i);
	}
	writeb (E8390_NODMA | E8390_PAGE0, ax_base + E8390_CMD);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_set_mac_address
 * Purpose: Exported for upper layer
 * ----------------------------------------------------------------------------
 */
static int ax88796b_set_mac_address (struct net_device *ndev, void *p)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	struct sockaddr *addr = p;
	unsigned long flags;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(ndev->dev_addr, addr->sa_data, ndev->addr_len);

	spin_lock_irqsave (&ax_local->page_lock, flags);
	ax88796b_set_macaddr (ndev);
	spin_unlock_irqrestore (&ax_local->page_lock, flags);

	return 0;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_reset
 * Purpose: Reset AX88796B MAC
 * ----------------------------------------------------------------------------
 */
static void ax88796b_reset (struct net_device *ndev)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	unsigned long reset_start_time = jiffies;

	readb (ax_base + EN0_RESET);

	ax_local->dmaing = 0;

	while ((readb (ax_base + EN0_SR) & ENSR_DEV_READY) == 0) {
		if (jiffies - reset_start_time > 2*HZ/100) {
			PRINTK (ERROR_MSG, PFX
				" reset did not complete.\n");
			break;
		}
	}
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_get_stats
 * Purpose: Collect the stats.
 * ----------------------------------------------------------------------------
 */
static struct net_device_stats *ax88796b_get_stats (struct net_device *ndev)
{
 	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase; 
	unsigned long flags;

	/* If the card is stopped, just return the present stats. */
	if (!netif_running (ndev))
		return &ax_local->stat;

	spin_lock_irqsave (&ax_local->page_lock,flags);
	/* Read the counter registers, assuming we are in page 0. */
	ax_local->stat.rx_frame_errors += readb (ax_base + EN0_COUNTER0);
	ax_local->stat.rx_crc_errors   += readb (ax_base + EN0_COUNTER1);
	ax_local->stat.rx_missed_errors+= readb (ax_base + EN0_COUNTER2);
	spin_unlock_irqrestore (&ax_local->page_lock, flags);

	return &ax_local->stat;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: make_mc_bits
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static inline void make_mc_bits (u8 *bits, struct net_device *ndev)
{
	u32 crc;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
	struct dev_mc_list *dmi;
	for (dmi=ndev->mc_list; dmi; dmi=dmi->next) {
		if (dmi->dmi_addrlen != ETH_ALEN) {
			PRINTK (INIT_MSG, PFX
				" invalid multicast address length given.\n");
			continue;
		}
		crc = ether_crc (ETH_ALEN, dmi->dmi_addr);
		/* 
		 * The 8390 uses the 6 most significant bits of the
		 * CRC to index the multicast table.
		 */
		bits[crc >> 29] |= (1 << ((crc >> 26) & 7));
	}
#else
	struct netdev_hw_addr *ha;
	netdev_for_each_mc_addr (ha, ndev) {
		crc = ether_crc (ETH_ALEN, ha->addr);
		bits[crc >> 29] |= (1 << ((crc >> 26) & 7));
	}
#endif
}


/*
 * ----------------------------------------------------------------------------
 * Function Name: do_set_multicast_list
 * Purpose: Set RX mode and multicast filter
 * ----------------------------------------------------------------------------
 */
static void do_set_multicast_list (struct net_device *ndev)
{
 	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase; 
	int i;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
	int mc_count = ndev->mc_count;
#else
	int mc_count = netdev_mc_count (ndev);
#endif

	if (!(ndev->flags & (IFF_PROMISC | IFF_ALLMULTI))) {
		memset (ax_local->mcfilter, 0, 8);
		if (mc_count)
			make_mc_bits (ax_local->mcfilter, ndev);
	} else {
		/* mcast set to accept-all */
		memset (ax_local->mcfilter, 0xFF, 8);
	}

	if (netif_running (ndev))
		writeb (E8390_RXCONFIG, ax_base + EN0_RXCR);

	writeb (E8390_NODMA | E8390_PAGE1, ax_base + E8390_CMD);
	for (i = 0; i < 8; i++) {
		writeb (ax_local->mcfilter[i], ax_base + EN1_MULT_SHIFT (i));
	}
	writeb (E8390_NODMA | E8390_PAGE0, ax_base + E8390_CMD);

  	if (ndev->flags&IFF_PROMISC)
  		writeb (E8390_RXCONFIG | 0x18, ax_base + EN0_RXCR);
	else if (ndev->flags & IFF_ALLMULTI || mc_count) 
  		writeb (E8390_RXCONFIG | 0x08, ax_base + EN0_RXCR);
	else 
  		writeb (E8390_RXCONFIG, ax_base + EN0_RXCR);
 }


/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_set_multicast_list
 * Purpose: Exported for upper layer
 * ----------------------------------------------------------------------------
 */
static void ax88796b_set_multicast_list (struct net_device *ndev)
{
	unsigned long flags;
	struct ax_device *ax_local = ax_get_priv (ndev);

	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	spin_lock_irqsave (&ax_local->page_lock, flags);
	do_set_multicast_list (ndev);
	spin_unlock_irqrestore (&ax_local->page_lock, flags);

	PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);
}	


/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796_PHY_init
 * Purpose: Initialize PHY media mode
 * ----------------------------------------------------------------------------
 */
static int ax88796_PHY_init (struct net_device *ndev, u8 echo)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	u16 advertise;
	u16 phy14h, phy15h, phy16h;
	u8 rty_cnt;

	/* Enable AX88796B FOLW CONTROL */
	writeb (ENFLOW_ENABLE, ax_base+EN0_FLOW);

	rty_cnt = 0;
restart:
	mdio_write (ndev, 0x10, 0x14, 0x742C);
	phy14h = mdio_read (ndev, 0x10, 0x14);
	phy15h = mdio_read (ndev, 0x10, 0x15);
	phy16h = mdio_read (ndev, 0x10, 0x16);
	if ((phy14h != 0x742C) || (phy15h != 0x03c8) || (phy16h != 0x4044)) {
		mdio_write (ndev, 0x10, 0x15, 0x03c8);
		mdio_write (ndev, 0x10, 0x16, 0x4044);
		if (rty_cnt == 0) {
			PRINTK (DRIVER_MSG, PFX "Restore PHY default setting");
		} else if (++rty_cnt > 10) {
			PRINTK (ERROR_MSG, PFX
				"Failed to restore PHY default setting");
			return -EIO;
		}
		goto restart;
	}

	advertise = mdio_read (ndev, 0x10, MII_ADVERTISE) & ~ADVERTISE_ALL;

	switch (ax_local->media) {
	case MEDIA_AUTO:
		if (echo)
			PRINTK (DRIVER_MSG, PFX 
				" The media mode is autosense.\n");
		advertise |= ADVERTISE_ALL | 0x400;
		break;

	case MEDIA_100FULL:
		if (echo)
			PRINTK (DRIVER_MSG, PFX 
				" The media mode is forced to 100full.\n");
		advertise |= ADVERTISE_100FULL | 0x400;
		break;

	case MEDIA_100HALF:
		if (echo)
			PRINTK (DRIVER_MSG, PFX 
				" The media mode is forced to 100half.\n");
		advertise |= ADVERTISE_100HALF;
		break;

	case MEDIA_10FULL:
		if (echo)
			PRINTK (DRIVER_MSG, PFX 
				" The media mode is forced to 10full.\n");
		advertise |= ADVERTISE_10FULL;
		break;

	case MEDIA_10HALF:
		if (echo)
			PRINTK (DRIVER_MSG, PFX 
				" The media mode is forced to 10half.\n");
		advertise |= ADVERTISE_10HALF;
		break;
	default:
		advertise |= ADVERTISE_ALL | 0x400;
		break;
	}

	mdio_write (ndev, 0x10, MII_ADVERTISE, advertise);
	mii_nway_restart (&ax_local->mii);

	return 0;
}


/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_init
 * Purpose: Initialize MAC registers
 * ----------------------------------------------------------------------------
 */
static int ax88796b_init (struct net_device *ndev, int startp)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	int ret = 0;

	/* Follow National Semi's recommendations for initing the DP83902. */
	writeb (E8390_NODMA | E8390_PAGE0 | E8390_STOP, ax_base + E8390_CMD);
	writeb (ax_local->bus_width, ax_base + EN0_DCFG);/* 8-bit or 16-bit */

//	/* Set AX88796B interrupt active high */
//	writeb (ENBTCR_INT_ACT_HIGH, ax_base + EN0_BTCR);

	/* Clear the remote byte count registers. */
	writeb (0x00,  ax_base + EN0_RCNTLO);
	writeb (0x00,  ax_base + EN0_RCNTHI);

	/* Set to monitor and loopback mode -- this is vital!. */
	writeb (E8390_RXOFF, ax_base + EN0_RXCR); /* 0x20 */
	writeb (E8390_TXOFF, ax_base + EN0_TXCR); /* 0x02 */

	/* Set the transmit page and receive ring. */
	writeb (NESM_START_PG, ax_base + EN0_TPSR);
	writeb (NESM_RX_START_PG, ax_base + EN0_STARTPG);
	writeb (NESM_RX_START_PG, ax_base + EN0_BOUNDARY);

	/* assert boundary+1 */
	ax_local->current_page = NESM_RX_START_PG + 1;
	writeb (NESM_STOP_PG, ax_base + EN0_STOPPG);

	ax_local->tx_prev_ctepr = 0;
	ax_local->tx_start_page = NESM_START_PG;
	ax_local->tx_curr_page = NESM_START_PG;
	ax_local->tx_stop_page = NESM_START_PG + TX_PAGES;

	/* Clear the pending interrupts and mask. */
	writeb (0xFF, ax_base + EN0_ISR);
	writeb (0x00,  ax_base + EN0_IMR);

	ax88796b_set_macaddr (ndev);

	if (startp) 
	{
		ret = ax88796_PHY_init (ndev, 1);
		if (ret != 0)
			return ret;

		/* Enable AX88796B TQC */
		writeb ((readb (ax_base+EN0_MCR) | ENTQC_ENABLE),
			ax_base+EN0_MCR);
	
		/* Enable AX88796B Transmit Buffer Ring */
		writeb (E8390_NODMA | E8390_PAGE3 | E8390_STOP,
				ax_base+E8390_CMD);
		writeb (ENTBR_ENABLE, ax_base+EN3_TBR);
		writeb (E8390_NODMA | E8390_PAGE0 | E8390_STOP,
				ax_base+E8390_CMD);

		writeb (0xff,  ax_base + EN0_ISR);
		writeb (ENISR_ALL, ax_base + EN0_IMR);
		writeb (E8390_NODMA | E8390_PAGE0 | E8390_START,
				ax_base+E8390_CMD);
		writeb (E8390_TXCONFIG, ax_base + EN0_TXCR); /* xmit on. */

		writeb (E8390_RXCONFIG, ax_base + EN0_RXCR); /* rx on,  */
		do_set_multicast_list (ndev);	/* (re)load the mcast table */
	}

	return ret;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax_media_link
 * Purpose: 
 * ----------------------------------------------------------------------------
 */
static void ax_media_link (struct net_device *ndev)
{
	u16 phy_reg;

	phy_reg = mdio_read (ndev, 0x10, MII_BMCR);

	PRINTK (DRIVER_MSG, "%s: link up, %sMbps, %s-duplex\n",
		ndev->name, (phy_reg & BMCR_SPEED100 ? "100" : "10"),
		(phy_reg & BMCR_FULLDPLX ? "Full" : "Half"));

	netif_carrier_on (ndev);
	netif_wake_queue (ndev);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax_get_link
 * Purpose: 
 * ----------------------------------------------------------------------------
 */
static int ax_get_link (struct ax_device *ax_local)
{
	u8 nway_done = 1;
	u16 phy_reg;

	phy_reg = mdio_read (ax_local->ndev, 0x10, MII_BMCR);
	if (phy_reg & BMCR_ANENABLE)
		nway_done = (mdio_read (ax_local->ndev, 0x10, MII_BMSR) &
					BMSR_ANEGCOMPLETE) ? 1 : 0;

	if (nway_done)
		return mii_link_ok (&ax_local->mii);

	return 0;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_watchdog
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static void ax88796b_watchdog (unsigned long arg)
{
	struct net_device *ndev = (struct net_device *)(arg);
 	struct ax_device *ax_local = ax_get_priv (ndev);
	unsigned long time_to_wake = AX88796_WATCHDOG_PERIOD_50MS;
	u8 link;
	u16 phy_reg;

	switch (ax_local->state) {
	case wait_autoneg:

		if (ax_get_link (ax_local)) {
			ax_media_link (ndev);
			ax_local->state = chk_link;
		} else if (ax_local->tick_times-- < 0) {
			ax_local->state = chk_cable_exist;
			ax_local->tick_times = (1800 / 50); //1.8s
		}
		break;
	case chk_link:
		link = ax_get_link (ax_local);

		if (netif_carrier_ok(ndev) != link) {

			if (link) {
				ax_media_link (ndev);
				ax_local->state = chk_link;
			} else {
				netif_stop_queue (ndev);
				netif_carrier_off (ndev);
				PRINTK (DRIVER_MSG, 
					"%s Link down.\n", ndev->name);
				ax_local->state = chk_cable_exist;
				ax_local->tick_times = (1800 / 50); //1.8s
			}
		}
		break;
	case chk_cable_exist:
		phy_reg = mdio_read (ndev, 0x10, 0x12);
		if ((phy_reg != 0x8012) && (phy_reg != 0x8013)) {
			mdio_write (ndev, 0x10, 0x16, 0x4040);
			mii_nway_restart(&ax_local->mii);
			ax_local->state = chk_cable_status;
			time_to_wake = AX88796_WATCHDOG_PERIOD_500MS;
			ax_local->tick_times = (4 * 1000 / 500); //4s
		} else if (--ax_local->tick_times <= 0) {
			mii_nway_restart(&ax_local->mii);
			ax_local->state = chk_cable_exist_again;
			ax_local->tick_times = (6 * 1000 / 50); //6s
		}
		break;
	case chk_cable_exist_again:
		/* if cable disconnected */
		phy_reg = mdio_read (ndev, 0x10, 0x12);
		if ((phy_reg != 0x8012) && (phy_reg != 0x8013)) {
			mii_nway_restart(&ax_local->mii);
			ax_local->state = chk_cable_status;
			time_to_wake = AX88796_WATCHDOG_PERIOD_500MS;
			ax_local->tick_times = (4 * 1000 / 500); //4s
		} else if (--ax_local->tick_times <= 0) {

			ax_local->phy_advertise = mdio_read (ndev, 0x10,
							     MII_ADVERTISE);
			ax_local->phy_bmcr = mdio_read (ndev, 0x10, MII_BMCR);
			mdio_write (ndev, 0x10, MII_BMCR, BMCR_PDOWN);
			time_to_wake = AX88796_WATCHDOG_PERIOD_3S;
			ax_local->state = phy_power_up;
		}
		break;

	case chk_cable_status:

		if (ax_get_link (ax_local)) {
			ax_local->state = chk_link;
		} else if (--ax_local->tick_times <= 0) {

			mdio_write (ndev, 0x10, 0x16, 0x4040);
			mii_nway_restart(&ax_local->mii);
			ax_local->state = chk_cable_exist_again;
			ax_local->tick_times = (6 * 1000 / 50); //6s
		} else {
			time_to_wake = AX88796_WATCHDOG_PERIOD_500MS;
		}
		break;
	case phy_power_up:
		/* Restore default setting */
		mdio_write (ndev, 0x10, MII_BMCR, ax_local->phy_bmcr);
		mdio_write (ndev, 0x10, MII_ADVERTISE, ax_local->phy_advertise);
		mdio_write (ndev, 0x10, 0x16, 0x4044);
		mii_nway_restart(&ax_local->mii);

		ax_local->state = chk_cable_exist_again;
		ax_local->tick_times = (6 * 1000 / 50); //6s
		break;
	default:
		break;
	}

	mod_timer (&ax_local->watchdog, jiffies + time_to_wake);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_get_hdr
 * Purpose: Grab the 796b specific header
 * ----------------------------------------------------------------------------
 */
static void
ax88796b_get_hdr (struct net_device *ndev, 
		  struct ax_pkt_hdr *hdr, int ring_page)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	u16 *buf = (u16 *)hdr;

	/* This shouldn't happen. If it does, it's the last thing you'll see */
	if (ax_local->dmaing)
	{
		PRINTK (ERROR_MSG, PFX
			" DMAing conflict in ax88796b_get_hdr"
			"[DMAstat:%d][irqlock:%d].\n",
			ax_local->dmaing, ax_local->irqlock);
		return;
	}

	ax_local->dmaing |= 0x01;

	writeb (E8390_NODMA | E8390_PAGE0, ax_base+ E8390_CMD);
	writeb (sizeof (struct ax_pkt_hdr), ax_base + EN0_RCNTLO);
	writeb (0, ax_base + EN0_RCNTHI);
	writeb (0, ax_base + EN0_RSARLO);		/* On page boundary */
	writeb (ring_page, ax_base + EN0_RSARHI);
	writeb (E8390_RREAD, ax_base + E8390_CMD);

	while (( readb (ax_base+EN0_SR) & ENSR_DMA_READY) == 0);

	*buf = READ_FIFO (ax_base + EN0_DATAPORT);
	*(++buf) = READ_FIFO (ax_base + EN0_DATAPORT);

	writeb (ENISR_RDC, ax_base + EN0_ISR);	/* Ack intr. */
	ax_local->dmaing = 0;

	le16_to_cpus (&hdr->count);
}


/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_block_input
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static void 
ax88796b_block_input (struct net_device *ndev, int count,
			struct sk_buff *skb, int ring_offset)
{
    struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	u16 *buf = (u16 *)skb->data;
	u16 i;

	/* This shouldn't happen. If it does, it's the last thing you'll see */
	if (ax_local->dmaing)
	{
		PRINTK (ERROR_MSG, PFX " DMAing conflict in ne_block_input "
			"[DMAstat:%d][irqlock:%d].\n",
			ax_local->dmaing, ax_local->irqlock);
		return;
	}

	ax_local->dmaing |= 0x01;

	writeb (E8390_NODMA | E8390_PAGE0, ax_base + E8390_CMD);
	writeb (count & 0xff, ax_base + EN0_RCNTLO);
	writeb (count >> 8, ax_base + EN0_RCNTHI);
	writeb (ring_offset & 0xff, ax_base + EN0_RSARLO);
	writeb (ring_offset >> 8, ax_base + EN0_RSARHI);
	writeb (E8390_RREAD, ax_base + E8390_CMD);


	while ((readb (ax_base+EN0_SR) & ENSR_DMA_READY) == 0);

#if (CONFIG_AX88796B_USE_MEMCPY == 1)
	{
		/* make the burst length be divided for 32-bit */
		i = ((count - 2) + 3) & 0x7FC;

		/* Read first 2 bytes */
		*buf = READ_FIFO (ax_base + EN0_DATAPORT);

		/* The address of ++buf should be agigned on 32-bit boundary */
		memcpy (++buf, ax_base+EN0_DATA_ADDR, i);
	}
#else
	{
		for (i = 0; i < count; i += 2) {
			*buf++ = READ_FIFO (ax_base + EN0_DATAPORT);
		}
	}
#endif

	writeb (ENISR_RDC, ax_base + EN0_ISR);	/* Ack intr. */
	ax_local->dmaing = 0;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax_trigger_send
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static void 
ax_trigger_send (struct net_device *ndev, unsigned int length, int start_page)
{
 	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;

	writeb (E8390_NODMA | E8390_PAGE0, ax_base + E8390_CMD);
	writeb (length & 0xff, ax_base + EN0_TCNTLO);
	writeb (length >> 8, ax_base + EN0_TCNTHI);
	writeb (start_page, ax_base + EN0_TPSR);

	writeb (E8390_NODMA | E8390_TRANS, ax_base + E8390_CMD);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax_block_output
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static void 
ax_block_output (struct net_device *ndev, int count,
		 const unsigned char *buf, const int start_page)
{
    struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	unsigned long dma_start;

	/* This shouldn't happen. If it does, it's the last thing you'll see */
	if (ax_local->dmaing)
	{
		PRINTK (ERROR_MSG, PFX " DMAing conflict in ne_block_output."
			"[DMAstat:%d][irqlock:%d]\n",
			ax_local->dmaing, ax_local->irqlock);
		return;
	}

	ax_local->dmaing |= 0x01;

	/* Now the normal output. */
	writeb (count & 0xff, ax_base + EN0_RCNTLO);
	writeb (count >> 8,   ax_base + EN0_RCNTHI);
	writeb (0x00, ax_base + EN0_RSARLO);
	writeb (start_page, ax_base + EN0_RSARHI);

	writeb (E8390_RWRITE, ax_base + E8390_CMD);

#if (CONFIG_AX88796B_USE_MEMCPY == 1)
	memcpy ((ax_base+EN0_DATA_ADDR), buf, ((count+ 3) & 0x7FC));
#else
	{
		u16 i;
		for (i = 0; i < count; i += 2) {
			WRITE_FIFO (ax_base + EN0_DATAPORT, 
					*((u16 *)(buf + i)));
		}
	}
#endif

	dma_start = jiffies;
	while ((readb(ax_base + EN0_ISR) & 0x40) == 0) {
		if (jiffies - dma_start > 2*HZ/100) {		/* 20ms */
			PRINTK (ERROR_MSG, PFX
				" timeout waiting for Tx RDC.\n");
			ax88796b_reset (ndev);
			ax88796b_init (ndev, 1);
			break;
		}
	}
	writeb (ENISR_RDC, ax_base + EN0_ISR);	/* Ack intr. */

	ax_local->dmaing = 0;
	return;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_start_xmit
 * Purpose: begin packet transmission
 * ----------------------------------------------------------------------------
 */
static int ax88796b_start_xmit (struct sk_buff *skb, struct net_device *ndev)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	int send_length;
	unsigned long flags;
	u8 ctepr=0, free_pages=0, need_pages;

	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	send_length = skb->len;

	spin_lock_irqsave (&ax_local->page_lock, flags);

	writeb (E8390_PAGE0 | E8390_NODMA, ax_base + E8390_CMD);

	writeb (0x00, ax_base + EN0_IMR);
	ax_local->irqlock = 1;

	need_pages = ((send_length - 1) >> 8) + 1;

	ctepr = readb (ax_base + EN0_CTEPR);

	if (ctepr & ENCTEPR_TXCQF) {
		free_pages = 0;
	} else if (ctepr == 0) {

		/*
		 * If someone issues STOP bit of command register at run time,
		 * the TX machine will stop the current transmission and the
		 * CTEPR register will be reset to zero.
		 * This is just precautionary measures.
		 */
		if (ax_local->tx_prev_ctepr != 0) {
			ax_local->tx_curr_page = ax_local->tx_start_page;
			ax_local->tx_prev_ctepr = 0;
		}

		free_pages = ax_local->tx_stop_page - ax_local->tx_curr_page;
	}
	else if (ctepr < ax_local->tx_curr_page - 1) {
		free_pages = ax_local->tx_stop_page - ax_local->tx_curr_page + 
					ctepr - ax_local->tx_start_page + 1;
	}
	else if (ctepr > ax_local->tx_curr_page - 1) {
		free_pages = ctepr + 1 - ax_local->tx_curr_page;
	}
	else if (ctepr == ax_local->tx_curr_page - 1) {
		if (ax_local->tx_full)
			free_pages = 0;
		else
			free_pages = TX_PAGES;
	}

	PRINTK (DEBUG_MSG, PFX " tx pkt len %d, need %d pages, free pages %d\n",
		skb->len, need_pages, free_pages);

	if (free_pages < need_pages) {
		PRINTK (DEBUG_MSG, "free_pages < need_pages\n");
		netif_stop_queue (ndev);
		ax_local->tx_full = 1;
		ax_local->irqlock = 0;	
		writeb (ENISR_ALL, ax_base + EN0_IMR);
		spin_unlock_irqrestore (&ax_local->page_lock, flags);
		return NETDEV_TX_BUSY;
	}

	if (DEBUG_MSG & DEBUG_FLAGS) {
		int i;
		PRINTK (DEBUG_MSG, PFX " Dump tx pkt %d", skb->len);
		for (i = 0; i < skb->len; i++) {
			if ((i % 16) == 0)
				PRINTK (DEBUG_MSG, "\n");
			PRINTK (DEBUG_MSG, 
				"%02x ", *(skb->data + i));
		}
		PRINTK (DEBUG_MSG, "\n");
	}
	ax_block_output (ndev, send_length, skb->data, ax_local->tx_curr_page);
	ax_trigger_send (ndev, send_length, ax_local->tx_curr_page);
	if (free_pages == need_pages) {
		netif_stop_queue (ndev);
		ax_local->tx_full = 1;
	}
	ax_local->tx_prev_ctepr = ctepr;
	ax_local->tx_curr_page = ((ax_local->tx_curr_page + need_pages) <
		ax_local->tx_stop_page) ? 
		(ax_local->tx_curr_page + need_pages) : 
		(need_pages - (ax_local->tx_stop_page - ax_local->tx_curr_page)
		 + ax_local->tx_start_page);

	ax_local->irqlock = 0;	
	writeb (ENISR_ALL, ax_base + EN0_IMR);

	spin_unlock_irqrestore (&ax_local->page_lock, flags);

	dev_kfree_skb (skb);

	ndev->trans_start = jiffies;
	ax_local->stat.tx_bytes += send_length;

	PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);

	return NETDEV_TX_OK;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax_tx_intr
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static void ax_tx_intr (struct net_device *ndev)
{
    struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	int status = readb (ax_base + EN0_TSR);

	ax_local->tx_full = 0;
	if (netif_queue_stopped (ndev))
		netif_wake_queue (ndev);

	/* Minimize Tx latency: update the statistics after we restart TXing. */
	if (status & ENTSR_COL)
		ax_local->stat.collisions++;
	if (status & ENTSR_PTX)
		ax_local->stat.tx_packets++;
	else 
	{
		ax_local->stat.tx_errors++;
		if (status & ENTSR_ABT) 
		{
			ax_local->stat.tx_aborted_errors++;
			ax_local->stat.collisions += 16;
		}
		if (status & ENTSR_CRS) 
			ax_local->stat.tx_carrier_errors++;
		if (status & ENTSR_FU) 
			ax_local->stat.tx_fifo_errors++;
		if (status & ENTSR_CDH)
			ax_local->stat.tx_heartbeat_errors++;
		if (status & ENTSR_OWC)
			ax_local->stat.tx_window_errors++;
	}
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax_tx_err
 * Purpose:
 * ----------------------------------------------------------------------------
 */
static void ax_tx_err (struct net_device *ndev)
{
    struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;

	unsigned char txsr = readb (ax_base+EN0_TSR);
	unsigned char tx_was_aborted = txsr & (ENTSR_ABT+ENTSR_FU);

	if (tx_was_aborted)
		ax_tx_intr (ndev);
	else {
		ax_local->stat.tx_errors++;
		if (txsr & ENTSR_CRS) ax_local->stat.tx_carrier_errors++;
		if (txsr & ENTSR_CDH) ax_local->stat.tx_heartbeat_errors++;
		if (txsr & ENTSR_OWC) ax_local->stat.tx_window_errors++;
	}
}
static int ax88796b_rx_poll(struct net_device *ndev, struct ax_device *ax_local,
		      int budget)
{
	void *ax_base = ax_local->membase;
	unsigned char rxing_page, this_frame, next_frame;
	unsigned short current_offset;
	struct ax_pkt_hdr rx_frame = {0,0,0,};
	int pkt_cnt = 0;
	int i;

	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	for ( i = 0 ; i < budget && netif_running(ndev); i++) {

		int pkt_len, pkt_stat;

		/* Get the rx page (incoming packet pointer). */
		rxing_page = readb (ax_base + EN0_CURPAG);

		/* 
		 * Remove one frame from the ring.
		 * Boundary is always a page behind.
		 */
		this_frame = readb (ax_base + EN0_BOUNDARY) + 1;
		if (this_frame >= ax_local->stop_page)
			this_frame = ax_local->rx_start_page;
		
		if (this_frame == rxing_page) {	/* Read all the frames? */
			break;				/* Done for now */
		}
		current_offset = this_frame << 8;
		ax88796b_get_hdr (ndev, &rx_frame, this_frame);

		PRINTK (DEBUG_MSG, PFX 
			" Current page 0x%x, boundary page 0x%x\n",
			rxing_page, this_frame);

		pkt_len = rx_frame.count - sizeof (struct ax_pkt_hdr);
		pkt_stat = rx_frame.status;
		next_frame = this_frame + 1 + ((pkt_len + 4) >> 8);

		if (pkt_len < 60  ||  pkt_len > 1518) {
			PRINTK (ERROR_MSG, PFX
				" bogus pkt size: %d, status=%#2x nxpg=%#2x.\n",
				rx_frame.count, rx_frame.status, rx_frame.next);
			ax_local->stat.rx_errors++;
			ax_local->stat.rx_length_errors++;
		} else if ((pkt_stat & 0x0F) == ENRSR_RXOK) {
			struct sk_buff *skb;
			int status;
			skb = dev_alloc_skb (pkt_len + 2);
			if (skb == NULL) {
				PRINTK (ERROR_MSG, PFX
					" Couldn't allocate a sk_buff"
					" of size %d.\n", pkt_len);
				ax_local->stat.rx_dropped++;
				break;
			}

			/* IP headers on 16 byte boundaries */
			skb_reserve (skb, 2);
			skb->dev = ndev;
			skb_put (skb, pkt_len);	/* Make room */
			ax88796b_block_input (ndev, pkt_len, skb,
					current_offset + sizeof (rx_frame));

			if (DEBUG_MSG & DEBUG_FLAGS) {
				int i;
				PRINTK (DEBUG_MSG, PFX 
					" Dump rx pkt %d", skb->len);
				for (i = 0; i < skb->len; i++) {
					if ((i % 16) == 0)
						PRINTK (DEBUG_MSG, "\n");
					PRINTK (DEBUG_MSG, 
						"%02x ", *(skb->data + i));
				}
				PRINTK (DEBUG_MSG, "\n");
			}

			skb->protocol = eth_type_trans (skb,ndev);

			status = netif_rx (skb);
			if (status != NET_RX_SUCCESS)
				PRINTK (ERROR_MSG,
					"netif_rx status %d\n", status);

			ndev->last_rx = jiffies;
			ax_local->stat.rx_packets++;
			ax_local->stat.rx_bytes += pkt_len;
			if (pkt_stat & ENRSR_PHY)
				ax_local->stat.multicast++;

			pkt_cnt++;
		} else {
			PRINTK (ERROR_MSG, PFX
				" bogus packet: status=%#2x"
				" nxpg=%#2x size=%d\n",
				rx_frame.status, rx_frame.next, rx_frame.count);
			ax_local->stat.rx_errors++;
			/* NB: The NIC counts CRC, frame and missed errors. */
			if (pkt_stat & ENRSR_FO)
				ax_local->stat.rx_fifo_errors++;
		}
		next_frame = rx_frame.next;

		ax_local->current_page = next_frame;
		writeb (next_frame-1, ax_base+EN0_BOUNDARY);

	}
	PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);

	return pkt_cnt;
}

static int ax88796b_poll(struct napi_struct *napi, int budget)
{
	struct ax_device *ax_local = container_of(napi, struct ax_device, napi);
	struct net_device *dev = ax_local->ndev;
	void *ax_base = ax_local->membase;
	int work_done;
	u8 cmd;
	unsigned long flags;

	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	spin_lock_irqsave(&ax_local->page_lock, flags);

	work_done = ax88796b_rx_poll(dev, ax_local, budget);

	
	if (work_done < budget) {

		cmd = readb (ax_base + E8390_CMD);
		writeb ((cmd & E8390_PAGE_MASK) , ax_base + E8390_CMD);
		writeb (ENISR_ALL, ax_base + EN0_IMR);
		 __napi_complete(napi);

	}

	
	spin_unlock_irqrestore(&ax_local->page_lock, flags);

	return work_done;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax_interrupt
 * Purpose:
 * ----------------------------------------------------------------------------
 */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
static irqreturn_t ax_interrupt (int irq, void *dev_id)
#else
static irqreturn_t ax_interrupt (int irq, void *dev_id, struct pt_regs * regs)
#endif
{
	struct net_device *ndev = dev_id;
	int interrupts;
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	u8 CurrImr;

	PRINTK (INT_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	if (ndev == NULL) 
	{
		PRINTK (ERROR_MSG,
			"net_interrupt(): irq %d for unknown device.\n", irq);
		return IRQ_RETVAL (0);
	}

	spin_lock (&ax_local->page_lock);

	writeb (E8390_NODMA | E8390_PAGE3, ax_base + E8390_CMD);

	CurrImr = readb (ax_base + EN0_IMR);

	writeb (E8390_NODMA | E8390_PAGE0, ax_base + E8390_CMD);
	writeb (0x00, ax_base + EN0_IMR);

	if (ax_local->irqlock) {
		printk ("Interrupt occurred when irqlock locked\n");
		spin_unlock (&ax_local->page_lock);
		return IRQ_RETVAL (0);
	}

	do {
		if ((interrupts = readb (ax_base + EN0_ISR)) == 0)
			break;

		writeb (interrupts, ax_base + EN0_ISR); /* Ack the interrupts */

		if (interrupts & ENISR_TX) {
			PRINTK (INT_MSG, PFX " TX int\n");
			ax_tx_intr (ndev);
		}

		if (interrupts & (ENISR_RX | ENISR_RX_ERR | ENISR_OVER)) {
			PRINTK (INT_MSG, PFX " RX int\n");

			if (napi_schedule_prep(&ax_local->napi)) {
				CurrImr = ENISR_ALL & ~(ENISR_RX | ENISR_RX_ERR | ENISR_OVER);
				__napi_schedule(&ax_local->napi);
			}
		}

		if (interrupts & ENISR_TX_ERR) {
			PRINTK (INT_MSG, PFX " TX err int\n");
			ax_tx_err (ndev);
		}

		if (interrupts & ENISR_COUNTERS) {   
			ax_local->stat.rx_frame_errors += 
					readb (ax_base + EN0_COUNTER0);
			ax_local->stat.rx_crc_errors += 
					readb (ax_base + EN0_COUNTER1);
			ax_local->stat.rx_missed_errors += 
					readb (ax_base + EN0_COUNTER2); 
			writeb (ENISR_COUNTERS, ax_base + EN0_ISR);
		}

		if (interrupts & ENISR_RDC)
			writeb (ENISR_RDC, ax_base + EN0_ISR);
	} while (0);

	writeb (CurrImr, ax_base + EN0_IMR);
	

	spin_unlock (&ax_local->page_lock);

	PRINTK (INT_MSG, PFX " %s end ..........\n", __FUNCTION__);

	return IRQ_RETVAL (1);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_open
 * Purpose: Open/initialize 796b
 * ----------------------------------------------------------------------------
 */
static int ax88796b_open (struct net_device *ndev)
{
	unsigned long flags;
	struct ax_device *ax_local = ax_get_priv (ndev);
	int ret = 0;

	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	ret = request_irq (ndev->irq, &ax_interrupt, 0, ndev->name, ndev);

	if (ret) {
		PRINTK (ERROR_MSG, PFX
			" unable to get IRQ %d (errno=%d).\n",
			ndev->irq, ret);
		return -ENXIO;
	}

	PRINTK (DEBUG_MSG, PFX " Request IRQ success !!\n");

	spin_lock_irqsave (&ax_local->page_lock, flags);

	ax88796b_reset (ndev);
	ret = ax88796b_init (ndev, 1);
	if (ret != 0) {
		spin_unlock_irqrestore (&ax_local->page_lock, flags);
		free_irq (ndev->irq, ndev);
		return ret;
	}

	if (DEBUG_MSG & DEBUG_FLAGS) {
		PRINTK (DEBUG_MSG, PFX
			"Dump MAC registers after initialization:\n");
		ax88796b_dump_regs (ax_local);
		ax88796b_dump_phy_regs (ax_local);
	}

	netif_carrier_off (ndev);
	netif_start_queue (ndev);
	ax_local->irqlock = 0;
	spin_unlock_irqrestore (&ax_local->page_lock, flags);

	ax_local->state = wait_autoneg;
	ax_local->tick_times = (4 * 1000 / 40); //4S

	init_timer (&ax_local->watchdog);
	ax_local->watchdog.function = &ax88796b_watchdog;
	ax_local->watchdog.expires = jiffies + AX88796_WATCHDOG_PERIOD_50MS;

	ax_local->watchdog.data = (unsigned long) ndev;
	add_timer (&ax_local->watchdog);

	napi_enable(&ax_local->napi);

	PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);

	return 0;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_close
 * Purpose: 
 * ----------------------------------------------------------------------------
 */
static int ax88796b_close (struct net_device *ndev)
{
	struct ax_device *ax_local = ax_get_priv (ndev);
	unsigned long flags;

 	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	del_timer_sync (&ax_local->watchdog);
	spin_lock_irqsave (&ax_local->page_lock, flags);
	ax88796b_init (ndev, 0);

	free_irq (ndev->irq, ndev);

	napi_disable(&ax_local->napi);

   	spin_unlock_irqrestore (&ax_local->page_lock, flags);
	netif_stop_queue (ndev);

	PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);
	return 0;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
const struct net_device_ops ax_netdev_ops = {
	.ndo_open		= ax88796b_open,
	.ndo_stop		= ax88796b_close,
	.ndo_start_xmit		= ax88796b_start_xmit,
	.ndo_get_stats		= ax88796b_get_stats,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
	.ndo_set_rx_mode = ax88796b_set_multicast_list,
#else
	.ndo_set_multicast_list = ax88796b_set_multicast_list,
#endif
	.ndo_set_mac_address 	= ax88796b_set_mac_address,
};
#endif

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax_probe
 * Purpose: 
 * ----------------------------------------------------------------------------
 */
static int ax_probe (struct net_device *ndev, struct ax_device *ax_local)
{
	int i;
	int reg0;
	void *ax_base = ax_local->membase;

	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	reg0 = readb (ax_base);
	if (reg0 == 0xFF) {
		return -ENODEV;
	}

	/* Do a preliminary verification that we have a 8390. */
	{
		int regd;
		writeb (E8390_NODMA | E8390_PAGE1 | E8390_STOP,
			ax_base + E8390_CMD);
		regd = readb (ax_base + EN0_COUNTER0);
		writeb (0xff, ax_base + EN0_COUNTER0);
		writeb (E8390_NODMA | E8390_PAGE0, ax_base + E8390_CMD);
		/* Clear the counter by reading. */
		readb (ax_base + EN0_COUNTER0);
		if (readb (ax_base + EN0_COUNTER0) != 0) {
			writeb (reg0, ax_base);
			/* Restore the old values. */
			writeb (regd, ax_base + EN0_COUNTER0);
			return -ENODEV;
		}
	}

	printk (version);

	if (DEBUG_MSG & DEBUG_FLAGS) {
		PRINTK (DEBUG_MSG, PFX
			"Dump MAC registers before initialization:\n");
		ax88796b_dump_regs (ax_local);
		ax88796b_dump_phy_regs (ax_local);
	}

	/* Reset card. */
	{
		unsigned long reset_start_time = jiffies;

		readb (ax_base + EN0_RESET);
		while ((readb (ax_base + EN0_SR) & ENSR_DEV_READY) == 0) {
			if (jiffies - reset_start_time > 2*HZ/100) {
				PRINTK (ERROR_MSG,
					" not found (no reset ack).\n");
					return -ENODEV;
				break;
			}
		}
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
	ndev->netdev_ops = &ax_netdev_ops;
#else
	ndev->open			= &ax88796b_open;
	ndev->stop			= &ax88796b_close;
	ndev->hard_start_xmit		= &ax88796b_start_xmit;
	ndev->get_stats			= &ax88796b_get_stats;
	ndev->set_multicast_list	= &ax88796b_set_multicast_list;
	ndev->set_mac_address		= ax88796b_set_mac_address,
#endif

	ndev->ethtool_ops = &ax88796b_ethtool_ops;

	ether_setup (ndev);

	ax_local->tx_start_page = NESM_START_PG;
	ax_local->rx_start_page = NESM_RX_START_PG;
	ax_local->stop_page = NESM_STOP_PG;
	ax_local->media = media;

	if (weight == 0) {
		netif_napi_add(ndev, &ax_local->napi, ax88796b_poll, 4);
		PRINTK (DRIVER_MSG, "NAPI_WEIGHT (default) = %d\n", 4);
	}
	else {
		netif_napi_add(ndev, &ax_local->napi, ax88796b_poll, weight);
		PRINTK (DRIVER_MSG, "NAPI_WEIGHT= %d\n", weight);
	}
#if (CONFIG_AX88796B_8BIT_WIDE == 1)
	ax_local->bus_width = 0;
#else
	ax_local->bus_width = 1;
#endif

	spin_lock_init (&ax_local->page_lock);

	ax88796b_load_macaddr (ndev, ndev->dev_addr);

	PRINTK (DRIVER_MSG, "%s: MAC ADDRESS ",DRV_NAME);
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		PRINTK (DRIVER_MSG, " %2.2x", ndev->dev_addr[i]);
	}

	PRINTK (DRIVER_MSG, "\n" PFX " found at 0x%x, using IRQ %d.\n",
		AX88796B_BASE, ndev->irq);

	/* Initialize MII structure */
	ax_local->mii.dev = ndev;
	ax_local->mii.mdio_read = mdio_read;
	ax_local->mii.mdio_write = mdio_write;
	ax_local->mii.phy_id = 0x10;
	ax_local->mii.phy_id_mask = 0x1f;
	ax_local->mii.reg_num_mask = 0x1f;

	ax88796b_init (ndev, 0);

	PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);

	return register_netdev (ndev);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_drv_probe
 * Purpose: Driver get resource and probe
 * ----------------------------------------------------------------------------
 */
static int ax88796b_drv_probe(struct platform_device *pdev)
{
	struct ax_device *ax_local;
	struct resource *res = NULL;
	void *addr;
	int ret;
	struct net_device *ndev;

	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	/* User can overwrite AX88796B's base address */
	if (!mem){

		res = platform_get_resource (pdev, IORESOURCE_MEM, 0);
		if (!res) {
			printk("%s: get no resource !\n", DRV_NAME);
			return -ENODEV;
		}

		mem = res->start;
	}

	/* Get the IRQ resource from kernel */
	if(!irq)
		irq = platform_get_irq(pdev, 0);

	/* Request the regions */
	if (!request_mem_region (mem, AX88796B_IO_EXTENT, "ax88796b")) {
		PRINTK (ERROR_MSG, PFX " request_mem_region fail !");
		return -EBUSY;
	}

	addr = ioremap (mem, AX88796B_IO_EXTENT);
	if (!addr) {
		ret = -EBUSY;
		goto release_region;
	}

	ndev = alloc_etherdev (sizeof (struct ax_device));
	if (!ndev) {
		PRINTK (ERROR_MSG, PFX " Could not allocate device.\n");
		ret = -ENOMEM;
		goto unmap_region;
	}

	SET_NETDEV_DEV(ndev, &pdev->dev);
	platform_set_drvdata(pdev, ndev);

	ndev->base_addr = mem;
	ndev->irq = irq;

	ax_local = ax_get_priv (ndev);
	ax_local->membase = addr;
 	ax_local->ndev = ndev;

	ret = ax_probe (ndev, ax_local);
	if (!ret) {
		PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);
		return 0;
	}

	platform_set_drvdata (pdev, NULL);
	free_netdev (ndev);
unmap_region:
	iounmap (addr);
release_region:
	release_mem_region (mem, AX88796B_IO_EXTENT);

	PRINTK (ERROR_MSG, PFX "not found (%d).\n", ret);
	PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);
	return ret;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_suspend
 * Purpose: Device suspend handling function
 * ----------------------------------------------------------------------------
 */
static int
ax88796b_suspend(struct platform_device *p_dev, pm_message_t state)
{
	struct net_device *ndev = dev_get_drvdata(&(p_dev)->dev);
	struct ax_device *ax_local = ax_get_priv (ndev);
	void *ax_base = ax_local->membase;
	unsigned long flags;

	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	if (!ndev || !netif_running (ndev)) {
		PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);
		return 0;
	}

	netif_device_detach (ndev);
	netif_stop_queue (ndev);

	spin_lock_irqsave (&ax_local->page_lock, flags);

	writeb (E8390_NODMA | E8390_PAGE0, ax_base + E8390_CMD);
	writeb (0x00, ax_base + EN0_IMR);

	if (ax_local->wol) {
		u8 pme = 0;

		if (ax_local->wol & WAKEUP_LSCWE) {
			pme |= ENWUCS_LSCWE;
			PRINTK (DEBUG_MSG, PFX 
				" Enable link change wakeup\n");
		}

		if (ax_local->wol & WAKEUP_MP) {
			pme |= ENWUCS_MPEN;
			PRINTK (DEBUG_MSG, PFX 
				" Enable magic packet wakeup\n");
		}

		writeb (E8390_NODMA | E8390_PAGE3, ax_base + E8390_CMD);
		writeb (pme, ax_base + EN3_WUCS);

		/* Enable D1 power saving */
		writeb (ENPMR_D1, ax_base + EN3_PMR);

		/* Set PME output type Push-Pull */
		writeb (E8390_NODMA | E8390_PAGE1, ax_base + E8390_CMD);
		writeb (ENBTCR_PME_PULL, ax_base + EN0_BTCR);
		writeb (E8390_NODMA | E8390_PAGE0, ax_base + E8390_CMD);
	} else {
		/* Enable D2 power saving */
		writeb (E8390_NODMA | E8390_PAGE3, ax_base + E8390_CMD);
		writeb (ENPMR_D2, ax_base + EN3_PMR);
		PRINTK (DEBUG_MSG, PFX " Enable D2 power saving mode\n");
	}

	spin_unlock_irqrestore (&ax_local->page_lock, flags);

	PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);
	return 0;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_resume
 * Purpose: Device resume handling function
 * ----------------------------------------------------------------------------
 */
static int
ax88796b_resume (struct platform_device *p_dev)
{
	struct net_device *ndev = dev_get_drvdata(&(p_dev)->dev);
	struct ax_device *ax_local = netdev_priv (ndev);
	void __iomem *ax_base = ax_local->membase;
	unsigned long flags;
	int ret;
	u8 pme;

	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	spin_lock_irqsave (&ax_local->page_lock, flags);

	if (ax_local->wol) {
		writeb (E8390_NODMA | E8390_PAGE3, ax_base + E8390_CMD);
		pme = readb (ax_base + EN3_WUCS);
		if (pme & ENWUCS_LSC) {
			PRINTK (DEBUG_MSG, PFX " Waked by link change\n");
		} else if (pme & ENWUCS_MPR) {
			PRINTK (DEBUG_MSG, PFX " Waked by magic packet\n");
		}
	} else {
		writeb (0xFF, ax_base + EN0_RESET);
		PRINTK (DEBUG_MSG, PFX " Host wakeup\n");
		mdelay (100);
	}

	netif_device_attach (ndev);

	ax88796b_reset (ndev);
	ret = ax88796b_init (ndev, 1);

	spin_unlock_irqrestore (&ax_local->page_lock, flags);

	if (ret == 0)
		netif_start_queue (ndev);

	PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);

	return 0;
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_exit_module
 * Purpose: Driver clean and exit
 * ----------------------------------------------------------------------------
 */
static int __devexit ax88796b_exit_module(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata (pdev);
	struct ax_device *ax_local = netdev_priv (ndev);
	void __iomem *ax_base = ax_local->membase;

	PRINTK (DEBUG_MSG, PFX " %s beginning ..........\n", __FUNCTION__);

	platform_set_drvdata (pdev, NULL);
	unregister_netdev (ndev);
	iounmap (ax_base);
	release_mem_region (mem, AX88796B_IO_EXTENT);
	free_netdev (ndev);

	PRINTK (DEBUG_MSG, PFX " %s end ..........\n", __FUNCTION__);

	return 1;
}

/*Fill platform driver information*/
static struct platform_driver ax88796b_driver = {
	.driver	= {
		.name    = "ax88796b",
		.owner	 = THIS_MODULE,
	},
	.probe   = ax88796b_drv_probe,
	.remove  = __devexit_p(ax88796b_exit_module),
	.suspend = ax88796b_suspend,
	.resume  = ax88796b_resume,
};

/*
 * ----------------------------------------------------------------------------
 * Function Name:
 * Purpose: Driver initialize
 * ----------------------------------------------------------------------------
 */
static int __init
ax88796b_init_mod(void)
{

	return platform_driver_register (&ax88796b_driver);
}

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796b_exit_module
 * Purpose: Platform driver unregister 
 * ----------------------------------------------------------------------------
 */
static void __exit
ax88796b_cleanup(void)
{

	/* Unregister platform driver*/
	platform_driver_unregister (&ax88796b_driver);
}

module_init(ax88796b_init_mod);
module_exit(ax88796b_cleanup);


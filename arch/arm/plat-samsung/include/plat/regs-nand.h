/* arch/arm/mach-s3c2410/include/mach/regs-nand.h
 *
 * Copyright (c) 2004-2005 Simtec Electronics <linux@simtec.co.uk>
 *	http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2410 NAND register definitions
*/

#ifndef __ASM_ARM_REGS_NAND
#define __ASM_ARM_REGS_NAND


#define S3C2410_NFREG(x) (x)

#define S3C2410_NFCONF  S3C2410_NFREG(0x00)
#define S3C2410_NFCMD   S3C2410_NFREG(0x04)
#define S3C2410_NFADDR  S3C2410_NFREG(0x08)
#define S3C2410_NFDATA  S3C2410_NFREG(0x0C)
#define S3C2410_NFSTAT  S3C2410_NFREG(0x10)
#define S3C2410_NFECC   S3C2410_NFREG(0x14)

#define S3C2440_NFCONT   S3C2410_NFREG(0x04)
#define S3C2440_NFCMD    S3C2410_NFREG(0x08)
#define S3C2440_NFADDR   S3C2410_NFREG(0x0C)
#define S3C2440_NFDATA   S3C2410_NFREG(0x10)
#define S3C2440_NFECCD0  S3C2410_NFREG(0x14)
#define S3C2440_NFECCD1  S3C2410_NFREG(0x18)
#define S3C2440_NFECCD   S3C2410_NFREG(0x1C)
#define S3C2440_NFSTAT   S3C2410_NFREG(0x20)
#define S3C2440_NFESTAT0 S3C2410_NFREG(0x24)
#define S3C2440_NFESTAT1 S3C2410_NFREG(0x28)
#define S3C2440_NFMECC0  S3C2410_NFREG(0x2C)
#define S3C2440_NFMECC1  S3C2410_NFREG(0x30)
#define S3C2440_NFSECC   S3C2410_NFREG(0x34)
#define S3C2440_NFSBLK   S3C2410_NFREG(0x38)
#define S3C2440_NFEBLK   S3C2410_NFREG(0x3C)

#define S3C2412_NFSBLK		S3C2410_NFREG(0x20)
#define S3C2412_NFEBLK		S3C2410_NFREG(0x24)
#define S3C2412_NFSTAT		S3C2410_NFREG(0x28)
#define S3C2412_NFMECC_ERR0	S3C2410_NFREG(0x2C)
#define S3C2412_NFMECC_ERR1	S3C2410_NFREG(0x30)
#define S3C2412_NFMECC0		S3C2410_NFREG(0x34)
#define S3C2412_NFMECC1		S3C2410_NFREG(0x38)
#define S3C2412_NFSECC		S3C2410_NFREG(0x3C)

#define S3C2410_NFCONF_EN          (1<<15)
#define S3C2410_NFCONF_512BYTE     (1<<14)
#define S3C2410_NFCONF_4STEP       (1<<13)
#define S3C2410_NFCONF_INITECC     (1<<12)
#define S3C2410_NFCONF_nFCE        (1<<11)
#define S3C2410_NFCONF_TACLS(x)    ((x)<<8)
#define S3C2410_NFCONF_TWRPH0(x)   ((x)<<4)
#define S3C2410_NFCONF_TWRPH1(x)   ((x)<<0)

#define S3C2410_NFSTAT_BUSY        (1<<0)

#define S3C2440_NFCONF_BUSWIDTH_8	(0<<0)
#define S3C2440_NFCONF_BUSWIDTH_16	(1<<0)
#define S3C2440_NFCONF_ADVFLASH		(1<<3)
#define S3C2440_NFCONF_TACLS(x)		((x)<<12)
#define S3C2440_NFCONF_TWRPH0(x)	((x)<<8)
#define S3C2440_NFCONF_TWRPH1(x)	((x)<<4)

#define S3C2440_NFCONT_LOCKTIGHT	(1<<13)
#define S3C2440_NFCONT_SOFTLOCK		(1<<12)
#define S3C2440_NFCONT_ILLEGALACC_EN	(1<<10)
#define S3C2440_NFCONT_RNBINT_EN	(1<<9)
#define S3C2440_NFCONT_RN_FALLING	(1<<8)
#define S3C2440_NFCONT_SPARE_ECCLOCK	(1<<6)
#define S3C2440_NFCONT_MAIN_ECCLOCK	(1<<5)
#define S3C2440_NFCONT_INITECC		(1<<4)
#define S3C2440_NFCONT_nFCE		(1<<1)
#define S3C2440_NFCONT_ENABLE		(1<<0)

#define S3C2440_NFSTAT_READY		(1<<0)
#define S3C2440_NFSTAT_nCE		(1<<1)
#define S3C2440_NFSTAT_RnB_CHANGE	(1<<2)
#define S3C2440_NFSTAT_ILLEGAL_ACCESS	(1<<3)

#define S3C2412_NFCONF_NANDBOOT		(1<<31)
#define S3C2412_NFCONF_ECCCLKCON	(1<<30)
#define S3C2412_NFCONF_ECC_MLC		(1<<24)
#define S3C2412_NFCONF_TACLS_MASK	(7<<12)	/* 1 extra bit of Tacls */

#define S3C2412_NFCONT_ECC4_DIRWR	(1<<18)
#define S3C2412_NFCONT_LOCKTIGHT	(1<<17)
#define S3C2412_NFCONT_SOFTLOCK		(1<<16)
#define S3C2412_NFCONT_ECC4_ENCINT	(1<<13)
#define S3C2412_NFCONT_ECC4_DECINT	(1<<12)
#define S3C2412_NFCONT_MAIN_ECC_LOCK	(1<<7)
#define S3C2412_NFCONT_INIT_MAIN_ECC	(1<<5)
#define S3C2412_NFCONT_nFCE1		(1<<2)
#define S3C2412_NFCONT_nFCE0		(1<<1)

#define S3C2412_NFSTAT_ECC_ENCDONE	(1<<7)
#define S3C2412_NFSTAT_ECC_DECDONE	(1<<6)
#define S3C2412_NFSTAT_ILLEGAL_ACCESS	(1<<5)
#define S3C2412_NFSTAT_RnB_CHANGE	(1<<4)
#define S3C2412_NFSTAT_nFCE1		(1<<3)
#define S3C2412_NFSTAT_nFCE0		(1<<2)
#define S3C2412_NFSTAT_Res1		(1<<1)
#define S3C2412_NFSTAT_READY		(1<<0)

#define S3C2412_NFECCERR_SERRDATA(x)	(((x) >> 21) & 0xf)
#define S3C2412_NFECCERR_SERRBIT(x)	(((x) >> 18) & 0x7)
#define S3C2412_NFECCERR_MERRDATA(x)	(((x) >> 7) & 0x3ff)
#define S3C2412_NFECCERR_MERRBIT(x)	(((x) >> 4) & 0x7)
#define S3C2412_NFECCERR_SPARE_ERR(x)	(((x) >> 2) & 0x3)
#define S3C2412_NFECCERR_MAIN_ERR(x)	(((x) >> 2) & 0x3)
#define S3C2412_NFECCERR_NONE		(0)
#define S3C2412_NFECCERR_1BIT		(1)
#define S3C2412_NFECCERR_MULTIBIT	(2)
#define S3C2412_NFECCERR_ECCAREA	(3)


/* for s3c_nand.c */
#define S5P_NFREG(x) (x)		// used ioremap

#define S3C_NFCONF				S5P_NFREG(0x00)
#define S3C_NFCONT				S5P_NFREG(0x04)
#define S3C_NFCMMD				S5P_NFREG(0x08)
#define S3C_NFADDR				S5P_NFREG(0x0c)
#define S3C_NFDATA8				S5P_NFREG(0x10)
#define S3C_NFDATA				S5P_NFREG(0x10)
#define S3C_NFMECCDATA0			S5P_NFREG(0x14)
#define S3C_NFMECCDATA1			S5P_NFREG(0x18)
#define S3C_NFSECCDATA			S5P_NFREG(0x1c)
#define S3C_NFSBLK				S5P_NFREG(0x20)
#define S3C_NFEBLK				S5P_NFREG(0x24)
#define S3C_NFSTAT				S5P_NFREG(0x28)
#define S3C_NFMECCERR0			S5P_NFREG(0x2c)
#define S3C_NFMECCERR1			S5P_NFREG(0x30)
#define S3C_NFMECC0				S5P_NFREG(0x34)
#define S3C_NFMECC1				S5P_NFREG(0x38)
#define S3C_NFSECC				S5P_NFREG(0x3c)
#define S3C_NFMLCBITPT			S5P_NFREG(0x40)
#define S3C_NF8ECCERR0          S5P_NFREG(0x44) 
#define S3C_NF8ECCERR1          S5P_NFREG(0x48)
#define S3C_NF8ECCERR2          S5P_NFREG(0x4C)           
#define S3C_NFM8ECC0            S5P_NFREG(0x50)         
#define S3C_NFM8ECC1            S5P_NFREG(0x54)        
#define S3C_NFM8ECC2            S5P_NFREG(0x58)       
#define S3C_NFM8ECC3            S5P_NFREG(0x5C)      
#define S3C_NFMLC8BITPT0        S5P_NFREG(0x60)     
#define S3C_NFMLC8BITPT1        S5P_NFREG(0x64)  

#define S3C_NFECCPRGECC0 		S5P_NFREG(0x20090)   // R ECC Parity Code0 Register for Page program 0x0000_0000
#define S3C_NFECCPRGECC1 		S5P_NFREG(0x20094)   // R ECC Parity Code1 Register for Page Program 0x0000_0000
#define S3C_NFECCPRGECC2 		S5P_NFREG(0x20098)   // R ECC parity code2 register for page program 0x0000_0000
#define S3C_NFECCPRGECC3 		S5P_NFREG(0x2009C)   // R ECC parity code3 register for page program 0x0000_0000
#define S3C_NFECCPRGECC4 		S5P_NFREG(0x200A0)   // R ECC parity code4 register for page program 0x0000_0000
#define S3C_NFECCPRGECC5 		S5P_NFREG(0x200A4)   // R ECC parity code5 register for page program 0x0000_0000
#define S3C_NFECCPRGECC6 		S5P_NFREG(0x200A8)   // R ECC parity code6 register for page program 0x0000_0000
#define S3C_NFECCCONF 			S5P_NFREG(0x20000)   // R/W ECC Configuration Register 0x0000_0000
#define S3C_NFECCCONT 			S5P_NFREG(0x20020)   // R/W ECC Control Register 0x0000_0000
#define S3C_NFECCSTAT 			S5P_NFREG(0x20030)   // R ECC Status Register 0x0000_0000
#define S3C_NFECCSECSTAT 		S5P_NFREG(0x20040)   // R ECC Sector Status Register 0x0000_0000

#define S3C_NFCONF_NANDBOOT		(1<<31)
#define S3C_NFCONF_ECCCLKCON	(1<<30)
#define S3C_NFCONF_ECC_MLC		(1<<24)
#define	S3C_NFCONF_ECC_1BIT		(0<<23)
#define	S3C_NFCONF_ECC_4BIT		(2<<23)
#define	S3C_NFCONF_ECC_8BIT		(1<<23)
#define S3C_NFCONF_TACLS(x)		((x)<<12)
#define S3C_NFCONF_TWRPH0(x)	((x)<<8)
#define S3C_NFCONF_TWRPH1(x)	((x)<<4)
#define S3C_NFCONF_ADVFLASH		(1<<3)
#define S3C_NFCONF_PAGESIZE		(1<<2)
#define S3C_NFCONF_ADDRCYCLE	(1<<1)
#define S3C_NFCONF_BUSWIDTH		(1<<0)

#define S3C_NFCONT_ECC_ENC		(1<<18)
#define S3C_NFCONT_LOCKTGHT		(1<<17)
#define S3C_NFCONT_LOCKSOFT		(1<<16)
#define S3C_NFCONT_MECCLOCK		(1<<7)
#define S3C_NFCONT_SECCLOCK		(1<<6)
#define S3C_NFCONT_INITMECC		(1<<5)
#define S3C_NFCONT_INITSECC		(1<<4)
#define S3C_NFCONT_nFCE1		(1<<2)
#define S3C_NFCONT_nFCE0		(1<<1)
#define S3C_NFCONT_INITECC		(S3C_NFCONT_INITSECC | S3C_NFCONT_INITMECC)

#define S3C_NFECCERR0_ECCBUSY	(1<<31)
#define S3C_NFECSTAT_ECCENCDONE	(1<<25)	// when 8bit ECC
#define S3C_NFECSTAT_ECCDECDONE	(1<<24)	// when 8bit ECC

#define S3C_NFSTAT_ECCENCDONE	(1<<7)
#define S3C_NFSTAT_ECCDECDONE	(1<<6)
#define S3C_NFSTAT_BUSY			(1<<0)
#define S3C_NFSTAT_ILEGL_ACC    (1<<7)
#define S3C_NFSTAT_RnB_CHANGE   (1<<6)
#define S3C_NFSTAT_nFCE1        (1<<3)
#define S3C_NFSTAT_nFCE0        (1<<2)
#define S3C_NFSTAT_Res1         (1<<1)
#define S3C_NFSTAT_READY        (1<<0)
#define S3C_NFSTAT_CLEAR        ((1<<9) |(1<<8) |(1<<7) |(1<<6))
#define S3C_NFSTAT_BUSY			(1<<0)


#endif /* __ASM_ARM_REGS_NAND */


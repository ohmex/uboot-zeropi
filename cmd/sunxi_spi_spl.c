/*
 * Copyright (C) 2016 Siarhei Siamashka <siarhei.siamashka@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

// ## hyphop ## 
// add sunxi spi read to uboot cmd
// x2 and x4 speed improve

#include <common.h>

#include <command.h>
#include <mapmem.h>

#include <spl.h>
#include <asm/gpio.h>
#include <asm/io.h>

#ifdef CONFIG_SPL_OS_BOOT
#error CONFIG_SPL_OS_BOOT is not supported yet
#endif

/*
 * This is a very simple U-Boot image loading implementation, trying to
 * replicate what the boot ROM is doing when loading the SPL. Because we
 * know the exact pins where the SPI Flash is connected and also know
 * that the Read Data Bytes (03h) command is supported, the hardware
 * configuration is very simple and we don't need the extra flexibility
 * of the SPI framework. Moreover, we rely on the default settings of
 * the SPI controler hardware registers and only adjust what needs to
 * be changed. This is good for the code size and this implementation
 * adds less than 400 bytes to the SPL.
 *
 * There are two variants of the SPI controller in Allwinner SoCs:
 * A10/A13/A20 (sun4i variant) and everything else (sun6i variant).
 * Both of them are supported.
 *
 * The pin mixing part is SoC specific and only A10/A13/A20/H3/A64 are
 * supported at the moment.
 */

/*****************************************************************************/
/* SUN4I variant of the SPI controller                                       */
/*****************************************************************************/

#define SUN4I_SPI0_CCTL             (0x01C05000 + 0x1C)
#define SUN4I_SPI0_CTL              (0x01C05000 + 0x08)
#define SUN4I_SPI0_RX               (0x01C05000 + 0x00)
#define SUN4I_SPI0_TX               (0x01C05000 + 0x04)
#define SUN4I_SPI0_FIFO_STA         (0x01C05000 + 0x28)
#define SUN4I_SPI0_BC               (0x01C05000 + 0x20)
#define SUN4I_SPI0_TC               (0x01C05000 + 0x24)

#define SUN4I_CTL_ENABLE            BIT(0)
#define SUN4I_CTL_MASTER            BIT(1)
#define SUN4I_CTL_TF_RST            BIT(8)
#define SUN4I_CTL_RF_RST            BIT(9)
#define SUN4I_CTL_XCH               BIT(10)

/*****************************************************************************/
/* SUN6I variant of the SPI controller                                       */
/*****************************************************************************/

#define SUN6I_SPI0_CCTL             (0x01C68000 + 0x24)
#define SUN6I_SPI0_GCR              (0x01C68000 + 0x04)
#define SUN6I_SPI0_TCR              (0x01C68000 + 0x08)
#define SUN6I_SPI0_FIFO_STA         (0x01C68000 + 0x1C)
#define SUN6I_SPI0_MBC              (0x01C68000 + 0x30)
#define SUN6I_SPI0_MTC              (0x01C68000 + 0x34)
#define SUN6I_SPI0_BCC              (0x01C68000 + 0x38)
#define SUN6I_SPI0_TXD              (0x01C68000 + 0x200)
#define SUN6I_SPI0_RXD              (0x01C68000 + 0x300)

#define SUN6I_CTL_ENABLE            BIT(0)
#define SUN6I_CTL_MASTER            BIT(1)
#define SUN6I_CTL_SRST              BIT(31)
#define SUN6I_TCR_XCH               BIT(31)

/*****************************************************************************/

#define CCM_AHB_GATING0             (0x01C20000 + 0x60)
#define CCM_SPI0_CLK                (0x01C20000 + 0xA0)
#define SUN6I_BUS_SOFT_RST_REG0     (0x01C20000 + 0x2C0)

#define AHB_RESET_SPI0_SHIFT        20
#define AHB_GATE_OFFSET_SPI0        20

#define SPI0_CLK_DIV_BY_1           0x0001
#define SPI0_CLK_DIV_BY_2           0x1000
#define SPI0_CLK_DIV_BY_4           0x1001

/*****************************************************************************/

/*
 * Allwinner A10/A20 SoCs were using pins PC0,PC1,PC2,PC23 for booting
 * from SPI Flash, everything else is using pins PC0,PC1,PC2,PC3.
 */
static void spi0_pinmux_setup(unsigned int pin_function)
{
	unsigned int pin;

	for (pin = SUNXI_GPC(0); pin <= SUNXI_GPC(2); pin++)
		sunxi_gpio_set_cfgpin(pin, pin_function);

	if (IS_ENABLED(CONFIG_MACH_SUN4I) || IS_ENABLED(CONFIG_MACH_SUN7I))
		sunxi_gpio_set_cfgpin(SUNXI_GPC(23), pin_function);
	else
		sunxi_gpio_set_cfgpin(SUNXI_GPC(3), pin_function);
}

/*
 * Setup 6 MHz from OSC24M (because the BROM is doing the same).
 */
static void spi0_enable_clock(void)
{
	/* Deassert SPI0 reset on SUN6I */
	if (IS_ENABLED(CONFIG_SUNXI_GEN_SUN6I))
		setbits_le32(SUN6I_BUS_SOFT_RST_REG0,
			     (1 << AHB_RESET_SPI0_SHIFT));

	/* Open the SPI0 gate */
	setbits_le32(CCM_AHB_GATING0, (1 << AHB_GATE_OFFSET_SPI0));

	/* Divide by 4 */ 
	//writel(SPI0_CLK_DIV_BY_4, IS_ENABLED(CONFIG_SUNXI_GEN_SUN6I) ?
	//SPI readed 1000000 bytes from 0 offset, time 1.615
	/* Divide by 2 */ 
	//SPI readed 1000000 bytes from 0 offset, time 0.897
	//writel(SPI0_CLK_DIV_BY_2, IS_ENABLED(CONFIG_SUNXI_GEN_SUN6I) ?
	/* Divide by 1 */ 
	//SPI readed 1000000 bytes from 0 offset, time 0.535
	writel(SPI0_CLK_DIV_BY_1, IS_ENABLED(CONFIG_SUNXI_GEN_SUN6I) ?
				  SUN6I_SPI0_CCTL : SUN4I_SPI0_CCTL);
	/* 24MHz from OSC24M */
	writel((1 << 31), CCM_SPI0_CLK);

	if (IS_ENABLED(CONFIG_SUNXI_GEN_SUN6I)) {
		/* Enable SPI in the master mode and do a soft reset */
		setbits_le32(SUN6I_SPI0_GCR, SUN6I_CTL_MASTER |
					     SUN6I_CTL_ENABLE |
					     SUN6I_CTL_SRST);
		/* Wait for completion */
		while (readl(SUN6I_SPI0_GCR) & SUN6I_CTL_SRST)
			;
	} else {
		/* Enable SPI in the master mode and reset FIFO */
		setbits_le32(SUN4I_SPI0_CTL, SUN4I_CTL_MASTER |
					     SUN4I_CTL_ENABLE |
					     SUN4I_CTL_TF_RST |
					     SUN4I_CTL_RF_RST);
	}
}

static void spi0_disable_clock(void)
{
	/* Disable the SPI0 controller */
	if (IS_ENABLED(CONFIG_SUNXI_GEN_SUN6I))
		clrbits_le32(SUN6I_SPI0_GCR, SUN6I_CTL_MASTER |
					     SUN6I_CTL_ENABLE);
	else
		clrbits_le32(SUN4I_SPI0_CTL, SUN4I_CTL_MASTER |
					     SUN4I_CTL_ENABLE);

	/* Disable the SPI0 clock */
	writel(0, CCM_SPI0_CLK);

	/* Close the SPI0 gate */
	clrbits_le32(CCM_AHB_GATING0, (1 << AHB_GATE_OFFSET_SPI0));

	/* Assert SPI0 reset on SUN6I */
	if (IS_ENABLED(CONFIG_SUNXI_GEN_SUN6I))
		clrbits_le32(SUN6I_BUS_SOFT_RST_REG0,
			     (1 << AHB_RESET_SPI0_SHIFT));
}

static void spi0_init(void)
{
	unsigned int pin_function = SUNXI_GPC_SPI0;

	if (IS_ENABLED(CONFIG_MACH_SUN50I))
		pin_function = SUN50I_GPC_SPI0;

	spi0_pinmux_setup(pin_function);
	spi0_enable_clock();
}

static void spi0_deinit(void)
{
	/* New SoCs can disable pins, older could only set them as input */
	unsigned int pin_function = SUNXI_GPIO_INPUT;
	if (IS_ENABLED(CONFIG_SUNXI_GEN_SUN6I))
		pin_function = SUNXI_GPIO_DISABLE;

	spi0_disable_clock();
	spi0_pinmux_setup(pin_function);
}

/*****************************************************************************/

#define SPI_READ_MAX_SIZE 60 /* FIFO size, minus 4 bytes of the header */

static void sunxi_spi0_read_data(u8 *buf, u32 addr, u32 bufsize,
				 u32 spi_ctl_reg,
				 u32 spi_ctl_xch_bitmask,
				 u32 spi_fifo_reg,
				 u32 spi_tx_reg,
				 u32 spi_rx_reg,
				 u32 spi_bc_reg,
				 u32 spi_tc_reg,
				 u32 spi_bcc_reg)
{
	writel(4 + bufsize, spi_bc_reg); /* Burst counter (total bytes) */
	writel(4, spi_tc_reg);           /* Transfer counter (bytes to send) */
	if (spi_bcc_reg)
		writel(4, spi_bcc_reg);  /* SUN6I also needs this */

	/* Send the Read Data Bytes (03h) command header */
	writeb(0x03, spi_tx_reg);
	writeb((u8)(addr >> 16), spi_tx_reg);
	writeb((u8)(addr >> 8), spi_tx_reg);
	writeb((u8)(addr), spi_tx_reg);

	/* Start the data transfer */
	setbits_le32(spi_ctl_reg, spi_ctl_xch_bitmask);

	/* Wait until everything is received in the RX FIFO */
	while ((readl(spi_fifo_reg) & 0x7F) < 4 + bufsize)
		;

	/* Skip 4 bytes */
	readl(spi_rx_reg);

	/* Read the data */
	while (bufsize-- > 0)
		*buf++ = readb(spi_rx_reg);

	/* tSHSL time is up to 100 ns in various SPI flash datasheets */
	udelay(1);
}

static void spi0_read_data(void *buf, u32 addr, u32 len)
{
	u8 *buf8 = buf;
	u32 chunk_len;

	while (len > 0) {
		chunk_len = len;
		if (chunk_len > SPI_READ_MAX_SIZE)
			chunk_len = SPI_READ_MAX_SIZE;

		if (IS_ENABLED(CONFIG_SUNXI_GEN_SUN6I)) {
			sunxi_spi0_read_data(buf8, addr, chunk_len,
					     SUN6I_SPI0_TCR,
					     SUN6I_TCR_XCH,
					     SUN6I_SPI0_FIFO_STA,
					     SUN6I_SPI0_TXD,
					     SUN6I_SPI0_RXD,
					     SUN6I_SPI0_MBC,
					     SUN6I_SPI0_MTC,
					     SUN6I_SPI0_BCC);
		} else {
			sunxi_spi0_read_data(buf8, addr, chunk_len,
					     SUN4I_SPI0_CTL,
					     SUN4I_CTL_XCH,
					     SUN4I_SPI0_FIFO_STA,
					     SUN4I_SPI0_TX,
					     SUN4I_SPI0_RX,
					     SUN4I_SPI0_BC,
					     SUN4I_SPI0_TC,
					     0);
		}

		len  -= chunk_len;
		buf8 += chunk_len;
		addr += chunk_len;
	}
}

/*****************************************************************************/

static int spl_spi_load_image2(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned int bytes  = 0;
	unsigned int offset = 0;

	unsigned long dst;
	unsigned char db  = 10;
	unsigned char db2 = 10;
	unsigned char l   = strlen(argv[0]);
	//0123456789
	//spiread.xx

	
	if ( l > 8 )
	switch (argv[0][8]) {
	    case 104: //h hex
	    db2=db=16;
	    break;
	    case 120: //x hex
	    db2=db=16;
	    break;
	    case 100: //d dec
	    db2=db=10;
	    break;
	}
	if ( l > 9 )
	switch (argv[0][9]) {
	    case 104: //h hex
	    db2=16;
	    break;
	    case 120: //x hex
	    db2=16;
	    break;
	    case 100: //d dec
	    db2=10;
	    break;
	}

	//printf("cmd %d %s %d\n", argv[0][9], &argv[0][9], l );

	switch (argc) {
	case 4:
		dst = simple_strtoul(argv[1], NULL, 16);
		offset = simple_strtoul(argv[2], NULL, db);
		bytes = simple_strtoul(argv[3], NULL, db2);
		break;
	default:
		return CMD_RET_USAGE;
	}

	ulong msecs = get_timer(0);

	spi0_init();

	//spi0_read_data((void *)addr, offset, size);
	//spi0_read_data( map_sysmem(dst, size),
	spi0_read_data( map_sysmem(dst, bytes),  offset, bytes);

	msecs = get_timer(msecs);

	printf("SPI readed %d bytes from %d offset, time %ld.%03d\n", 
	    bytes,
	    offset,
  	    msecs / 1000, (int)(msecs % 1000)
        );

	spi0_deinit();
	return 0;
}

U_BOOT_CMD(
	spiread,    4,    1,  spl_spi_load_image2,
	"hd HEX_DST HEX_OFFSET DEC_BYTES",
	"read from sunxi spi flash\n"
	"spiread.h  - hex_dst hex_offset hex_bytes\n"
	"spiread.d  - hex_dst dec_offset dec_bytes\n"
	"spiread.dh - hex_dst dec_offset hex_bytes\n"
	"spiread.hd - hex_dst hex_offset dec_bytes\n"
);


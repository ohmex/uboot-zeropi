/*
 * (C) Copyright 2013 Patrice Bouchand <pbfwdlist_gmail_com>
 * lzma uncompress command in Uboot
 *
 * made from existing cmd_unzip.c file of Uboot
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

// ## hyphop ##
// add more uncompress methods gzip lzo

#include <common.h>
#include <command.h>
#include <mapmem.h>
#include <asm/io.h>
#include <linux/lzo.h>

#include <lzma/LzmaTools.h>
#include <lzma/LzmaTools.h>

static int do_lzmadec(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned long src, dst;
	SizeT src_len = ~0UL, dst_len = ~0UL;
	int ret;

	switch (argc) {
	case 4:
		dst_len = simple_strtoul(argv[3], NULL, 10);
		/* fall through */
	case 3:
		src = simple_strtoul(argv[1], NULL, 16);
		dst = simple_strtoul(argv[2], NULL, 16);
		break;
	default:
		return CMD_RET_USAGE;
	}

	ret = lzmaBuffToBuffDecompress(map_sysmem(dst, dst_len), &src_len,
				       map_sysmem(src, 0), dst_len);

	printf("Uncompressed size: %ld = %#lX, ret: %d\n", (ulong)src_len,
	       (ulong)src_len, ret );
	setenv_hex("filesize", src_len);

	if (ret != SZ_OK)
		return 1;

	return 0;
}

static int do_lzodec(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned long src, dst;
	SizeT src_len = ~0UL, dst_len = ~0UL;
	//size_t src_len = 0;
	//ulong  src_len = 0;
	//ulong  dst_len = 0;
	int ret;

	switch (argc) {
	case 4:
		dst_len = simple_strtoul(argv[3], NULL, 10);
		/* fall through */
	case 3:
		src = simple_strtoul(argv[1], NULL, 16);
		dst = simple_strtoul(argv[2], NULL, 16);
		break;
	default:
		return CMD_RET_USAGE;
	}

	//size_t size = 0;
	ret = 
	lzop_decompress(map_sysmem(src, 0), dst_len,
				 map_sysmem(dst, dst_len), 
				//&size );
				&src_len );
	//src_len = size;

	printf("Uncompressed size: %ld = %#lX, ret: %d\n", (ulong)src_len,
	       (ulong)src_len, ret );
	setenv_hex("filesize", src_len);

//	need check ok status
//	if (ret != SZ_OK)
//	if ( src_len < 1 ) 
	if ( ret ) 
		return 1;

	return 0;
}


static int do_gzdec(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned long src, dst;
	ulong src_len = 0 , dst_len = 0;
	int ret;

	switch (argc) {
	case 5:
		src_len = simple_strtoul(argv[4], NULL, 10);
	case 4:
		dst_len = simple_strtoul(argv[3], NULL, 10);
		/* fall through */
	case 3:
		src = simple_strtoul(argv[1], NULL, 16);
		dst = simple_strtoul(argv[2], NULL, 16);
		break;
	default:
		return CMD_RET_USAGE;
	}


//int gunzip(void *dst, int dstlen, unsigned char *src, unsigned long *lenp)
//{
//	int i, flags;
//
//	/* skip header */
//	i = 10;
//	flags = src[3];
//	if (src[2] != DEFLATED || (flags & RESERVED) != 0) {
//		puts ("Error: Bad gzipped data\n");
//		return (-1);
//	}
//	if ((flags & EXTRA_FIELD) != 0)
//		i = 12 + src[10] + (src[11] << 8);
//	if ((flags & ORIG_NAME) != 0)
//		while (src[i++] != 0)
//			;
//	if ((flags & COMMENT) != 0)
//		while (src[i++] != 0)
//			;
//	if ((flags & HEAD_CRC) != 0)
//		i += 2;
//	if (i >= *lenp) {
//		puts ("Error: gunzip out of data in header\n");
//		return (-1);
//	}
//
//	return zunzip(dst, dstlen, src, lenp, 1, i);
//}

	ret = 
	gunzip( 
	    map_sysmem(dst, dst_len),
	    dst_len,
	    map_sysmem(src, 0),
	    &src_len 
	);

	printf("Uncompressed size: %ld = %#lX, ret: %d\n", (ulong)src_len,
	       (ulong)src_len, ret );
	setenv_hex("filesize", src_len);

	if ( ret ) 
		return 1;

	return 0;
}

U_BOOT_CMD(
	lzmadec,    4,    1,    do_lzmadec,
	"lzma uncompress a memory region",
	"srcaddr dstaddr [dstsize]"
);

U_BOOT_CMD(
	lzodec,    4,    1,    do_lzodec,
	"lzo uncompress a memory region",
	"srcaddr dstaddr dstsize|srcsize"
);

U_BOOT_CMD(
	gzdec,    5,    1,    do_gzdec,
	"gzip uncompress a memory region",
	"srcaddr dstaddr dstsize srcsize"
);


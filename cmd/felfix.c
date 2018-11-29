
// ## hyphop ##

#include <common.h>
#include <command.h>


static unsigned char fel_boot_data[]  = {
0x06, 0x00, 0x00, 0xEA, 0x65, 0x47, 0x4F, 0x4E, 0x2E, 0x42, 
0x54, 0x30, 0x97, 0x0E, 0x6E, 0x27, 0x00, 0x20, 0x00, 0x00, 
0x53, 0x50, 0x4C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x02, 0x00, 0x00, 0xEA, 0x00, 0xF0, 0x20, 0xE3, 
0x00, 0xF0, 0x20, 0xE3, 0x00, 0xF0, 0x20, 0xE3, 0x10, 0x0F, 
0x11, 0xEE, 0x02, 0x0A, 0x10, 0xE3, 0x20, 0xE0, 0xA0, 0x03, 
0x00, 0xE0, 0x1F, 0x15, 0x1E, 0xFF, 0x2F, 0xE1, 0x20, 0x00, 
0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define FEL_HDR_LEN 80
#define FEL_LEN  8192

static int do_fel(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	void *buf;
	buf = 0;

//	unsigned long dst;
//	dst = simple_strtoul("0x00000000", NULL, 16);
//	buf = map_sysmem(dst, FEL_LEN);

	//8192 / 512 = 16 = hex 10

        unsigned long mode = 10;
	memset( buf, 0, FEL_LEN );

//	unmap_sysmem(buf);

	switch (argc) {
		case 2:
			mode = simple_strtoul(argv[1], NULL, 10);
			break;
		default:
			return CMD_RET_USAGE;
	}
	
	switch (mode) {
		case 1:
		    memcpy( buf, fel_boot_data, FEL_HDR_LEN );
		    break;
		case 2:
		    // 16 * 512 = 8192
		    //  0123		
		    // 0123		
		    // 			
		    run_command("mmc read 0 40 10", 0);
		    break;		
		case 3:			
		    break;		
		case 4:			
		    printf("make SPL header packup to last block\n");
		    //run_command_list("\n\n\n", -1, 0);
		    run_command("mmc read 0 40 10", 0);
		    run_command("md.b 0 40", 0);
		    run_command("mmc read 0 10 10", 0);
		    run_command("md.b 0 40", 0);
		    run_command("echo mmc write 0 40 10", 0);
		    return 0;
		default:		
			return CMD_RET_USAGE;
	}				
	run_command("mmc write 0 10 10", 0);
	printf("FEL mode is %ld, please do reset cmd now\n", mode );
	
	return 0;
}


static int do_felrun(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

    //return_to_fel(fel_stash.sp, fel_stash.lr);
    printf("not realized now (\n" );

    return 0;

};

U_BOOT_CMD(
	fel,	2,	1,	do_fel,
	"FEL mode - for mmc boot loader",
"	1 - on    - write FEL mode loader to mmc\n"
"	2 - off   - try to restore normal uboot loader on mmc\n"
"	3 - clear - remove any boot loader from mmc\n"
"	4 - backup- help save boot loader to SPL last block on mmc\n"
);

U_BOOT_CMD(
	felrun,	1,	1,	do_felrun,
	"return/go to FEL mode",
	""
);

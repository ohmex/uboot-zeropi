//#define CONFIG_BOOTCOMMAND2 "run mizycmd; run distro_bootcmd; run bootfail_cmd;"
#define CONFIG_BOOTCOMMAND2 "run distro_bootcmd; run bootfail_cmd;"
#define CONFIG_PREBOOT_MIZY "run mizycmd"

#define CONFIG_ENV_MIZY	"mizycmd=" \
    "setenv bootdelay 10;" \
    "setenv bootfail_cmd \"echo boot failed\";" \
    "setenv mmc_block_boot 't=${target}; setenv target MMC${devnum}; mmc read ${scriptaddr} 410 10 && source ${scriptaddr}; setenv target $t';" \
    "setenv mmc_boot 'if mmc dev ${devnum}; then setenv devtype mmc; run mmc_block_boot; run scan_dev_for_boot_part; fi';" \
    "sm=0x4A0593D8;" \
    "if i2c dev 0 && i2c probe 3c;" \
    "then;" \
    "gpio set 6;" \
    "a=0x43110;" \
    "cp.b $sm ${a}000 4096;" \
    "i2c write ${a}000 3c 0 1A -s;" \
    "i2c write ${a}01A 3c 0 7 -s;" \
    "i2c write ${a}021 3c 0 401 -s;" \
    "i2c write ${a}422 3c 0 401 -s;" \
    "i2c write ${a}823 3c 0 8 -s;" \
    "i2c write ${a}82B 3c 0 2 -s;" \
    "echo \"Xpress Splash\";" \
    "fi" \
    "                                               " \
    "                                               " \
    "                                               " \
    "                                          ;true\0"


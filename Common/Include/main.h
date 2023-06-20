/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2008, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
* 
* Filename: main.h
*
* Author   : Winston <winstonv@amiindia.co.in>
*
******************************************************************/

#ifndef __MAIN_H__
#define __MAIN_H__
#include "projdef.h"
#ifndef MSDOS
#include "icc_what.h"
#endif

#include "version.h"
//#define CONFIG_SPX_FEATURE_YAFUFLASH_ENABLE_INTERACTIVE_UPGRADE
#ifdef MSDOS
#define    NETWORK_MEDIUM               0x01
#define    KCS_MEDIUM                   0x03
#define    USB_MEDIUM                   0x06
#if 0
#define    SERIAL_MEDIUM                0x02
#endif
#endif
#if defined (__x86_64__) || defined (WIN64)
extern unsigned int NewImageSize;
#else
extern unsigned long NewImageSize;
#endif
extern int vdbg;
extern int non_interactive;
int SplitFlashOffset;
char *FlashModName[8];
int ActivateOnly;
int ModuleType;
extern int Blockselect;

#ifdef  ICC_OS_LINUX
#define PACKED __attribute__ ((packed))
#else 
#define PACKED
#pragma pack(1)
#endif

typedef struct environment_s {
    unsigned int crc;    /* CRC32 over data bytes    */
    unsigned int size;   /* Size of environment */
    unsigned char *data;
} env_t;

#define MAX_MODULE_NAME 64

#ifdef ICC_OS_LINUX
    /* for linux and PPC implementations */
    #define spstrcasecmp    strcasecmp
    #define spstrncasecmp   strncasecmp
    #define spsnprintf      snprintf
#else
    /* for Windows implementations */
    #define spstrcasecmp    _stricmp
    #define spstrncasecmp   _strnicmp
    #define spsnprintf      _snprintf
    #define getch           _getch
    #define strdup          _strdup
    #define putch           _putch
    #define kbhit           _kbhit
#endif

#ifdef WINDOWS
typedef void*                           HANDLE;
extern HANDLE m_hD;
#endif

#define BMC_FLASH        0x0001
#define BIOS_FLASH       0x0002
#define CPLD_FLASH      0x0004
#define MMC_FLASH       0x0008
#define ME_FLASH        0x0010

#define STR_CONF_SEC_NAME "/YAFU_DEFAULTS"

#define AUTO_INACTIVE_IMAGE     0x0
#define IMAGE_1                 0x1
#define IMAGE_2                 0x2
#define IMAGE_BOTH              0x3

#define FLASH_BOTH_IMAGES       0x02


#define MAX_FILE_NAME_SIZE                    255
#define MAX_PASSWORD_SIZE                   21
#define MAX_USERNAME_SIZE                    17
#define STR_FILE_SIZE_INVALID                   "The File name has more than 256 characters\n"
#define STR_ERR_INVALID_PARAMETERS      "Invalid Parameters.\n"
#define STR_FILE_NAME_NOT_GIVEN                 "\nFile Name not given by user\n"
#define STR_PRESERVE_ITERACTIVE_OPTION   " '-preserve-XXX' option should not be used with 'preserve-config' Option \n\n"
#define STR_PRESERVE_QUIET_OPTION        " '-ignore-existing-overrides' option should only be used with 'preserve-XXX' Option \n\n"
#define STR_FULLUPGRADE_OPTION          " \n'-preserve-config' or 'force-boot' option should not be used with '-info' Option \n"
#define STR_INTERACTIVE_OPTION          " \n'-preserve-config' or '-force-boot' option should not be used with '-i' Option \n"
#define STR_INVALIDUPGRADE_OPTION       " \nPlease use a valid Option to Upgrade \n "
#define STR_ENTER_VALID_OPTION          "Enter a valid option \n"
#define STR_MEDIUM_OPTION                       "\n Only one medium should be used at a time \n"
#define STR_NOMEDIUM_OPTION                  "\n Enter the medium like -nw or -cd or -kcs option \n"
#define STR_ENTER_IP_HOST                               "\nPlease Enter the IP/HostName of the device \n"
#if 0
#define STR_ENTER_TERM                               "\nPlease Enter the serial terminal configuration of the device with '-term' and '-baudrate' options \n"
#endif
#define STR_USB_KCS_NOOPTION_IP                 "\nIf  USB or KCS or Serial medium is used, -ip should not be used.\n"
#define STR_USB_KCS_NOOPTION_HOST                 "\nIf  USB or KCS or Serial medium is used, -host should not be used.\n"
#define STR_INVALID_USERPASS_OPTION "\n '-u' & '-p' option should be used with '-nw' option only \n"
#define STR_USERPASSCONFLICT_OPTION "\n Both '-u' & '-p' option should be used together with '-nw' option \n "
#define STR_VERSION_VALID_OPTION        "\n -V option should not be given with other options.\n"
#define STR_VERSION_INVALID_E_OPTION        "\n -e option should not be given with other options.\n"
#define STR_ENTER_HOST_OR_IP            "\nInvalid Option. Enter IP address or Hostname.Should not give both the options.\n"
#define STR_ERR_OPTION_ARGUMENT         "Option requires parameter. Please Enter the valid parameter value.\n"
#define STR_INVALID_VYES_OPTION         "\n '-non-interactive' cannot be used along with 'ignore-diff-image', 'ignore-same-image', '-ignore-module-location' & '-ignore-boot-version' options.\n"
#define STR_NW_OPTION "\nInvalid Option.'-port' option should be used along with '-nw'option \n"
#define STR_INVALID_IMAGE_SELECTOR_OPTION "\n'-img-select' option should not be used with '-ignore-reselect-image' Option \n" 
#define STR_INVALID_ACTIVATE_OPTION      "\n '-activate' should not be used with '-no-reboot' option \n"
#define MAX_UBOOT_VAR_COUNT             64
#define MAX_UBOOT_VAR_LENGTH            320
#define MAX_UBOOT_VAR_NAME_LENGTH       32
#define MAX_SIZE_TO_READ                0x4000
#define RECIEVE_TIME_OUT                5000
#define MAX_FILE_LEN                    20
#define MAX_BKUP_LEN                    5
#define MAX_INPUT_LEN                   20
#define IP6_ADDR_LEN                    16
#define RETRYCOUNT (4)
#define MAX_SIZE_TO_READ_MMC  0x4000
#define FWUPG_IMAGE_SIGNATURE "PICMGFWU"
#define FWUPG_MD5_SIGNATURE_LENGTH     16
#define MAX_COMPONENT 8

#define BOOT_COMPONENT 0
#define APP_COMPONENT 1
#define MMC_COMPONENT 3
#define SKIP_OEM_DATA 16

#define FWUPG_HEADER_SIGNATURE_LENGTH 8
#define FWUPG_COMP_REVISION_LENGTH    2
#define FWUPG_DESCRIPTION_LENGTH 21
#define FWUPG_IMAGE_HEADER_VERSION    0 
#define FWUPG_FIRMWARE_SIZE_LENGTH 4
#define FWUPG_COMP_VERSION_LENGTH 6


typedef struct
{
    int dual_image_support;
    int common_conf;
    int full_firmware_upgrade_wt_version_cmp;
    int fwupdate_section_based_flash;
    int do_not_upgrade_both_firmware_on_uboot_mismatch;
    int signed_hashed_image_support;
    int online_flashing_support;
    int preserve_config;
    int extendedlog_support;    
    int mmc_boot_support;
}FEATURE_LIST;


typedef struct
{
     char   FileName[MAX_FILE_NAME_SIZE];
     char   MMCImgName[MAX_FILE_NAME_SIZE];	 
     char   Password[MAX_PASSWORD_SIZE];
     char   Username[MAX_USERNAME_SIZE];
     char   ModulesToUpgrade[15][15];
     char   ModUpgradeStatus;
     int PasswordExist;
     int UserNameExist;
     int ModuleUpgrade;
     int BootPreserve;
     int ConfigPreserve;
     int ConfigPreserveBitMask;
     int IgnoreExisting;
     int  Interactive;
     int  Full;
     int Auto;
     int Info;
     int UsbMedium;
     int NwMedium;
     int KcsMedium;
     int SerialMedium;
     int IPEntered;     
     int SerialTermEntered;
     int SerialBaudEntered;
     unsigned long BaudRate;
     int HostEntered;
     int SkipFMHCheck;
     int SkipCRCValidation;
     int RebootFirmware;
     int InteractiveFull;
     int PlatformCheck;
     int DiffImage;
     int SameImage;
     int ModLocation;
     int BootVersion;
     int Portentered;
     int Portnum;
     int ReselectImage;
     int NotPreserveCfg;
     int ImgInfo;
     int SignKey;
     int Silent;
     char SignKeyFile[MAX_FILE_NAME_SIZE];
     int ImgSelect; 
     int SplitImg;
     int FlashModCnt;
     int FlashModImg;
     int ExtlogPreserve;
     int versioncmpflash;
     unsigned char ActivateNode;
     int DoMMCImgUpdate; /*rom.ima*/
     int DoSPIImgUpdate;	/*mmc.ima*/
     int UpdateSPIImage; /*rom_component.ima*/
     int UpdateMMCImage; /*mmc_component.ima*/	 
} UPDATE_INFO;

#define ERR_INVCREDENTIAL  "\nAuthentication Failed.\n"
#define ERR_INV_ROLE  "\nInvalid Role of user.\nUser should have correct privilege to access the tool\n"

//#define VERSION_NUMBER          "2.32"


typedef struct
{
    char*           String;
    unsigned short  ErrNo;
} ERRNO_STR;

typedef struct 
{
    char Config_Name[MAX_FILE_LEN];
    int Status;
} YafuPreserveConf_T;

typedef struct
{
   unsigned char picmgId;
   unsigned char Version;	
   unsigned char UpgImgCapablities;
   unsigned char upgradeTimeout;
   unsigned char selftestTimeout;
   unsigned char rollbackTimeout;
   unsigned char inaccessTimeout;
} PACKED GetTargetCapabilities_T;

typedef struct 
{
	 unsigned char	FWRev1;
	 unsigned char	FWRev2;
	unsigned int	AuxillaryFWRevInfo;

}PACKED FWVersion_T;

typedef struct 
{
     unsigned char   UpgradeActionType;              /* Upload or Prepare */
     unsigned char   Components;                     /* Upgradeable components */
     unsigned char   Checksum;                       /* Zero checksum */
	
}PACKED  UpgAction_Hdr_T;

typedef struct
{
   unsigned int   imageSize;
   unsigned char* pImageData;
   unsigned char  componentId;
  GetTargetCapabilities_T GetTargetcap;
}  PACKED GetImgBufInfo_T;

typedef struct 
{
   unsigned char version[FWUPG_COMP_VERSION_LENGTH];
   char          desc[FWUPG_DESCRIPTION_LENGTH];
   unsigned char length[FWUPG_FIRMWARE_SIZE_LENGTH];
} PACKED UpgActRecDes_T;

typedef struct 
{
     unsigned char               Signature[8];  /* "PICMGFWU" by HPM-1 Spec */
     unsigned char               ImgFormatVer;
     unsigned char               DeviceID;
     unsigned char               ManufacturerID[3];
     unsigned char               ProductID[2];
     unsigned int                 Time;
     unsigned char               UpgImgCapablities;
     unsigned char               Components;             
     unsigned char               SelfTestTimeout;
     unsigned char               Rollbackimeout;        
     unsigned char               InaccessiblityTimeout;
     unsigned char               EarliestCompatibleVer[2];
     FWVersion_T               FWRevision;
     unsigned short               OEMDataLength;
     unsigned char               Checksum;               /* Zero Checksum */
    
} PACKED ImageHeaderRcrd_T;

#ifndef __GNU__
#pragma pack()
#endif

typedef struct
{
    UpgAction_Hdr_T  UpgActionHdr;
    FWVersion_T   FWVer;              /* Firmware version */
    unsigned char   FWDescStr[21];                  /* Firmware description */
    unsigned int  FWlength;                       /* Firmware length */
    
} UpgActRec_T;


#define STR_HELP        "\
Usage: Yafuflash [OPTION] [FW_IMAGE_FILE] \n \
Perform BMC Flash Update \n \
-?                                            Displays the utility usage \n \
-h                                            Displays the utility usage \n \
-V                                            Displays the version of the tool \n \
-e                                            List outs a few examples of the tool \n\
OPTION : \n \
-info                                         Displays information about current FW and new FW.\n \
-msi,-img-section-info                        Displays information about current FW Sections.\n \
-mi,-img-info                                 Displays information about current FW Versions.\n \
-fb,-force-boot                               Option to FORCE BootLoader upgrade during full upgrade.\n \
                                              Also, skips user interaction in Interactive Upgrade mode.\n \
                                              This option is not allowed with interactive upgrade option\n \
-bu,-block-upgrade                            Option to Flash using Block by Block method \n \
-netfn 0xXX                                   Option to Flash using OEM specific Netfuncion\n"



#define STR_HELP_MORE                                " \
-pc,-preserve-config                          Option to preserve Config Module during full upgrade.\n \
                                              If platform supports Dual Image, this option skips user\n \
                                              interaction, preserves config and continues update process.\n \
                                              This option is not allowed with interactive upgrade option.\n \
-ipc,-ignore-platform-check                   If this image is for a different platform, this option skips\n \
                                              user interaction and continues update process.\n \
-idi,-ignore-diff-image                       If this image differs from the one currently programmed, this\n \
                                              option skips user interaction and continues update process.\n \
-isi,-ignore-same-image                       If this image is same as the one currently programmed, this\n \
                                              option skips user interaction and continues update process.\n"


#define STR_HELP_MORE1                     " \
-iml,-ignore-module-location                  If module(s) of this image is/are in a different location, this\n \
                                              option skips user interaction and continues update process.\n \
-ibv,-ignore-boot-version                     If bootloader version is different and -force-boot is not specified,\n \
                                              this option skips user interaction and continues update process.\n \
                                              The bootloader will be updated.\n \
-iri,-ignore-reselect-image                   This option skips reselecting the active image.\n \
-inc,-ignore-non-preserve-config              If the Images of both flash share the same Configuration area.\n \
                                              Not preserving will restore to default factory settings, this option skips it.\n \
-msp,-split-img                               Use this option to flash split image.\n \
-f-XXX,-flash-XXX                             Use this option to flash spection section where XXX denotes name of the section,\n \
                                              example -flash-conf. If it is split image need to give -split-img along with this option.\n \
-q,-quiet                                     Use the option to show the minimum flash progress details.\n"


#define STR_HELP6   " \
-pe,-preserve-extlog                          Option to preserve extlog configuration during firmware flash\n"

#ifdef CONFIG_SPX_FEATURE_YAFUFLASH_ENABLE_INTERACTIVE_UPGRADE
#define STR_HELP1   " \
-i                                            Option to interactive upgrade (upgrade only required Modules)\n \
-f,-full                                      Performs full firmware upgrade with Interactive Upgrade mode. Skips option to select individual module upgrade.\n \
                                              This option must be used along with -i (-interactive) option.\n"
#else
#define STR_HELP1  ""
#endif

#ifdef  CONFIG_SPX_FEATURE_SD_EMMC_SUPPORT
#define STR_HELP8 " \
-sd                                           Option to update image in SD Card\n"
#endif

#define STR_HELP3   " \
-sc,-skip-crc                                 Option to skip the CRC check(Only for Dual Image Support)\n \
-sf,-skip-fmh                                 Option to skip the FMH check(Only for Dual Image Support)\n \
-d                                            Option to specify the peripheral(Only for Dual Image Support)\n\
                                                   <BIT0> - BMC\n\
                                                   <BIT1> - BIOS\n\
                                                   <BIT2> - CPLD\n \
                                                   <BIT4> - ME\n \
-mse,-img-select                              Option to specify the Image to be updated\n\
                                                    0 - Inactive Image\n\
                                                    1 - Image 1\n\
                                                    2 - Image 2\n\
                                                    3 - Both Images\n \
-a,-activate                                  Option to activate peripheral devices\n\
                                                   <BIT0> - BMC\n\
                                                   <BIT1> - BIOS\n\
                                                   <BIT2> - CPLD\n \
 -ini 						 	Option to give ini file as input.Ini file should be present in the current directory of the Yafuflash executable or in /etc folder\n\
 							1. Yafu_SingleImage.ini - For Single Image.\n\
 							2. Yafu_DualImage.ini   - For Dual Image.\n\
 							3. Yafu_MMCImage.ini  - For MMC Image.\n\
-spi , -mmc                                   Option to Flash HPM Image Component wise\n\
 							    0 -BOTH\n\
 							    1 -SPI Image\n\
 							    2- MMC Image\n"
                                                   
#define STR_HELP7   " \
-nr,-no-reboot                                Option to skip the reboot\n"


#define STR_HELP4   " \
-pXXX,-preserve-XXX                           Option to preserve XXX configuration. Where XXX falls in sdr, fru, sel, ipmi, auth, net,\n \
                                              ntp, snmp, ssh, kvm, syslog. If the preserve status of another configuration is enabled, then \n \
                                              it will ask to confirm that those configuration is to be preserved.\n \
-pXXX,-preserve-XXX -ieo,                     Option to preserve only XXX configuration \n \
-ignore-existing-overrides                    -ignore-existing-overrides must be used with preserve-XXX option \n"

#define STR_HELP5  " \
-rp,-replace-publickey                        Option to replace the Signed Image Key in Existing Firmware\n \
-vcf,-version-cmp-flash                       Option to skip flashing modules only if the versions are same by selecting (N/n).\n \
                                              Option (Y/y) Selects full firmware upgrade mode.\n \
-non-interactive                              This option skips user interaction. This option cannot be used along with 'ignore-diff-image',\n \
                                              'ignore-same-image', '-ignore-module-location' & '-ignore-boot-version' options.\n"

#ifndef MSDOS
#define STR_HELP2  "\
MEDIUM : \n \
-cd                                           Option to use USB Medium \n \
-nw,-ip,-u,-p,-host,-port                        Option to use Network Medium \n \
                                              '-ip' Option to enter IP, when using Network Medium \n \
                                              '-host' Option to enter host name, When using Network Medium \n \
                                              '-u' Option to enter UserName, When using Network Medium \n \
                                              '-p' Option to enter Password, When using Network Medium \n \
                                              '-port' Option to enter Port Number\n \
-kcs                                          Option to use KCS Medium  \n \
FW_IMAGE_FILE : \n \
fw_image_file                                 Firmware Image file name\n"

#else
#define STR_HELP2 "\
MEDIUM : \n \
-kcs                                          Option to use KCS Medium  \n\
FW_IMAGE_FILE : \n \
fw_image_file                                 Firmware Image file name\n"

#endif
/* 
-serial,-term,-baudrate                       Option to use Serial Medium  \n\
                                              '-term' Option to enter serial terminal, when using Serial Medium (ex: /dev/ttyS0) \n\
                                              '-baudrate' Option to enter serial terminal baud rate, when using Serial Medium (ex: 115200) \n\
*/
#ifndef MSDOS
#define STR_EXAMPLE "\
EXAMPLES : \n\
NETWORK Medium:\n\
Flash the Firmware with preserving boot section                     : # Yafuflash -nw -ip xx:xx:xx:xx -u admin -p admin rom.ima \n\
Flash the Firmware without preserving boot section                  : # Yafuflash -nw -ip xx:xx:xx:xx -u admin -p admin rom.ima {-force-boot | -fb}\n\
Flash the Firmware with preserving both boot and conf section       : # Yafuflash -nw -ip xx:xx:xx:xx -u admin -p admin rom.ima {-preserve-config | -pc}\n\
Flash the Firmware with preserving conf section only                : # Yafuflash -nw -ip xx:xx:xx:xx -u admin -p admin rom.ima {-force-boot | -fb} {-preserve-config | -pc}\n\
Flash the Firmware by using BMC hostname                            : # Yafuflash -nw -host bmchost -u admin -p admin rom.ima {-force-boot | -fb} \n\n\
Displays the information about the Images                           : # Yafuflash -nw -ip xx:xx:xx:xx -u admin -p admin rom.ima -info\n\n\
Flash the ME Firmware Firmware                                      : # Yafuflash -nw -ip xx:xx:xx:xx -u admin -p admin -d <BIT16> ME_Img\n\n\
USB Medium:\n\
Flash the Firmware with USB Medium                                  : # Yafuflash -cd rom.ima\n\
Flash the Firmware with preserving conf section                     : # Yafuflash -cd rom.ima {-preserve-config | -pc}\n\n\
KCS Medium:\n\
Flash the Firmware with KCS Medium                                  : # Yafuflash -kcs rom.ima\n\
Flash the Firmware with KCS Medium using OEM specific Netfun        : # Yafuflash -kcs -netfn 0xXX rom.ima\n\
Flash the Firmware with preserving conf section                     : # Yafuflash -kcs rom.ima {-preserve-config | -pc}\n\n\
QUIET Operation:\n\
Full Upgrade Mode                                                   : # Yafuflash [MEDIUM]  {-full | -f} {-preserve-config | -pc}  {-ignore-platform-check | -ipc}  {-ignore-diff-image | -idi}\n\
                                                                       {-ignore-module-location | -iml} {-ignore-same-image | -isi}  {{-ignore-boot-version | -ibv} | {-force-boot | -fb}}\n\
                                                                    [FW_IMAGE_FILE]\n\
Flash Split Image Firmware                                          : # Yafuflash -nw -ip xx:xx:xx:xx -u admin -p admin {-split-img | -msp} root.ima\n\
Non Interactive section based upgrade                               : # Yafuflash -nw -ip xx:xx:xx:xx -u admin -p admin rom.ima {-flash-conf | -f-conf} {-flash-www | -f-www}\n\
Non Interactive section based upgrade with split image              : # Yafuflash -nw -ip xx:xx:xx:xx -u admin -p admin root.ima {-split-img | -msp} {-flash-osimage | -f-osimage} {-flash-conf | -f-conf}\n\
Flash the HPM Image using network medium                            :# Yafuflash -nw -ip xx.xx.xx.xx -u admin -p admin output.hpm \n\
Flash the HPM Image component using network medium                  :# Yafuflash -nw -ip xx.xx.xx.xx -u admin -p admin output.hpm {-spi | -mmc}\n\
Flash the Single Image using INI File                               :# Yafuflash -ini Yafu_SingleImage.ini rom.ima \n\
Flash the Dual Image using INI File                                 :# Yafuflash -ini Yafu_DualImage.ini rom.ima \n\
Flash the MMC using INI File                                        :# Yafuflash -ini Yafu_MMCImage.ini rom.ima\n\
"

/*Serial Medium:\n\
Flash the Firmware with Serial Medium                               : # Yafuflash -serial -term xxxxx:yyyyyy -u admin -p admin rom.ima\n\*/
#else   
#define STR_EXAMPLE "\
EXAMPLES : \n\
KCS Medium:\n\
Flash the Firmware with KCS Medium                                  : # Yafuflash -kcs rom.ima\n\
Flash the Firmware with preserving conf section                     : # Yafuflash -kcs rom.ima {-preserve-config | -pc}\n\n\
QUIET Operation:\n\
Full Upgrade Mode                                                   : # Yafuflash -kcs  {-preserve-config | -pc}  {-ignore-platform-check | -ipc}  {-ignore-diff-image | -idi}\n\
                                                                       {-ignore-module-location | -iml} {-ignore-same-image | -isi}  {{-ignore-boot-version | -ibv} | {-force-boot | -fb}}\n\
                                                                       [FW_IMAGE_FILE]\n\
"
#endif

#ifdef CONFIG_SPX_FEATURE_YAFUFLASH_ENABLE_INTERACTIVE_UPGRADE
#define STR_EXAMPLE1   "\
Interactive Upgrade Mode                                            : # Yafuflash [MEDIUM] -i  {-ignore-platform-check | -ipc}\n\
                                                                      {-ignore-diff-image | -idi}  {-full | -f} [FW_IMAGE_FILE]\n\n\
"
#else
#define STR_EXAMPLE1  ""
#endif

#define STR_EXAMPLE3   "\
Flash with preserve sdr configurations only (Iterative mode)        : #./Yafuflash [MEDIUM] {-preserve-sdr | -psdr} \n\
Flash with preserve sdr & ntp configurations only (Iterative mode)  : #./Yafuflash [MEDIUM] {-preserve-sdr | -psdr} {-preserve-ntp | -pntp} \n\n\
Flash with preserve fru configurations only (Quiet mode)            : #./Yafuflash [MEDIUM] {-preserve-fru | -pfru} {-ignore-existing-overrides | -ieo} \n\
Flash with preserve fru and snmp configurations only (Quiet mode)   : #./Yafuflash [MEDIUM] {-preserve-fru | -pfru} {-preserve-snmp | -psnmp} {-ignore-existing-overrides | -ieo} \n\n\
"
#ifndef MSDOS
#define STR_EXAMPLE2  "\
NOTE: YafuFlash tool has both IPv4 and IPv6 Support. So, All the options can be used with both type of IPs.\n\
Above showed IPs and user credentials are used for example purpose only. Please give valid BMC IP and User Credentials\n\n\
"
#else
#define STR_EXAMPLE2  ""

#endif
#endif /* __MAIN_H__ */



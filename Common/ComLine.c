/******************************************************************
******************************************************************
***                                                                                                         ***
***        (C)Copyright 2008, American Megatrends Inc.                               ***
***                                                                                                         ***
***                    All Rights Reserved                                                         ***
***                                                                                                         ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA                        ***
***                                                                                                         ***
***                     Phone 770.246.8600                                                       ***
***                                                                                                         ***
******************************************************************
******************************************************************
******************************************************************
*
* Filename: ComLine.c
*
* Author   : Winston <winstonv@amiindia.co.in>
*
******************************************************************/
#include <stdlib.h>
#include <errno.h>
#include "main.h"
#ifndef MSDOS
#include "icc_what.h"
#endif

#include "stdio.h"
#include "string.h"

#ifdef ICC_OS_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "flashcmds.h"

#define NET_IPV4 0
#define NET_IPV6 1

#ifdef ICC_OS_LINUX
#include <termios.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifndef MSDOS
#include "icc_what.h"
#include "IPMIDefs.h"
#include "libipmi_session.h"
#include "libipmi_errorcodes.h"
#include "libipmi_struct.h"
#include "IPMI_AMIDevice.h"
#endif

#ifndef ICC_OS_LINUX
#define strtok_r strtok_s
#endif
extern UPDATE_INFO Parsing;
extern char device[46];

char Platformname[15];
extern INT8U byMedium;
extern int SPIDevice;
extern int IsRunLevelON;
extern char *FlashModName[8];
int gParsingCliArgs = 0;
int gImgFile = 0;
INT8U OemNetFn = OEM_AMI_NETFN;

void PrintHelp()
{
    printf(STR_HELP);
#ifdef MSDOS
    system("pause");
    system("cls");
#endif
    printf(STR_HELP_MORE);
#ifdef MSDOS
    system("pause");
    system("cls");
#endif
    printf(STR_HELP_MORE1);
    printf(STR_HELP1);
    printf(STR_HELP3);
    printf(STR_HELP7);
    printf(STR_HELP4);
    printf(STR_HELP5);
    printf(STR_HELP2);
    printf(STR_HELP6);
#ifdef CONFIG_SPX_FEATURE_SD_EMMC_SUPPORT
    printf(STR_HELP8);
#endif
}

int getIPAddressbyhostname(char host[64]);

static int Parse_NonConfigurable_Arguments(int Argc, int *index, char *Arg, char *Value)
{
    char *SplitOpt = NULL, *token = NULL;
#ifndef MSDOS
    int retval = 0, Count = 0;
#endif

    if ((spstrcasecmp(Arg, "-?")) == 0 || (spstrcasecmp(Arg, "-h")) == 0)
    {
        PrintHelp();
        return -1;
    }

    if (spstrcasecmp(Arg, "-V") == 0)
    {
        if (Argc > 2)
        {
            printf(STR_VERSION_VALID_OPTION);
            PrintHelp();
            return -1;
        }
        else
        {
            Print_copyrights();
            exit(0);
        }
    }
    else if (spstrcasecmp(Arg, "-e") == 0)
    {
        if (Argc > 2)
        {
            printf(STR_VERSION_INVALID_E_OPTION);
            PrintHelp();
            return -1;
        }
        else
        {
            printf(STR_EXAMPLE);
            printf(STR_EXAMPLE1);
            printf(STR_EXAMPLE3);
            printf(STR_EXAMPLE2);
            exit(0);
        }
    }
    else if (spstrcasecmp(Arg, "-info") == 0)
    {
        Parsing.Info = 0x01;
        return 0;
    }
    else if (spstrcasecmp(Arg, "-auto") == 0)
    {
        printf(STR_ERR_INVALID_PARAMETERS);
        printf(" %s\r\n", Arg);
        PrintHelp();
        return -1;
    }
    else if (Arg[0] == ':')
    {
        printf(STR_ERR_INVALID_PARAMETERS);
        printf(" %s\r\n", Arg);
        PrintHelp();
        return -1;
    }
    else if ((Arg[0] != '-') && (Arg[0] != ':') && (Arg[0] != '0'))
    {
        if (strlen(Arg) > MAX_FILE_NAME_SIZE)
        {
            printf(STR_FILE_SIZE_INVALID);
            printf("%s\r\n", Arg);
            return -1;
        }
        Parsing.DoSPIImgUpdate = 1;
        strcpy(Parsing.FileName, Arg);
        gImgFile = 1;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-img-section-info") == 0) || (spstrcasecmp(Arg, "-msi") == 0))
    {
        Parsing.ImgInfo = 1;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-img-info") == 0) || (spstrcasecmp(Arg, "-mi") == 0))
    {
        Parsing.ImgInfo = 2;
        return 0;
    }
    else
    {
        printf(STR_ERR_INVALID_PARAMETERS);
        printf(" %s\r\n", Arg);
        PrintHelp();
        return -1;
    }

    return -1;
}

static int Parse_Configurable_Arguments(int Argc, int *index, char *Arg, char *Value)
{
    char Filename[MAX_FILE_NAME_SIZE];
    char *SplitOpt = NULL, *token = NULL;
#ifndef MSDOS
    char hostname[64];
    int retval = 0, Count = 0;
#endif

    if (spstrcasecmp(Arg, "-ip") == 0)
    {
        Parsing.IPEntered = 0x01;
        if (Value != NULL)
        {
            if (strlen(Value) < sizeof(device))
            {
                strcpy(device, Value);
                if (gParsingCliArgs)
                    (*index)++;
            }
            else
            {
                printf("Network address string is too long.\r\n");
                exit(YAFU_NAME_LONG);
            }
        }
        else
        {
            printf("Please supply the network address of the device.\r\n");
            exit(YAFU_INVALID_NAME);
        }

        if (!isIPvalid())
        {
            printf("Please provide valid IP address after '-ip' option\r\n");
            exit(YAFU_INVALID_IP);
        }
        return 0;
    }

#ifndef MSDOS
    if (spstrcasecmp(Arg, "-host") == 0)
    {
        Parsing.HostEntered = 0x01;
        if (Value != NULL)
        {
            if (strlen(Value) < sizeof(hostname))
            {
                strcpy(hostname, Value);
                if (gParsingCliArgs)
                    (*index)++;
            }
            else
            {
                printf(" HostName length is too long.\r\n");
                exit(YAFU_NAME_LONG);
            }
        }
        else
        {
            printf("Please supply a HostName of the device.\r\n");
            exit(YAFU_INVALID_NAME);
        }
        retval = getIPAddressbyhostname(hostname);
        if (retval != 0)
        {
            printf("Please supply a valid hostname of the device.\r\n");
            exit(YAFU_INVALID_NAME);
        }
        return 0;
    }
#endif

    if ((spstrcasecmp(Arg, "-sd")) == 0)
    {
        if (strlen(Arg) > MAX_FILE_NAME_SIZE)
        {
            printf(STR_FILE_SIZE_INVALID);
            printf("%s\r\n", Arg);
            return -1;
        }

        Parsing.DoMMCImgUpdate = 1;
        strcpy(Parsing.MMCImgName, Value);
        (*index)++;
        return 0;
    }

    if ((spstrcasecmp(Arg, "-spi")) == 0)
    {
        Parsing.UpdateSPIImage = 1;
        return 0;
    }

    if ((spstrcasecmp(Arg, "-mmc")) == 0)
    {
        Parsing.UpdateMMCImage = 1;
        return 0;
    }

    if (spstrcasecmp(Arg, "-ini") == 0)
    {
        return 0;
    }

    if (spstrcasecmp(Arg, "-d") == 0)
    {
        if (Value != NULL)
        {
            char *endptr;
            SPIDevice = strtol(Value, &endptr, 0);

            if ((endptr == Value) || (*endptr != '\0') || (errno == ERANGE))
            {
                //*endptr != '\0' - catches if character present after digits
                // endptr == Value - catches empty input
                printf("%s ", Arg);
                printf(STR_ERR_OPTION_ARGUMENT);
                return -1;
            }

            if (SPIDevice == 0) // Backward compatablity For BMC Flash
                SPIDevice = 1;

            // Check whether the provided peripheral is either BMC/BIOS/CPLD/MMC/ME
            if ((SPIDevice != BMC_FLASH) && (SPIDevice != BIOS_FLASH) && (SPIDevice != CPLD_FLASH) && (SPIDevice != MMC_FLASH) && (SPIDevice != ME_FLASH))
            {
                printf("%s ", Arg);
                printf(STR_ERR_OPTION_ARGUMENT);
                return -1;
            }

            if (gParsingCliArgs)
                (*index)++;
            return 0;
        }
        else
        {
            printf("%s ", Arg);
            printf(STR_ERR_OPTION_ARGUMENT);
            PrintHelp();
            return -1;
        }
    }

    if ((spstrcasecmp(Arg, "-activate") == 0) || (spstrcasecmp(Arg, "-a") == 0))
    {
        ActivateOnly = 1;
        if (Value != NULL)
        {
            char *endptr;
            Parsing.ActivateNode = strtol(Value, &endptr, 0);

            if ((endptr == Value) || (*endptr != '\0') || (errno == ERANGE))
            {
                printf("%s ", Arg);
                printf(STR_ERR_OPTION_ARGUMENT);
                return -1;
            }

            // Check whether the provided peripheral is either BMC/BIOS/CPLD
            if ((Parsing.ActivateNode != BMC_FLASH) && (Parsing.ActivateNode != BIOS_FLASH) && (Parsing.ActivateNode != CPLD_FLASH))
            {
                printf("%s ", Arg);
                printf(STR_ERR_OPTION_ARGUMENT);
                return -1;
            }

            if (gParsingCliArgs)
                (*index)++;
            return 0;
        }
        else
        {
            Parsing.ActivateNode = 0xFF;
            return 0;
        }
    }

    if ((spstrcasecmp(Arg, "-no-reboot") == 0) || (spstrcasecmp(Arg, "-nr") == 0))
    {
        Parsing.RebootFirmware = 0x00;
        return 0;
    }

    if ((spstrcasecmp(Arg, "-skip-fmh") == 0) || (spstrcasecmp(Arg, "-sf") == 0))
    {
        Parsing.SkipFMHCheck = 0x01;
        return 0;
    }
    if ((spstrcasecmp(Arg, "-skip-crc") == 0) || (spstrcasecmp(Arg, "-sc") == 0))
    {
        Parsing.SkipCRCValidation = 0x01;
        return 0;
    }

    if ((spstrcasecmp(Arg, "-img-select") == 0) || (spstrcasecmp(Arg, "-mse") == 0))
    {
        if (Value != NULL)
        {
            Parsing.ImgSelect = atoi(Value);
            if (gParsingCliArgs)
                (*index)++;
        }

        if ((Parsing.ImgSelect != IMAGE_1) && (Parsing.ImgSelect != IMAGE_2) && (Parsing.ImgSelect != IMAGE_BOTH) && (Parsing.ImgSelect != AUTO_INACTIVE_IMAGE))
        {
            printf(" %s\r\n", Arg);
            printf(STR_ERR_OPTION_ARGUMENT);
            PrintHelp();
            return -1;
        }

        return 0;
    }

    if (spstrcasecmp(Arg, "-vdbg") == 0)
    {
        // Enable verbose debug
        vdbg = 1;
        return 0;
    }
    else if (spstrcasecmp(Arg, "-vyes") == 0)
    {
        // Enable non interactive backward compatibility.
        non_interactive = 1;
        return 0;
    }
    else if (spstrcasecmp(Arg, "-i") == 0)
    {
#ifdef CONFIG_SPX_FEATURE_YAFUFLASH_ENABLE_INTERACTIVE_UPGRADE
        Parsing.Full = 0x00;
        Parsing.Interactive = 0x01;
        return 0;
#else
        printf(STR_ERR_INVALID_PARAMETERS);
        printf(" %s\r\n", Arg);
        PrintHelp();
        return -1;
#endif
    }
    else if ((spstrcasecmp(Arg, "-version-cmp-flash") == 0) || (spstrcasecmp(Arg, "-vcf") == 0))
    {
        Parsing.versioncmpflash = 1;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-replace-publickey") == 0) || (spstrcasecmp(Arg, "-rp") == 0))
    {
        Parsing.SignKey = 0x01;
        if ((Value != NULL) && (strlen(Value) < MAX_FILE_NAME_SIZE))
        {
            strcpy(Parsing.SignKeyFile, Value);
        }
        else if (Value == NULL)
        {
            printf("Invalid Signed Image Key filename \r\n");
            exit(YAFU_INVALID_NAME);
        }
        else
        {
            printf("Signed Image Key filename string is too long \r\n");
            exit(YAFU_NAME_LONG);
        }
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-force-boot") == 0) || (spstrcasecmp(Arg, "-fb") == 0))
    {
        non_interactive = 0x01;
        Parsing.BootPreserve = 0x00;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-config") == 0) || (spstrcasecmp(Arg, "-pc") == 0))
    {
        Parsing.ConfigPreserve = 0x01;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-root") == 0) || (spstrcasecmp(Arg, "-pr") == 0))
    {
        Parsing.RootPreserve = 0x01;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-www") == 0) || (spstrcasecmp(Arg, "-pw") == 0))
    {
        Parsing.WWWPreserve = 0x01;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-ignore-platform-check") == 0) || (spstrcasecmp(Arg, "-ipc") == 0))
    {
        Parsing.PlatformCheck = 0x01;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-ignore-diff-image") == 0) || (spstrcasecmp(Arg, "-idi") == 0))
    {
        Parsing.DiffImage = 0x01;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-ignore-same-image") == 0) || (spstrcasecmp(Arg, "-isi") == 0))
    {
        Parsing.SameImage = 0x01;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-ignore-module-location") == 0) || (spstrcasecmp(Arg, "-iml") == 0))
    {
        Parsing.ModLocation = 0x01;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-ignore-boot-version") == 0) || (spstrcasecmp(Arg, "-ibv") == 0))
    {
        Parsing.BootVersion = 0x01;
        return 0;
    }
    else if (spstrcasecmp(Arg, "-non-interactive") == 0)
    {
        non_interactive = 1;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-ignore-reselect-image") == 0) || (spstrcasecmp(Arg, "-iri") == 0))
    {
        Parsing.ReselectImage = 1;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-ignore-non-preserve-config") == 0) || (spstrcasecmp(Arg, "-inc") == 0))
    {
        Parsing.NotPreserveCfg = 1;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-extlog") == 0) || (spstrcasecmp(Arg, "-pe") == 0))
    {
        // Parsing.ExtlogPreserve = 0x01;
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT13;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-full") == 0) || (spstrcasecmp(Arg, "-f") == 0))
    {
        Parsing.InteractiveFull = 0x01;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-quiet") == 0) || (spstrcasecmp(Arg, "-q") == 0))
    {
        Parsing.Silent = 0x01;
        return 0;
    }
    else if (spstrcasecmp(Arg, "-kcs") == 0)
    {
        Parsing.KcsMedium = 0x01;
        byMedium = KCS_MEDIUM;
        return 0;
    }
    else if (spstrcasecmp(Arg, "-netfn") == 0)
    {
        if (Value != NULL)
        {
            if (Value[0] == '0' && Value[1] == 'x')
            {
                if ((spstrcasecmp(Value, OEM_MIN_RANGE) >= 0) && (spstrcasecmp(Value, OEM_MAX_RANGE) <= 0)) // Oem NetFn range( 0x30 - 0x3f)
                {
                    OemNetFn = strtol(Value, NULL, 16);
                    (*index)++;
                    return 0;
                }
                else
                {
                    printf(STR_ERR_INVALID_PARAMETERS);
                    printf(" Given Netfun %s is out of range of OEM Netfun.\r\n", Value);
                    PrintHelp();
                    return -1;
                }
            }
            else
            {
                printf(STR_ERR_INVALID_PARAMETERS);
                printf("Invalid NetFun.  %s\r\n", Value);
                PrintHelp();
                return -1;
            }
        }
    }
    else if ((spstrcasecmp(Arg, "-block-upgrade") == 0) || (spstrcasecmp(Arg, "-bu") == 0))
    {
        Blockselect = 0x01;
        return 0;
    }

#ifndef MSDOS // the below options are not supported in DOS

    else if (spstrcasecmp(Arg, "-cd") == 0)
    {
        Parsing.UsbMedium = 0x01;
        byMedium = USB_MEDIUM;
        return 0;
    }
    else if (spstrcasecmp(Arg, "-nw") == 0)
    {
        Parsing.NwMedium = 0x01;
        byMedium = NETWORK_MEDIUM;
        return 0;
    }
#if 0       
    else if(spstrcasecmp(Arg,"-serial") == 0)
    {
        Parsing.SerialMedium = 0x01;
        byMedium = SERIAL_MEDIUM;
        return 0;
    }
#endif
    else if (spstrcasecmp(Arg, "-u") == 0)
    {
        Parsing.UserNameExist = 0x01;
        if (strlen(Value) < MAX_USERNAME_SIZE)
        {
            strcpy(Parsing.Username, Value);
            if (gParsingCliArgs)
                (*index)++;
        }
        else
        {
            printf("Username field string is too long \r\n");
            exit(YAFU_NAME_LONG);
        }
        return 0;
    }
    else if (spstrcasecmp(Arg, "-p") == 0)
    {
        Parsing.PasswordExist = 0x01;
        if (strlen(Value) < MAX_PASSWORD_SIZE)
        {
            strcpy(Parsing.Password, Value);
            if (gParsingCliArgs)
                (*index)++;
        }
        else
        {
            printf("Password  field string is too long \r\n");
            exit(YAFU_NAME_LONG);
        }
        return 0;
    }
    else if (spstrcasecmp(Arg, "-port") == 0)
    {
        Parsing.Portentered = 0x01;
        Parsing.Portnum = atoi(Value);
        return 0;
    }
#if 0
    else if( spstrcasecmp( Arg, "-term" ) == 0 )
    {
        Parsing.SerialTermEntered = 0x01;
        if ( Value != NULL )
        {
            if ( strlen(Value) < sizeof( device ) )
            {
                strcpy( device, Value );
                if (gParsingCliArgs)
                    (*index)++;
            }
            else
            {
                printf( "Serial terminal string is too long.\r\n" );
                exit(YAFU_NAME_LONG);
            }
        }
        else
        {
            printf( "Please supply the serial terminal of the device.\r\n" );
            exit(YAFU_INVALID_NAME);
        }
        return 0;
    }
    else if( spstrcasecmp( Arg, "-baudrate" ) == 0 )
    {
        Parsing.SerialBaudEntered = 0x01;
        Parsing.BaudRate = atol(Value);
        return 0;
    }
#endif

#endif
    else if ((spstrcasecmp(Arg, "-preserve-sdr") == 0) || (spstrcasecmp(Arg, "-psdr") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT0;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-fru") == 0) || (spstrcasecmp(Arg, "-pfru") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT1;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-sel") == 0) || (spstrcasecmp(Arg, "-psel") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT2;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-ipmi") == 0) || (spstrcasecmp(Arg, "-pipmi") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT3;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-net") == 0) || (spstrcasecmp(Arg, "-pnet") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT4;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-ntp") == 0) || (spstrcasecmp(Arg, "-pntp") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT5;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-snmp") == 0) || (spstrcasecmp(Arg, "-psnmp") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT6;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-ssh") == 0) || (spstrcasecmp(Arg, "-pssh") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT7;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-kvm") == 0) || (spstrcasecmp(Arg, "-pkvm") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT8;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-auth") == 0) || (spstrcasecmp(Arg, "-pauth") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT9;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-preserve-syslog") == 0) || (spstrcasecmp(Arg, "-psyslog") == 0))
    {
        Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask | BIT10;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-ignore-existing-overrides") == 0) || (spstrcasecmp(Arg, "-ieo") == 0))
    {
        Parsing.IgnoreExisting = 1;
        return 0;
    }
    else if ((spstrcasecmp(Arg, "-split-img") == 0) || (spstrcasecmp(Arg, "-msp") == 0))
    {
        Parsing.SplitImg = 1;
        return 0;
    }
    else if ((spstrncasecmp(Arg, "-flash-", strlen("-flash-")) == 0) || spstrncasecmp(Arg, "-f-", strlen("-f-")) == 0)
    {
        Parsing.FlashModImg = 1;
        token = strtok_r(Arg, "-", &SplitOpt);
        if (SplitOpt != NULL && strlen(SplitOpt))
        {
            FlashModName[Parsing.FlashModCnt] = (char *)malloc(strlen(SplitOpt) + 1);
            strncpy(FlashModName[Parsing.FlashModCnt], SplitOpt, strlen(SplitOpt) + 1);
            Parsing.FlashModCnt++;
        }
        return 0;
    }
    else if ((Arg[0] != '-') && (gParsingCliArgs == 0))
    {
        printf("ERROR: Invalid Entry \"%s\" found in the Yafu INI Configuration... Ignoring this option...\r\n", Arg);
        return 0;
    }

    return -1;
}

int CommandLineParsing(int argc, char *argv[])
{
    int i, j, UpgradeOption = 0;
    char Filename[MAX_FILE_NAME_SIZE];
    char MMCImgName[MAX_FILE_NAME_SIZE];
    char *SplitOpt = NULL, *token = NULL;
#ifndef MSDOS
    char hostname[64];
    int retval = 0, Count = 0;
#endif
    HANDLE iniHandle = 0;
    int SecCount = 0;
    int EntryCount = 0;
    UINT8 tmpType = 0;
    char *value = NULL;
    char *SectionName = NULL;
    char *EntryName = NULL;
    int SecNameLength = 0;
    char IniFileName[MAX_FILE_NAME_SIZE] = {0};
    char IniConfigFile[MAX_FILE_NAME_SIZE] = {0};
    int conflit_count = 0;

    if (argc < 2)
    {
        PrintHelp();
        return -1;
    }

    memset((char *)&Parsing, 0, sizeof(Parsing));
    memcpy((char *)&Filename[0], (char *)&Parsing.FileName[0], MAX_FILE_NAME_SIZE);
    memcpy((char *)&MMCImgName[0], (char *)&Parsing.MMCImgName[0], MAX_FILE_NAME_SIZE);
    Parsing.BootPreserve = 0x01;
    Parsing.Full = 0x01;
    Parsing.SkipFMHCheck = 0x00;
    Parsing.SkipCRCValidation = 0x00;
    Parsing.RebootFirmware = 0x01;
    Parsing.ImgSelect = -1;
    SPIDevice = 1;
    IsRunLevelON = 0;
    Parsing.FlashModCnt = 0;
    Parsing.FlashModImg = 0;
    Parsing.ActivateNode = 0;
    ActivateOnly = 0;
    Parsing.DoMMCImgUpdate = 0;
    Parsing.DoSPIImgUpdate = 0;

    for (i = 0; i < argc; i++)
    {
        if (spstrcasecmp(argv[i], "-ini") == 0)
        {
            strcpy(IniConfigFile, argv[i + 1]);
            break;
        }
    }
    for (i = 0; i < argc; i++)
    {
        if ((spstrcasecmp(argv[i], "-preserve-config") == 0) || (spstrcasecmp(argv[i], "-pc") == 0))
        {
            conflit_count++;
        }
        else if (spstrcasecmp(argv[i], "-flash-conf") == 0)
        {
            conflit_count++;
        }
        if (conflit_count == 2)
        {
            printf("Invalid Option: While Flashing Conf section, Whole Configuration can't be Preserved ...\r\n");
            return -1;
        }
    }

    sprintf(IniFileName, "%s", IniConfigFile);
    iniHandle = InitIniTable(IniFileName);
    if ((char *)iniHandle == NULL)
    {
        printf("INFO: Yafu INI Configuration File not found... Default options will not be applied...\r\n");
        goto check_cmdline_args;
    }

    SecCount = GetIniSectionCount(iniHandle);
    EntryCount = GetIniEntryCount(iniHandle);
    if ((SecCount - 1) <= 0)
    {
        printf("INFO: Yafu INI Configuration not seem to be Valid... Default options will not be applied...\r\n");
        goto check_cmdline_args;
    }

    for (i = 0; i < (SecCount - 1); i++)
    {
        SectionName = (char *)GetIniSectionName(iniHandle, i);
        SecNameLength = (int)strlen(SectionName) + 1;

        if (spstrcasecmp(SectionName, STR_CONF_SEC_NAME) != 0)
        {
            printf("INFO: Yafu INI Configuration seems to be corrupted... Default options will not be applied...\r\n");
            goto check_cmdline_args;
        }

        for (j = 0; j < EntryCount; j++)
        {
            EntryName = (char *)GetIniEntryName(iniHandle, j);
            if (EntryName != NULL)
            {
                if (ReadIniEntry(iniHandle, SectionName, &EntryName[SecNameLength], &value, &tmpType) == TRUE)
                {
                    Parse_Configurable_Arguments(argc, &i, &EntryName[SecNameLength], value);
                }
            }
        }
    }

check_cmdline_args:
    gParsingCliArgs = 1;

    for (i = 1; i < argc; i++)
    {
        if (Parse_Configurable_Arguments(argc, &i, argv[i], argv[i + 1]) < 0)
        {
            if (Parse_NonConfigurable_Arguments(argc, &i, argv[i], argv[i + 1]) < 0)
                return -1;
        }
    }

    gParsingCliArgs = 0;

    /* Assigning default RMCP port number when '-P' option is not specified*/
    if (Parsing.Portentered == 0x00)
    {
        Parsing.Portnum = 623;
    }

    if (Parsing.SignKey == 0)
    {
        if ((spstrcasecmp(Filename, Parsing.FileName) == 0) && (Parsing.ImgInfo == 0) && (Parsing.ActivateNode == 0x00))
        {
            printf(STR_FILE_NAME_NOT_GIVEN);
            PrintHelp();
            return -1;
        }
    }
    else if ((spstrcasecmp(Filename, Parsing.FileName) != 0) && (Parsing.ImgInfo != 0) && (Parsing.ActivateNode == 0x00))
    {
        printf(STR_ERR_INVALID_PARAMETERS);
        PrintHelp();
        return -1;
    }
    /*#ifdef CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT
        if((Parsing.ImgSelect != -1) && (Parsing.ReselectImage))
        {
             printf(STR_INVALID_IMAGE_SELECTOR_OPTION);
             PrintHelp();
             return -1;
        }
    #endif*/

    if (((Parsing.UsbMedium == 0x00) && (Parsing.NwMedium == 0x00) && (Parsing.KcsMedium == 0x00) && (Parsing.SerialMedium == 0x00)) ||
        ((Parsing.UsbMedium == 0x00) && (Parsing.NwMedium == 0x00) && (Parsing.KcsMedium == 0x00) && (Parsing.SerialMedium == 0x00) &&
         (Parsing.IPEntered == 0x01)))
    {
        printf(STR_NOMEDIUM_OPTION);
        PrintHelp();
        return -1;
    }

    if (((Parsing.UsbMedium) ^ (Parsing.NwMedium) ^ (Parsing.KcsMedium) ^ (Parsing.SerialMedium)) == 0x00)
    {
        printf(STR_MEDIUM_OPTION);
        PrintHelp();
        return -1;
    }

    if (((Parsing.UsbMedium == 0x01) || (Parsing.KcsMedium == 0x01) || (Parsing.SerialMedium == 0x01)) && (Parsing.IPEntered == 0x01))
    {
        printf(STR_USB_KCS_NOOPTION_IP);
        PrintHelp();
        return -1;
    }
    if ((Parsing.UsbMedium == 0x01) && (Parsing.HostEntered == 0x01))
    {
        printf(STR_USB_KCS_NOOPTION_HOST);
        PrintHelp();
        return -1;
    }
    if ((Parsing.NwMedium == 0x01) && (Parsing.IPEntered == 0x00) && (Parsing.HostEntered == 0x00))
    {
        printf(STR_ENTER_IP_HOST);
        PrintHelp();
        return -1;
    }
#if 0
    if((Parsing.SerialMedium == 0x01) && ((Parsing.SerialTermEntered == 0x00) || (Parsing.SerialBaudEntered == 0x00)) )
    {
        printf(STR_ENTER_TERM);
        PrintHelp();
        return -1;
    }
#endif
    if ((Parsing.IPEntered == 0x01) && (Parsing.HostEntered == 0x01))
    {
        printf(STR_ENTER_HOST_OR_IP);
        PrintHelp();
        return -1;
    }

    if ((Parsing.NwMedium == 0x00) && (Parsing.SerialMedium == 0x00) && ((Parsing.UserNameExist == 0x01) || (Parsing.PasswordExist == 0x01)))
    {
        printf(STR_INVALID_USERPASS_OPTION);
        PrintHelp();
        return -1;
    }

    if (((Parsing.NwMedium == 0x01) || (Parsing.SerialMedium == 0x01)) && (((Parsing.UserNameExist == 0x01) && (Parsing.PasswordExist == 0x00)) || ((Parsing.UserNameExist == 0x00) && (Parsing.PasswordExist == 0x01))))
    {
        printf(STR_USERPASSCONFLICT_OPTION);
        PrintHelp();
        return -1;
    }

    if (Parsing.NwMedium == 0x00 && Parsing.Portentered == 0x01)
    {
        printf(STR_NW_OPTION);
        PrintHelp();
        return -1;
    }

    if ((non_interactive) && (Parsing.DiffImage || Parsing.SameImage || Parsing.ModLocation || Parsing.BootVersion || Parsing.ReselectImage || Parsing.NotPreserveCfg))
    {
        printf(STR_INVALID_VYES_OPTION);
        return -1;
    }

    if (((Parsing.BootPreserve != 0x01) || (Parsing.ConfigPreserve == 0x01)) && (Parsing.Interactive == 0x01))
    {
        printf(STR_INTERACTIVE_OPTION);
        PrintHelp();
        return -1;
    }

    if ((Parsing.Full == 0x01 || Parsing.Interactive == 0x01))
    {
        UpgradeOption = Parsing.Full;
        UpgradeOption <<= 1;
        UpgradeOption += Parsing.Interactive;
        UpgradeOption <<= 1;
        UpgradeOption += Parsing.Auto;
        UpgradeOption <<= 1;
        if (Parsing.ModuleUpgrade >= 0x01)
            UpgradeOption += 0x01;

        if ((UpgradeOption != 0x02) && (UpgradeOption != 4) && (UpgradeOption != 8) && (UpgradeOption != 0x01))
        {
            Parsing.Full = Parsing.Interactive = Parsing.Auto = Parsing.ModuleUpgrade = 0x00;
            printf(STR_INVALIDUPGRADE_OPTION);
            PrintHelp();
            return -1;
        }
    }

    if ((Parsing.UsbMedium == 0x00) && (Parsing.NwMedium == 0x00) && (Parsing.SerialMedium == 0x00) && (Parsing.Full == 0x00) && (Parsing.Info == 0x00) && (Parsing.ImgInfo == 0x00) && (Parsing.Auto == 0x00) && (Parsing.Interactive == 0x00) && (Parsing.BootPreserve == 1) && (Parsing.ConfigPreserve == 0))
    {
        printf(STR_ENTER_VALID_OPTION);
        PrintHelp();
        return -1;
    }

    if ((Parsing.ConfigPreserveBitMask != 0x00) && (Parsing.ConfigPreserve != 0x00))
    {
        printf(STR_PRESERVE_ITERACTIVE_OPTION);
        PrintHelp();
        return -1;
    }

    if ((Parsing.ConfigPreserveBitMask == 0x00) && (Parsing.IgnoreExisting == 0x01))
    {
        printf(STR_PRESERVE_QUIET_OPTION);
        PrintHelp();
        return -1;
    }

    if ((Parsing.RebootFirmware == 0x00) && (gImgFile == 1) && (Parsing.ActivateNode != 0x00))
    {
        printf(STR_INVALID_ACTIVATE_OPTION);
        PrintHelp();
        return -1;
    }

    if ((gImgFile != 0x00) && (ActivateOnly != 0x00))
    {
        ActivateOnly = 0;
        Parsing.RebootFirmware = 0x00;
    }

    return 0;
}

int getIPAddressbyhostname(char host[64])
{

#ifndef MSDOS
    struct addrinfo *pRes = NULL;
    struct addrinfo *ptr = NULL;
    struct sockaddr_in *pSockaddr;
    struct sockaddr_in6 *pSockaddr6;
    int err, iResult;
    char IPv6Addr[46];

#ifdef ICC_OS_WINDOWS
    WSADATA wsaData;
    DWORD ipbufferlength = 46, iRetval;
    LPSOCKADDR sockaddr_ip;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStrartup Failed %d\r\n", iResult);
        return -1;
    }

#endif

    err = getaddrinfo(host, NULL, NULL, &pRes);
    if (err != 0)
    {
        printf("Error in getting addrinfo %s\r\n", gai_strerror(err));
        exit(YAFU_INVALID_NAME);
    }
    ptr = pRes;

    if (ptr->ai_family == AF_INET6)
    {

#ifdef ICC_OS_WINDOWS
        sockaddr_ip = (LPSOCKADDR)ptr->ai_addr;
        iRetval = WSAAddressToString(sockaddr_ip, (DWORD)ptr->ai_addrlen, NULL, IPv6Addr, &ipbufferlength);
        if (iRetval != 0)
        {
            printf("WSAAddressToString failed with %u\r\n", WSAGetLastError());
            return -1;
        }
#else
        pSockaddr6 = (struct sockaddr_in6 *)ptr->ai_addr;
        inet_ntop(AF_INET6, &pSockaddr6->sin6_addr, IPv6Addr, sizeof(IPv6Addr));
#endif
        strcpy(device, IPv6Addr);
    }
    else
    {
        pSockaddr = (struct sockaddr_in *)ptr->ai_addr;
        strcpy(device, inet_ntoa(pSockaddr->sin_addr));
    }
    freeaddrinfo(pRes);

#ifdef ICC_OS_WINDOWS
    WSACleanup();
#endif

#endif
    return 0;
}

#ifdef ICC_OS_WINDOWS
int stringToNumber(const char *s, unsigned int min, unsigned int max,
                   unsigned int *ret, int type)
{
    long number;
    char *end;

    /* Handle octal, etc. */
    errno = 0;
    if (type == NET_IPV4)
        number = strtol(s, &end, 0);
    else
        number = strtol(s, &end, 16);

    if (*end == '\0' && end != s)
    {
        /* we parsed a number, let's see if we want this */
        if (errno != ERANGE && min <= number && number <= max)
        {
            *ret = number;
            return 0;
        }
    }
    return -1;
}
#endif

int isIPvalid()
{
#ifndef MSDOS

#ifdef ICC_OS_WINDOWS
    unsigned char buf[46];

    /* copy dotted string, because we need to modify it */
    strncpy(buf, device, sizeof(device));

    if (strchr(buf, ':') == NULL)
    {                     // Checking IPV4 Format
#define MAX_IP_NUMBER 255 // max dotted string lieteral for ip like "255.255.255.255"
        char *p = buf, *q;
        unsigned int onebyte;
        int i;

        for (i = 0; i < 3; i++)
        {
            if ((q = strchr(p, '.')) == NULL) // less than 3 dots - error.
                return FALSE;
            *q = '\0';
            if (stringToNumber(p, 0, MAX_IP_NUMBER, &onebyte, NET_IPV4) == -1) // value more than 255 - error.
                return FALSE;
            p = q + 1;
        }
        /* we've checked 3 bytes, now we check the last one */
        if (stringToNumber(p, 0, MAX_IP_NUMBER, &onebyte, NET_IPV4) == -1)
            return FALSE;
    }
    else
    {                          // Checking IPV6 Format
#define MAX_IPV6_NUMBER 0xffff // max dotted string lieteral for ipv6 like "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"
        char *p = buf, *q;
        unsigned int onebyte;
        int i;

        for (i = 0; i < 7; i++)
        {
            if ((i == 0) && *p == ':')
            {
                // fist digi is all zero
                p++;
                continue;
            }
            else
            {
                if ((q = strchr(p, ':')) == NULL)
                    break;

                // the between is zero
                if (q == p)
                {
                    p = q + 1;
                    continue;
                }

                *q = '\0';
                if (stringToNumber(p, 0, MAX_IPV6_NUMBER, &onebyte, NET_IPV6) == -1) // value more than 65535 - error.
                    return FALSE;
                p = q + 1;
            }
        }
        /* Checking the last one */
        if (stringToNumber(p, 0, MAX_IPV6_NUMBER, &onebyte, NET_IPV6) == -1)
            return FALSE;
    }
#else
    int retval = TRUE;
    unsigned char buf[sizeof(struct in6_addr)];
    retval = inet_pton(AF_INET, device, buf);
    if (retval == 0)
    {
        retval = inet_pton(AF_INET6, device, buf);
        if (retval == FALSE)
        {
            return retval;
        }
    }
#endif

#endif
    return TRUE;
}

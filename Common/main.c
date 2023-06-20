/*****************************************************************
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
* Filename: main.c
*
* Author   : Winston <winstonv@amiindia.co.in>
*
******************************************************************/

/* Includes */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#ifdef WINDOWS
#include <conio.h>
#include <windows.h>
#include <winioctl.h>
#include <winsvc.h>
#else
#include <ctype.h>
#endif
#include "flashcmds.h"
#include "meupdate.h"
#include "main.h"
#include "fmh.h"
#include "bmc_ifc.h"
#ifndef MSDOS
#include "icc_what.h"
#include "IPMIDefs.h"
#include "libipmi_session.h"
#include "libipmi_errorcodes.h"
#include "IPMI_AMIDevice.h"
#include "libipmi_struct.h"
#include "libipmi_AMIOEM.h"
#include "coreTypes.h"
#else
#include "flashlib.h"
#endif

#if defined(__x86_64__) || defined(WIN64)
unsigned int NewImageSize = 0;
unsigned int oldVersion = 0x00;
#else
unsigned long NewImageSize = 0;
unsigned long oldVersion = 0x00;
#endif
extern char Platformname[15];

char Img1BootVer[16] = "\0";
char Img2BootVer[16] = "\0";
char NewImgBootVer[16] = "\0";

extern int GetPasswordInput(char *pwd);
extern void FreeFMHMemory(void);

int vdbg = 0;            // enable by hidden command line option
int non_interactive = 0; // enable by hidden command line option
int Blockselect = 0;     // enable the block by block upgrade
#define MAX_FMHVER_LEN 512
#define ACTIVE "Active"
#define BACKUP "Backup"

#ifdef ICC_OS_LINUX
typedef struct
{
    char ModuleName[MAX_MODULE_NAME];
    int LoadModule;
} OpenIPMIDrv_T;

OpenIPMIDrv_T OpenIPMIDriver[] = {
    {"ipmi_devintf", 0},
    {"ipmi_si", 0},
    {"ipmi_msghandler", 0},
};

#endif

DWORD g_ErrCode;

char BootValues[MAX_UBOOT_VAR_COUNT][MAX_UBOOT_VAR_LENGTH];
char bvnArr[MAX_UBOOT_VAR_COUNT * MAX_UBOOT_VAR_NAME_LENGTH]; // memory for BootVarsName
char UpReleaseID[MAX_MODULE_NAME];
char UpCodeBaseVersionID[MAX_MODULE_NAME];
char CurReleaseID[MAX_MODULE_NAME];
char CurCodeBaseVersionID[MAX_MODULE_NAME];
int ImgOpt = 0;
env_t env;

INT8U FlashSelected = 0, PreserveFlag = 0xff;
static int Config = 0, Boot = 0;
int FullFlashDone = 0, Matchfound = 0, found = 0, Bkupflag = 0;
static INT8U *Buffer;
static INT8U *Buffer_HPM[MAX_COMPONENT];
static FILE *fp;
static FILE *pImageFile;
FlashMH *FwModuleHdr[MAX_MODULE];
FlashMH *CurrFwModHdr[MAX_MODULE];
FlashMH *DualFwModHdr[MAX_MODULE];
FlashMH *DispFwmodHdr[MAX_MODULE];
FlashMH *DispCurrFwModHdr[MAX_MODULE];
FlashMH *DispDualFwmodHdr[MAX_MODULE];
FlashMH *PrimFwModHdr[MAX_MODULE];
INT32U ConfigRomAddr = -1, ConfigFirmAddr = 0;
INT32U ConfigRomSize = 0, ConfigFirmSize = 0;
INT32U ConfigRomModLoc = 0, ConfigFirmModLoc = 0;
INT32U BootRomModLoc = 0, BootFirmModLoc = 0;
INT32U BootRomAddr = -1, BootFirmAddr = 0;
INT32U BootRomSize = 0, BootFirmSize = 0;
INT32U BootRomAllocAddr = 0, BootFirmAllocAddr = 0;
INT32U DualBootRomAllocAddr = 0, DualBootFirmAllocAddr = 0;
INT32U ConfigRomAllocAddr = 0, ConfigFirmAllocAddr = 0;
INT32U ExtlogFirmAddr = 0, ExtlogFirmAllocAddr = 0, ExtlogRomAddr = -1;
INT32U FreeSpaceRomAddr = 0, FreeSpaceFirmAddr = 0, FreeSpaceFirmSize = 0;
INT32U BootImgFirmAddr = 0, BootImgRomAddr = 0;
int ModuleCount = 0, DispCount = 0;
UPDATE_INFO Parsing;
FEATURE_LIST featuresList; // list of features supported by BMC
int g_confflag = 0;
int ActiveCurImg = 0, FlashBothImg = 0;
int InActiveImg = 0, ImgCount = IMAGE_1;
int NoOfImg = 0;
int FlashFMH = 0, DualFMH = 0;
INT8U ModuleNameRom[15][15];
INT8U ModuleNameFirm[15][15];
INT8U DispModuleNameFirm[15][15];
INT8U DispModuleNameRom[15][15];
INT8U DualDispModuleNameFirm[15][15];
INT8U Modulechklist[15][15];
int ModCount = 0;
int FwUploadImg = 0;
int ImageDiffer1 = 0;
int ImageDiffer2 = 0;
int DualImageSup = FALSE, SignedImageSup = FALSE, CommonConf = TRUE;
int SkipFreeArea = 0, DualImageReq;
INT16U BootVarsCount = 0, BootOnly = 0;
int IsMMCUpdated = 0;
int DOMMCUpdate = 0;
unsigned long MMCAddofAllocMem = 0;
char bufArr_MMC[MAX_SIZE_TO_READ_MMC]; // memory for buf

#if defined(__x86_64__) || defined(WIN64)
unsigned int EraseBlkSize = 0, FlashSize = 0;
#else
unsigned long EraseBlkSize = 0, FlashSize = 0;
#endif
char device[46];
INT8U byAuthType = 0x00;
INT8U byMedium = 0;
int SPIDevice = 0;
int IsRunLevelON = 0;
int ActiveImg = 0;
extern int Mode;
unsigned int firmwareLength[MAX_COMPONENT];
unsigned int firmwareLength1 = 0;
INT8U IsBootComponentPresent = 0;
INT8U IsAppComponentPresent = 0;
INT8U IsMMCComponenttPresent = 0;
INT8U IsValidHPMImage = 0;
INT8U InvalidComponentFound = 0;
INT8U ValidCoponentFound = 0;
int extflag = 0;
#define EXTLOGPARAM 13

extern INT16U ECFPercent;
extern INT16U VerifyPercent;
extern INT16U EFStar;
extern INT16U Progressdis;

extern int RecoveryFlashMode;
extern int ActivateOnly;

extern int WarningMessage();
extern int CommandLineParsing(int argc, char *argv[]);
#if defined(__x86_64__) || defined(WIN64)
extern int VerifyImageChecksum(unsigned int NewImageSize, char *Buffer, int ModuleCount);
#else
extern int VerifyImageChecksum(unsigned long NewImageSize, char *Buffer, int ModuleCount);
#endif

#ifndef MSDOS
const ERRNO_STR GetString[] = {
    /* String */ /* Error Number*/
    {ERR_INVCREDENTIAL, SC_UNAUTHORISED_NAME},
    {ERR_INVCREDENTIAL, LIBIPMI_E_AUTH_CODE_INVALID},
    {ERR_INV_ROLE, SC_INV_ROLE}};
#endif

#ifdef ICC_OS_WINDOWS //    Windows specific driver code

/* windows version define */
#define AMIFLDRV_DRIVER_NAME "UCORE"
#define DRV_BUFFER_HEADER 0x10
typedef void *HANDLE;

HANDLE m_hD = NULL;
TCHAR strDriverName[12];
const TCHAR strDriverFileName[] = "UCORE";
extern BYTE NTDrv_Buffer[1024 * 20];
extern BYTE W64Drv_Buffer[1024 * 20];
extern BYTE Win98Drv_Buffer[1024 * 20];

BYTE NTDrv_Buffer[1024 * 20] = {'#', 'I', 'O', 'D', 0, 0, 0, 0};    // Referenced by FLASH_LoadDriver, GUI.CPP
BYTE W64Drv_Buffer[1024 * 20] = {'#', 'M', 'M', 'D', 0, 0, 0, 0};   // Referenced by FLASH_LoadDriver, GUI.CPP
BYTE Win98Drv_Buffer[1024 * 20] = {'#', 'W', 'W', 'D', 0, 0, 0, 0}; // Referenced by FLASH_LoadDriver, GUI.CPP
char g_szExtractPath[MAX_PATH];
typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

/* Determines whether the specified process is running under WIN64. */
BOOL IsWow64()
{
    BOOL bIsWow64 = FALSE;
    LPFN_ISWOW64PROCESS fnIsWow64Process;

    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
        {
            // handle error
        }
    }
    return bIsWow64;
}

/*
 * @fn FLASH_LoadDriver (Windows Version)
 * @Driver related function
 */
BOOL FLASH_LoadDriver(/*HANDLE hDevice*/)
{
    TCHAR szDrvPath[MAX_PATH];
    TCHAR strVxD[MAX_PATH];
    HANDLE hDevice;
    BOOL bIsWow64 = FALSE;

#if WIN64
    bIsWow64 = 1;
// #else
//	bIsWow64 = IsWow64();
#endif
    /* at this point, there are 3 possibilities:
       1. drivers are not embedded.
           2. drivers are embedded but failed writting to disk(WinPE)
           3. drivers are embedded and extracted to WinTempDir/CurDir/(PATH=)
              in case 1 or 2, we let program continue with the hope that driver may
              exist in current directory.    */
    if (*g_szExtractPath != NULL)
    {                                       // driver extracted?
        strcpy(szDrvPath, g_szExtractPath); // load driver from g_szExtractPath
    }
    else
    {
        // load driver from current dir.
        GetCurrentDirectory(MAX_PATH, szDrvPath);
        if (szDrvPath[strlen(szDrvPath) - 1] != '\\')
            strcat(szDrvPath, "\\");
    }
    strcat(szDrvPath, strDriverFileName /* "UCORE" */); // append "UCORE" to path

    /* At this point szDrvPath = "Drv:\Path\UCORE" */
    if (bIsWow64)
    {
        strcat(szDrvPath, "W64.SYS");
        strcpy(strDriverName, "UCOREW64");
    }
    else
    {
        strcat(szDrvPath, "SYS.SYS");
        strcat(strDriverName, "UCORESYS");
    }
    UnloadDeviceDriver(strDriverName /*"UCORESYS"*/);
    //        __DBG(printf("LoadDeviceDriver\n\tName=%s\n\tPath=%s)\n\tstart\n", strDriverName, szDrvPath);)
    if (!LoadDeviceDriver(strDriverName /*"UCORESYS"*/, szDrvPath, &hDevice))
        return FALSE;
    m_hD = hDevice;
    return TRUE;
}

BOOL FLASH_UnloadDriver()
{

    if (!m_hD)
        return TRUE;
    CloseHandle(m_hD);
    return UnloadDeviceDriver(strDriverName);
}

/****************************************************************************
 *
 *    FUNCTION: LoadDeviceDriver( const TCHAR, const TCHAR, HANDLE *)
 *
 *    PURPOSE: Registers a driver with the system configuration manager
 *        and then loads it.
 *
 *****************************************************************************/
BOOL LoadDeviceDriver(const TCHAR *Name, const TCHAR *Path, HANDLE *lphDevice)
{
    SC_HANDLE schSCManager;
    BOOL okay;

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    // Remove previous instance
    RemoveDriver(schSCManager, Name);

    // Ignore success of installation: it may already be installed.
    InstallDriver(schSCManager, Name, Path);

    // Ignore success of start: it may already be started.
    StartDriver(schSCManager, Name);

    // Do make sure we can open it.
    okay = OpenDevice(Name, lphDevice);

    CloseServiceHandle(schSCManager);

    return okay;
}
/****************************************************************************
 * *
 * *    FUNCTION: UnloadDeviceDriver( const TCHAR *)
 * *
 * *    PURPOSE: Stops the driver and has the configuration manager unload it.
 * *
 * ****************************************************************************/
BOOL UnloadDeviceDriver(const TCHAR *Name)
{
    SC_HANDLE schSCManager;
    BOOL okay;

    schSCManager = OpenSCManager(NULL,                 // machine (NULL == local)
                                 NULL,                 // database (NULL == default)
                                 SC_MANAGER_ALL_ACCESS // access required
    );
    StopDriver(schSCManager, Name);
    okay = RemoveDriver(schSCManager, Name);

    CloseServiceHandle(schSCManager);

    return okay;
}
/****************************************************************************
 * *
 * *    FUNCTION: InstallDriver( IN SC_HANDLE, IN LPCTSTR, IN LPCTSTR)
 * *
 * *    PURPOSE: Creates a driver service.
 * *
 * ****************************************************************************/
BOOL InstallDriver(IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName, IN LPCTSTR ServiceExe)
{
    SC_HANDLE schService;

    // NOTE: This creates an entry for a standalone driver. If this
    // is modified for use with a driver that requires a Tag,
    // Group, and/or Dependencies, it may be necessary to
    // query the registry for existing driver information
    // (in order to determine a unique Tag, etc.).

    schService = CreateService(SchSCManager,          // SCManager database
                               DriverName,            // name of service
                               DriverName,            // name to display
                               SERVICE_ALL_ACCESS,    // desired access
                               SERVICE_KERNEL_DRIVER, // service type
                               SERVICE_DEMAND_START,  // start type
                               SERVICE_ERROR_NORMAL,  // error control type
                               ServiceExe,            // service's binary
                               NULL,                  // no load ordering group
                               NULL,                  // no tag identifier
                               NULL,                  // no dependencies
                               NULL,                  // LocalSystem account
                               NULL                   // no password
    );
    // printf("CreateService %x\n", schService);
    if (schService == NULL)
        return FALSE;

    CloseServiceHandle(schService);

    return TRUE;
}
/****************************************************************************
 * *
 * *    FUNCTION: StartDriver( IN SC_HANDLE, IN LPCTSTR)
 * *
 * *    PURPOSE: Starts the driver service.
 * *
 * ****************************************************************************/
BOOL StartDriver(IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName)
{
    SC_HANDLE schService;
    BOOL ret;
    BOOL ret1;

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS);
    // printf("OpenService %x\n", schService);
    if (schService == NULL)
        return FALSE;

    ret = (ret1 = StartService(schService, 0, NULL)) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING;
    // printf("StartService %x\n", ret1);
    // printf("StartService+GetLastError %x\n", ret);

    CloseServiceHandle(schService);

    return ret;
}
/****************************************************************************
 * *
 * *    FUNCTION: OpenDevice( IN LPCTSTR, HANDLE *)
 * *
 * *    PURPOSE: Opens the device and returns a handle if desired.
 * *
 * ****************************************************************************/
BOOL OpenDevice(IN LPCTSTR DriverName, HANDLE *lphDevice)
{
    TCHAR completeDeviceName[64];
    HANDLE hDevice;

    //
    // Create a \\.\XXX device name that CreateFile can use
    // NOTE: We're making an assumption here that the driver
    // has created a symbolic link using it's own name
    //(i.e. if the driver has the name "XXX" we assume
    // that it used IoCreateSymbolicLink to create a
    // symbolic link "\DosDevices\XXX". Usually, there
    // is this understanding between related apps/drivers.
    //
    // An application might also peruse the DEVICEMAP
    // section of the registry, or use the QueryDosDevice
    // API to enumerate the existing symbolic links in the
    // system.
    //
    wsprintf(completeDeviceName, TEXT("\\\\.\\%s"), DriverName);
    hDevice = CreateFile(completeDeviceName,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    // printf("CreateFile %x\n", hDevice);
    if (hDevice == ((HANDLE)-1))
        return FALSE;

    // If user wants handle, give it to them.  Otherwise, just close it.
    if (lphDevice)
        *lphDevice = hDevice;
    else
        CloseHandle(hDevice);

    return TRUE;
}
/****************************************************************************
 * *
 * *    FUNCTION: StopDriver( IN SC_HANDLE, IN LPCTSTR)
 * *
 * *    PURPOSE: Has the configuration manager stop the driver (unload it)
 * *
 * ****************************************************************************/
BOOL StopDriver(IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName)
{
    SC_HANDLE schService;
    BOOL ret;
    SERVICE_STATUS serviceStatus;

    schService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS);
    // printf("OpenService %x\n", schService);
    if (schService == NULL)
        return FALSE;

    ret = ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
    // printf("ControlService %x\n", ret);
    CloseServiceHandle(schService);

    return ret;
}
/****************************************************************************
 * *
 * *    FUNCTION: RemoveDriver( IN SC_HANDLE, IN LPCTSTR)
 * *
 * *    PURPOSE: Deletes the driver service.
 * *
 * ****************************************************************************/
BOOL RemoveDriver(IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName)
{
    SC_HANDLE schService;
    BOOL ret;
    long counter;

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS);
    // printf("OpenService %x\n", schService);
    if (schService == NULL)
        return FALSE;

    ret = DeleteService(schService);
    // printf("DeleteService %x\n", ret);
    CloseServiceHandle(schService);

    for (counter = 0; counter < 4096; counter++)
    {
        if (schService = OpenService(SchSCManager,
                                     DriverName,
                                     SERVICE_ALL_ACCESS))
            CloseServiceHandle(schService);
        else
            break;
    }
    // printf("counter %lx\n", counter);
    return ret;
}

#endif

#ifdef ICC_OS_LINUX
void CheckAndUnloadIPMIDriver()
{
    int DrvCount = 0, Display = 0;
    char Buffer[MAX_MODULE_NAME] = {0}, ModuleName[MAX_MODULE_NAME] = {0};
    FILE *Drv = NULL;

    if (byMedium != KCS_MEDIUM)
    {
        return;
    }

    for (DrvCount = 0; DrvCount < sizeof(OpenIPMIDriver) / sizeof(OpenIPMIDrv_T); DrvCount++)
    {
        memset(Buffer, 0, sizeof(Buffer));

        sprintf(Buffer, "lsmod|grep %s", OpenIPMIDriver[DrvCount].ModuleName);

        Drv = popen(Buffer, "r");
        if (Drv == NULL)
        {
            OpenIPMIDriver[DrvCount].LoadModule = 0;
            continue;
        }

        memset(ModuleName, 0, sizeof(ModuleName));

        fscanf(Drv, "%64s", ModuleName);

        if ((memcmp(ModuleName, OpenIPMIDriver[DrvCount].ModuleName, strlen(ModuleName)) == 0) && (strlen(ModuleName) != 0))
        {
            memset(Buffer, 0, sizeof(Buffer));

            if (Display == 0)
            {
                printf("-------------------------------------------------\n");
                printf("Open IPMI Drivers\n");
                printf("-------------------------------------------------\n");
            }

            printf("Un-Loading %s\n", OpenIPMIDriver[DrvCount].ModuleName);

            sprintf(Buffer, "rmmod %s", OpenIPMIDriver[DrvCount].ModuleName);
            system(Buffer);
            OpenIPMIDriver[DrvCount].LoadModule = 1;
            Display++;
        }

        pclose(Drv);
    }
}

void LoadOpenIPMIDrivers()
{
    INT8U DrvCount = 0, Display = 0;
    char Buffer[MAX_MODULE_NAME] = {0};

    if (byMedium != KCS_MEDIUM)
    {
        return;
    }

    for (DrvCount = 0; DrvCount < sizeof(OpenIPMIDriver) / sizeof(OpenIPMIDrv_T); DrvCount++)
    {
        if (OpenIPMIDriver[DrvCount].LoadModule == 1)
        {
            memset(Buffer, 0, sizeof(Buffer));

            if (Display == 0)
            {
                printf("-------------------------------------------------\n");
                printf("Open IPMI Drivers\n");
                printf("-------------------------------------------------\n");
            }

            if ((memcmp(OpenIPMIDriver[DrvCount].ModuleName, "ipmi_si", strlen(OpenIPMIDriver[DrvCount].ModuleName)) == 0) && (FlashSelected == 1) && (Parsing.RebootFirmware == 1))
            {
                printf("BMC is booting up, Please load ipmi_si Open IPMI Driver after the BMC Boots up!!!!\n");
                Display++;
                continue;
            }

            sprintf(Buffer, "modprobe %s", OpenIPMIDriver[DrvCount].ModuleName);

            printf("Loading Open IPMI Driver:%s\n", OpenIPMIDriver[DrvCount].ModuleName);

            system(Buffer);
            Display++;
        }
    }
}
#endif
/*
 * @fn printerror
 * @brief This Function is to display appropriate ERROR message
 * @param wErr - Error Value
 * @return Returns 0 on success
 */

void printerror(unsigned short wErr)
{
#ifndef MSDOS
    int i;
    if (wErr != LIBIPMI_E_SUCCESS)
    {
        if (IS_MEDIUM_ERROR(wErr))
            printf("Medium Error ");
        else
        {
            for (i = 0; i < sizeof(GetString) / 2; i++)
            {
                if (GetString[i].ErrNo == GET_ERROR_CODE(wErr))
                {
                    printf("%s", GetString[i].String);
                    break;
                }
            }
            return;
        }
        printf("%x\n", GET_ERROR_CODE(wErr));
    }
#endif
}
/*
 * @fn Print_copyrights
 * @brief this Function is to display the copyrights of Yafu
 * @return Returns 0 on success
 */
void Print_copyrights()
{
    printf("\n");
    printf("-------------------------------------------------\n");
    printf("YAFUFlash - Firmware Upgrade Utility (Version %s)\n", VERSION_NUMBER);
    printf("-------------------------------------------------\n");
    printf("(C)Copyright 2023, HXZY\n");
}
/*
 * @fn ResetFunction
 * @brief This Function is to reset the firmware
 * @param hSession - Current Session Pointer
 * @return Returns 0 on success
 */
int ResetFunction(IPMI20_SESSION_T *hSession)
{

    INT16U WaitTime = 0x02;

    FlashSelected = 1;
    if (Parsing.ConfigPreserve == 1)
        WaitTime = 0x00;

    printf("\rResetting the firmware..........\n");
    ResetDevice(hSession, WaitTime);
    return 0;
}

/*
 * @fn BuildModuleMap
 * @brief This Function is to Bulid the Map of the Modules in New image
 * @return Returns 0 on success
 */

int BuildModuleMap()
{
    int c = 0, j = 0;
    DWORD i = 0, l = 0, size = 64;
    unsigned char FMHSignature[9];
    INT32U ModuleLocation = 0, ModuleSize = 0;

    for (i = 0; i <= NewImageSize; i += 64)
    {

        for (j = 0, l = i; j < 8; j++, l++)
            FMHSignature[j] = Buffer[l];

        FMHSignature[j] = '\0';

        if (spstrcasecmp((char *)FMHSignature, FMH_SIGNATURE) == 0) // check FMH Signature
        {
            if (0 != fseek(fp, i, SEEK_SET))
            {
                printf("Error in fseek\n");
                return -1;
            }

#if defined(__x86_64__) || defined(_WIN64)
            FwModuleHdr[c] = (FlashMH *)malloc(sizeof(FlashMH));
#else
            FwModuleHdr[c] = (FlashMH *)malloc(64 * (sizeof(char)));
#endif
            if (FwModuleHdr[c] == NULL)
                return -1;

#if defined(__x86_64__) || defined(_WIN64)
            memset(FwModuleHdr[c], 0, sizeof(FwModuleHdr[c]));
#else
            memset(FwModuleHdr[c], 0, sizeof(FwModuleHdr));
#endif

            if (size != fread(FwModuleHdr[c], 1, size, fp))
            {
                free(FwModuleHdr[c]);
                return -1;
            }

            if (FwModuleHdr[c]->ModuleType == MODULE_FMH_FIRMWARE) // Firmware Module
            {
                if (((FwModuleHdr[c]->Fmh_Version.FwVersion & 0x00FF) == FMH_MAJOR) &&
                    (((FwModuleHdr[c]->Fmh_Version.FwVersion & 0xFF00) >> 8) <= FMH_MINOR))
                {
                    ModuleLocation = FwModuleHdr[c]->ModuleLocation + FwModuleHdr[c]->FmhLocation;
                }
                else
                {
                    ModuleLocation = FwModuleHdr[c]->ModuleLocation;
                }

                ModuleSize = FwModuleHdr[c]->ModuleSize;

                GetReleaseaandCodeBaseVersion(Buffer + ModuleLocation, ModuleSize, "FW_RELEASEID=", UpReleaseID);
                GetReleaseaandCodeBaseVersion(Buffer + ModuleLocation, ModuleSize, "FW_CODEBASEVERSION=", UpCodeBaseVersionID);
            }

            if (FwModuleHdr[c]->EndSignature == FMH_END_SIGNATURE)
            {
                FwModuleHdr[c]->FmhLocation = i;
                c++;
            }
            else
                free(FwModuleHdr[c]);
        }
    }

    ModuleCount = c;

    return 0;
}
/*
 * @fn GetImageSize
 * @brief this function is used to get the image size
 * @return Returns 0 on success
 */
int GetImageSize(char *imgName)
{
#if defined(__x86_64__) || defined(WIN64)
    unsigned int len = 0;
#else
    unsigned long len = 0;
#endif

    fp = fopen(imgName, "rb");
    if (fp == NULL)
    {
        printf("Image file (%s) could not be opened or not present.\n", imgName);
        exit(YAFU_FILE_OPEN_ERR);
    }
    else
    {
        if (fseek(fp, 0, SEEK_END))
        {
            fclose(fp);
            return -1;
        }
        else
        {
            len = ftell(fp);
            NewImageSize = len;
            if (len == -1)
            {
                printf("ftell returned error: %s\n", strerror(errno));
                fclose(fp);
                return -1;
            }
            if (NewImageSize == 0)
            {
                printf("WARNING!! No data in the Image File\nInvalid Image File!!\n");
                exit(YAFU_CC_IMAGE_SIZE_INVALID);
            }
        }
    }

    return 0;
}

/*
 * @fn SaveFwFile
 * @brief this fuction is to Save the Firmware file in to buffer
 * @return Returns 0 on success
 */
int SaveFwFile()
{
    if (0 != fseek(fp, 0, SEEK_SET))
        return -1;
    Buffer = (INT8U *)malloc(NewImageSize);
    if (Buffer == NULL)
        return -1;
    memset(Buffer, 0, NewImageSize);
    if (NewImageSize != fread(Buffer, 1, NewImageSize, fp))
    {
        free(Buffer);
        return -1;
    }
    if (BuildModuleMap() != 0)
    {
        free(Buffer);
        return -1;
    }
    return 0;
}
/*
 * @fn SpaceAlign
 * @brief this Function is to align the space while display firmware info
 * @return Returns 0 on success
 */
int SpaceAlign(int space)
{
    int Count;
    char Nullchar = 32;
    for (Count = 0; Count < space; Count++)
        printf("%c", Nullchar);
    return 0;
}

/*
 ** @fn DisplayFirmwareVersion
 ** @brief this function displays the Firmware Version from INI
 ** @return Returns 0 on success
 ** @       Return -1 on failure
 **/
int DisplayFirmwareVersion(IPMI20_SESSION_T *hSession)
{
    AMIGetFwVersionRes_T *pAMIGetFwVersionRes;
    FwVersionRes_T *FwInfo = NULL;
    INT8U *pFMHVersion = NULL;
    char ModuleName[10];
    int Ret = 0, i = 0, j = 0, count = 1;

    pFMHVersion = malloc(MAX_FMHLENGTH);
    if (pFMHVersion == NULL)
        return -1;

    pAMIGetFwVersionRes = (AMIGetFwVersionRes_T *)pFMHVersion;
    if ((Ret = GetFirmwareVersion(hSession, pFMHVersion)) != 0)
    {
        printf("Error:GetFirmwareVersion\n");
        goto exit;
    }

    printf("=======================================\n");
    printf("           Firmware Details \n");
    printf("=======================================\n");
    printf("  ModuleName   Description   Version    Device\n");
    for (i = 0; i < pAMIGetFwVersionRes->Count; i++)
    {
        FwInfo = (FwVersionRes_T *)malloc(sizeof(FwVersionRes_T));
        if (FwInfo == NULL)
        {
            printf("Error malloc FwInfo \n");
            return -1;
        }
        memset((char *)FwInfo, 0, sizeof(FwVersionRes_T));
        memcpy(FwInfo, pFMHVersion + j + 2, sizeof(FwVersionRes_T));

        strncpy((char *)ModuleName, (char *)FwInfo->Name, 8);
        ModuleName[8] = (char)'\0';
#ifdef CONFIG_SPX_FEATURE_YAFUFLASH_FLASH_UBOOT_ON_ACTIVE_IMAGE
        if ((FwInfo->Device == 2) && (spstrcasecmp(ModuleName, "boot_img") == 0))
        {
            j += sizeof(FwVersionRes_T);
            if (FwInfo != NULL)
                free(FwInfo);
            continue;
        }
#endif

        printf("%d.", count);
        if (strlen((char *)FwInfo->Name) >= 8)
        {
            printf("%s", ModuleName);
#if defined(__x86_64__) || defined(_WIN64)
            SpaceAlign(13 - (unsigned)strlen(ModuleName));
#else
            SpaceAlign(13 - strlen(ModuleName));
#endif
        }
        else
        {
            printf("%s", FwInfo->Name);
#if defined(__x86_64__) || defined(_WIN64)
            SpaceAlign(13 - (unsigned)strlen(FwInfo->Name));
#else
            SpaceAlign(13 - strlen(FwInfo->Name));
#endif
        }

#ifdef CONFIG_SPX_FEATURE_YAFUFLASH_FLASH_UBOOT_ON_ACTIVE_IMAGE
        if (spstrcasecmp(ModuleName, "boot_img") == 0)
        {
            printf("%s ", ACTIVE);
            goto next;
        }
#endif

        if (FwInfo->Instance == 1)
            printf("%s ", ACTIVE);
        else
            printf("%s ", BACKUP);
    next:
        SpaceAlign(7);
        if ((FwInfo->Major == 0x2d) && (FwInfo->Minor == 0x2d))
        {
            if (FMH_MAJOR >= 1 && FMH_MINOR >= 6)
            {
                printf("%c.%c.%c%c ", '-', '-', '-', '-');
            }
            else
            {
                printf("%c.%c ", '-', '-');
            }
        }
        else if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
        {
            printf("%d.%d.%.2s ", FwInfo->Major, FwInfo->Minor, FwInfo->AuxVer);
        }
        else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
        {
            printf("%d.%d.%.6s ", FwInfo->Major, FwInfo->Minor, FwInfo->AuxVer);
        }
        else
        {
            printf("%d.%d ", FwInfo->Major, FwInfo->Minor);
        }
        printf("\tImage %d", FwInfo->Device);
        printf("\n");

        j += sizeof(FwVersionRes_T);
        count++;

        if (FwInfo != NULL)
        {
            free(FwInfo);
            FwInfo = NULL;
        }
    }

exit:
    if (pFMHVersion != NULL)
        free(pFMHVersion);

    return Ret;
}

/*
 * @fn DisplayExistingFirmwareInfo
 * @brief this function display the information of ExistingImage from Flash
 * @return Returns 0 on success
 */
int DisplayExistingFirmwareInfo(int AltFlag)
{
    int i = 0;
    char ModuleName[10];
    int confflag = 0;

    printf("=======================================\n");
    printf("           Firmware Details \n");
    printf("=======================================\n");
    if (Parsing.ImgInfo == 1)
    {
        printf("       Existing Image from Flash \n\n");
    }
    else
    {
        printf("              Image Version\n");
    }
    if (AltFlag == 0)
    {
        printf("  ModuleName   Description   Version\n");

        for (i = 0; i < ModuleCount; i++)
        {
            printf("%d.", i + 1);
            if (strlen((char *)FwModuleHdr[i]->ModuleName) >= 8)
            {
                strncpy((char *)ModuleName, (char *)FwModuleHdr[i]->ModuleName, 8);
                ModuleName[8] = (char)'\0';
                printf("%s", ModuleName);
#if defined(__x86_64__) || defined(_WIN64)
                SpaceAlign(13 - (unsigned)strlen(ModuleName));
#else
                SpaceAlign(13 - strlen(ModuleName));
#endif
            }
            else
            {
                if (strcmp(FwModuleHdr[i]->ModuleName, "conf") == 0)
                    confflag++;

                if (confflag == 2)
                {
                    printf("%s", "bkupconf");
                    confflag = 0;
                    SpaceAlign(13 - (unsigned)strlen("bkupconf"));
                }
                else
                {
                    printf("%s", FwModuleHdr[i]->ModuleName);
#if defined(__x86_64__) || defined(_WIN64)
                    SpaceAlign(13 - (unsigned)strlen(FwModuleHdr[i]->ModuleName));
#else
                    SpaceAlign(13 - strlen(FwModuleHdr[i]->ModuleName));
#endif
                }
            }
            printf("%s ", ModuleNameRom[i]);
#if defined(__x86_64__) || defined(_WIN64)
            SpaceAlign(13 - (unsigned)strlen(ModuleNameRom[i]));
#else
            SpaceAlign(13 - strlen(ModuleNameRom[i]));
#endif
            if ((FwModuleHdr[i]->Module_Version.ModMajorVer == 0x2d) && (FwModuleHdr[i]->Module_Version.ModMinorVer == 0x2d))
            {
                if (FMH_MAJOR >= 1 && FMH_MINOR >= 6)
                {
                    printf("%c.%c.%c%c ", '-', '-', '-', '-');
                }
                else
                {
                    printf("%c.%c ", '-', '-');
                }
            }
            else if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
            {
                printf("%d.%d.%.2s ", FwModuleHdr[i]->Module_Version.ModMajorVer, *(&(FwModuleHdr[i]->Module_Version.ModMinorVer) + 1), FwModuleHdr[i]->ModuleAuxVer);
            }
            else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
            {
                printf("%d.%d.%.6s ", FwModuleHdr[i]->Module_Version.ModMajorVer, *(&(FwModuleHdr[i]->Module_Version.ModMinorVer) + 1), FwModuleHdr[i]->ModuleAuxVer);
            }
            else
            {
                printf("%d.%d ", FwModuleHdr[i]->Module_Version.ModMajorVer, *(&(FwModuleHdr[i]->Module_Version.ModMinorVer) + 1));
            }
            printf("\n");
        }
    }
    else
    {
        printf("ConfigImageAddr = 0x%X\n", ConfigRomAddr);
        printf("ConfigImageSize = 0x%X\n", ConfigRomSize);
        printf("ConfigImageModLoc = 0x%X\n", ConfigRomModLoc);
        printf("BootImageModLoc = 0x%X\n", BootRomModLoc);
        printf("BootImageAddr = 0x%X\n", BootRomAddr);
        printf("BootImageSize = 0x%X\n", BootRomSize);
        printf("BootImageAllocAddr = 0x%X\n", BootRomAllocAddr);
    }
    return 0;
}

/*
 * @fn DisplayFirmwareInfo
 * @brief this function display the information about RomImage and ExistingImage from Flash
 * @return Returns 0 on success
 */
int DisplayFirmwareInfo()
{
    int i;
    char ModuleName[10];
    int confflag = 0;

    if (Parsing.ImgInfo == 0)
    {

        if (DualImageSup == FALSE)
        {
            printf("\r===============================================================================\n");
            printf("                             Firmware Details \n");
            printf("===============================================================================\n");
            printf("                               RomImage         ExistingImage \n\n");
            printf("    ModuleName   Description   Version          Version\n");
        }
        else if (DualImageSup == TRUE && DualFMH == 0)
        {
            printf("===============================================================================\n");
            printf("                             Firmware Details \n");
            printf("===============================================================================\n");
            if (ActiveImg == 1)
            {
                printf("                               RomImage        Image 1\n\n");
            }
            else
            {
                printf("                               RomImage        Image 2\n\n");
            }
            printf("    ModuleName   Description   Version         Version\n");
        }
        else
        {
            printf("===============================================================================\n");
            printf("                             Firmware Details \n");
            printf("===============================================================================\n");
            if (((FlashBothImg == TRUE) && (FwUploadImg == 2)) || ((ActiveCurImg == 2) && (FlashBothImg == FALSE)))
            {
                printf("                               RomImage    Image 2     Image 1\n\n");
            }
            else
            {
                printf("                               RomImage    Image 1     Image 2\n\n");
            }
            printf("    ModuleName   Description   Version     Version     Version\n");
        }
    }
    else if ((Parsing.ImgInfo == 1) || (Parsing.ImgInfo == 2))
    {
        printf("===============================================================================\n");
        printf("                             Firmware Details \n");
        printf("===============================================================================\n");
        if (Parsing.ImgInfo == 1)
        {
            printf("                               Image 1     Image 2 \n\n");
        }
        else
        {
            printf("                               Image 1     Image 2\n");
            printf("                                Version     Version\n\n");
        }
        printf("    ModuleName   Description   Version     Version\n");
    }

    for (i = 0; i < DispCount; i++)
    {
        printf("%d.  ", i + 1);
        if (strlen((char *)DispFwmodHdr[i]->ModuleName) >= 8)
        {
            strncpy((char *)ModuleName, (char *)DispFwmodHdr[i]->ModuleName, 8);
            ModuleName[8] = (char)'\0';
            printf("%s", ModuleName);
            // SpaceAlign((int)(13 - strlen(ModuleName)));
#if defined(__x86_64__) || defined(_WIN64)
            SpaceAlign(13 - (unsigned)strlen(ModuleName));
#else
            SpaceAlign(13 - strlen(ModuleName));
#endif
        }
        else
        {
            if (strcmp(DispFwmodHdr[i]->ModuleName, "conf") == 0)
                confflag++;

            if (confflag == 2)
            {
                printf("%s", "bkupconf");
                confflag = 0;
                SpaceAlign(13 - strlen("bkupconf"));
            }
            else
            {
                printf("%s", DispFwmodHdr[i]->ModuleName);
                //          SpaceAlign((int)(13 - strlen((char*)DispFwmodHdr[i]->ModuleName)));
#if defined(__x86_64__) || defined(_WIN64)
                SpaceAlign(13 - (unsigned)strlen(DispFwmodHdr[i]->ModuleName));
#else
                SpaceAlign(13 - strlen(DispFwmodHdr[i]->ModuleName));
#endif
            }
        }

        printf("%s ", DispModuleNameRom[i]);
        //   SpaceAlign((int)(13 - strlen((char*)DispModuleNameRom[i])));
#if defined(__x86_64__) || defined(_WIN64)
        SpaceAlign(13 - (unsigned)strlen(DispModuleNameRom[i]));
#else
        SpaceAlign(13 - strlen(DispModuleNameRom[i]));
#endif
        if ((DispFwmodHdr[i]->Module_Version.ModMajorVer == 0x2d) && (DispFwmodHdr[i]->Module_Version.ModMinorVer == 0x2d))
        {
            if (FMH_MAJOR >= 1 && FMH_MINOR >= 6)
            {
                printf("%c.%c.%c%c ", '-', '-', '-', '-');
            }
            else
            {
                printf("%c.%c ", '-', '-');
            }
        }
        else if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
            printf("%d.%d.%.2s ", DispFwmodHdr[i]->Module_Version.ModMajorVer, *(&(DispFwmodHdr[i]->Module_Version.ModMinorVer) + 1), DispFwmodHdr[i]->ModuleAuxVer);
        else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
            printf("%d.%d.%.06d ", DispFwmodHdr[i]->Module_Version.ModMajorVer, *(&(DispFwmodHdr[i]->Module_Version.ModMinorVer) + 1), atoi(DispFwmodHdr[i]->ModuleAuxVer));
        else
            printf("%d.%d ", DispFwmodHdr[i]->Module_Version.ModMajorVer, *(&(DispFwmodHdr[i]->Module_Version.ModMinorVer) + 1));

        SpaceAlign(5);

        if ((DispCurrFwModHdr[i]->Module_Version.ModMajorVer == 0x2d) && (DispCurrFwModHdr[i]->Module_Version.ModMinorVer == 0x2d))
        {
            if (FMH_MAJOR >= 1 && FMH_MINOR >= 6)
            {
                printf("%c.%c.%c%c ", '-', '-', '-', '-');
            }
            else
            {
                printf("%c.%c", '-', '-');
            }
        }
        else if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
            printf("%d.%d.%.2s", DispCurrFwModHdr[i]->Module_Version.ModMajorVer, *(&(DispCurrFwModHdr[i]->Module_Version.ModMinorVer) + 1), DispCurrFwModHdr[i]->ModuleAuxVer);
        else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
            printf("%d.%d.%.06d ", DispCurrFwModHdr[i]->Module_Version.ModMajorVer, *(&(DispCurrFwModHdr[i]->Module_Version.ModMinorVer) + 1), atoi(DispCurrFwModHdr[i]->ModuleAuxVer));
        else
            printf("%d.%d ", DispCurrFwModHdr[i]->Module_Version.ModMajorVer, *(&(DispCurrFwModHdr[i]->Module_Version.ModMinorVer) + 1));

        if (DualFMH != 0)
        {
            SpaceAlign(6);

            if ((DispDualFwmodHdr[i]->Module_Version.ModMajorVer == 0x2d) && (DispDualFwmodHdr[i]->Module_Version.ModMinorVer == 0x2d))
            {
                if (FMH_MAJOR >= 1 && FMH_MINOR >= 6)
                {
                    printf("%c.%c.%c%c \n", '-', '-', '-', '-');
                }
                else
                {
                    printf("%c.%c \n", '-', '-');
                }
            }
            else if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
                printf("%d.%d.%.2s \n", DispDualFwmodHdr[i]->Module_Version.ModMajorVer, *(&(DispDualFwmodHdr[i]->Module_Version.ModMinorVer) + 1), DispDualFwmodHdr[i]->ModuleAuxVer);
            else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
                printf("%d.%d.%.6s \n", DispDualFwmodHdr[i]->Module_Version.ModMajorVer, *(&(DispDualFwmodHdr[i]->Module_Version.ModMinorVer) + 1), DispDualFwmodHdr[i]->ModuleAuxVer);
            else
                printf("%d.%d \n", DispDualFwmodHdr[i]->Module_Version.ModMajorVer, *(&(DispDualFwmodHdr[i]->Module_Version.ModMinorVer) + 1));
        }
        else
            printf("\n");
    }

    return 0;
}
/*
 * @fn DisplayAltFirmwareInfo
 * @brief this function is to display the Alternate Information about RomImage and ExistingImage from Flash
 * @return Returns 0 on success
 */
int DisplayAltFirmwareInfo()
{

    // int i;
    // char ModuleName[10];

    printf("===============================================================================\n");
    printf("                             Firmware Details \n");
    printf("===============================================================================\n");
    printf("    RomImage                  ExistingImage from Flash \n\n");

    printf("ConfigRomAddr = 0x%X         ConfigFirmAddr = 0x%X\n", ConfigRomAddr, ConfigFirmAddr);
    printf("ConfigRomSize = 0x%X         ConfigFirmSize = 0x%X\n", ConfigRomSize, ConfigFirmSize);
    printf("ConfigRomModLoc = 0x%X       ConfigFirmModLoc = 0x%X\n", ConfigRomModLoc, ConfigFirmModLoc);
    printf("BootRomModLoc = 0x%X         BootFirmModLoc = 0x%X\n", BootRomModLoc, BootFirmModLoc);
    printf("BootRomAddr = 0x%X           BootFirmAddr = 0x%X\n", BootRomAddr, BootFirmAddr);
    printf("BootRomSize = 0x%X           BootFirmSize = 0x%X\n", BootRomSize, BootFirmSize);
    printf("BootRomAllocAddr = 0x%X      BootFirmAllocAddr = 0x%X\n", BootRomAllocAddr, BootFirmAllocAddr);
    printf("ConfigRomAllocAddr = 0x%X    ConfigFirmAllocAddr = 0x%X\n", ConfigRomAllocAddr, ConfigFirmAllocAddr);

    printf("\n");

    return 0;
}
/*
 * @fn PreserveUbootEnvVariables
 * @brief this fuction Prserve Uboot Env Variables from the firmware
 * @param hSession - Current Session Pointer
 * @param BootVarsName - Boot Variable Names
 * @param BootVarsCount - Count of Boot Variables
 * @return Returns 0 on success
 */
int PreserveUbootEnvVariables(IPMI20_SESSION_T *hSession, char *BootVarsName, INT16U *BootVarsCount)
{

    char *BootVar, *BootVal;
    char valarr[MAX_BOOTVAL_LENGTH]; // memory for BootVal
    char vararr[MAX_BOOTVAR_LENGTH]; // memory for BootVar
    int Count = 0, TotalLen = 0, len = 0, length = 0;

    printf("Preserving Env Variables ...             \r");
    fflush(stdout);
    if (GetAllBootVars(hSession, BootVarsName, BootVarsCount) != 0)
    {
        printf("\nError: GetAllBootVars failed\n");
        fflush(stdout);
        return -1;
    }

    BootVarsName += sizeof(AMIYAFUGetBootVarsRes_T);
    BootVar = vararr;
    memset(BootVar, 0, MAX_BOOTVAR_LENGTH);
    if (vdbg)
    {
        printf("\nBootVarsCount = %d\n", *BootVarsCount);
        fflush(stdout);
    }

    for (Count = 0; Count < *BootVarsCount; Count++)
    {
        strcpy(BootVar, (BootVarsName + TotalLen));

        // len=strlen(BootVar);
#if defined(__x86_64__) || defined(_WIN64)
        len = (unsigned)strlen(BootVar);
#else
        len = (unsigned)strlen(BootVar);
#endif
        TotalLen += (len + 1);
        BootVal = valarr;

        memset(BootVal, 0, MAX_BOOTVAL_LENGTH);
        if (GetBootConfig(hSession, BootVal, BootVar) != 0)
        {
            printf("\nError: GetBootConfig Failed\n");
            fflush(stdout);
            return -1;
        }

        BootVal += sizeof(AMIYAFUGetBootConfigRes_T);

        // length = strlen(BootVal);
#if defined(__x86_64__) || defined(_WIN64)
        length = (unsigned)strlen(BootVal);
#else
        length = strlen(BootVal);
#endif

        memset(BootValues[Count], 0, MAX_UBOOT_VAR_LENGTH);
        memcpy(BootValues[Count], BootVal, (length + 1));
    }

    printf("\rPreserving Env Variables...             done\n"); // fflush(stdout);
    return 0;
}
#if 0
/*
 * @fn SetUbootEnv
 * @brief this fuction Sets Uboot Env Variable values  and retry five times if it fails
 * @param hSession - Current Session Pointer
 * @param BootVarsName -Holds the Boot Variable Names
 * @param BootVarsCount -Holds Count of Boot Variables
 * @return Returns 0 on success
*/
int SetUbootEnv(IPMI20_SESSION_T *hSession,char *BootVarsName,INT16U* BootVarsCount)
{

    char verifyArr[MAX_UBOOT_VAR_COUNT * MAX_UBOOT_VAR_NAME_LENGTH];
    char *EnvironmentVariables;
    int CheckVarsCount = 0;
    int RetryCount_EV = 0;
    int RetVal = 0;
	
    while (1)
    {
        EnvironmentVariables = BootVarsName;
        if(SettingEnvUbootVariables(hSession,EnvironmentVariables,BootVarsCount) != 0 )
        {
            if (vdbg)
                printf("Error in SettingEnvUbootVariables \n");
        }

        // There were instances where SettingEnvUbootVariables() succeeded, but some
        // variables were missing on the SP.  So added a check to read back all
        // ENV variables again to check against count.  If count is mismatched,
        // we will retry upto RETRYCOUNT times, and give up if not successful.
        EnvironmentVariables = verifyArr;
        memset (EnvironmentVariables, 0, sizeof (verifyArr));
        if(GetAllBootVars(hSession,EnvironmentVariables,(unsigned short*)&CheckVarsCount) != 0)
        {
            printf("Error in GetAllBootVars \n");
        }

        if (CheckVarsCount != *BootVarsCount)
        {
            RetryCount_EV += 1;
            if (vdbg) { printf ("Warning: mismatch in env count [%d] vs [%d]\n", *BootVarsCount, CheckVarsCount); 
            fflush (stdout); }
            if (RetryCount_EV > RETRYCOUNT)
            {
                RetVal = -1;
                if (vdbg) { printf ("Error: Retry count exceeded, giving up...\n"); fflush (stdout); }
                break;
            }
            else
            {
                if (vdbg) { printf ("Retry #%d \n", RetryCount_EV); fflush (stdout); }
                continue;
            }
        }
        else
        {
            break;
        }
    } // while (1)

return RetVal;
}
#endif
/*
 * @fn SettingEnvUbootVariables
 * @brief this fuction Sets Uboot Env Variable values
 * @param hSession - Current Session Pointer
 * @param BootVarsName -Holds the Boot Variable Names
 * @param BootVarsCount -Holds Count of Boot Variables
 * @return Returns 0 on success
 */
int SettingEnvUbootVariables(IPMI20_SESSION_T *hSession, char *BootVarsName, INT16U *BootVarsCount)
{
    char *BootVal;
    char *BootVar;
    char valarr[MAX_BOOTVAL_LENGTH]; // memory for BootVal
    char vararr[MAX_BOOTVAR_LENGTH]; // memory for BootVar

    int Count = 0, TotalLen = 0, len = 0;
    TotalLen = 0;
    BootVar = vararr;
    memset(BootVar, 0, MAX_BOOTVAR_LENGTH);
    BootVarsName += sizeof(AMIYAFUGetBootVarsRes_T);
    printf("Setting Env variables ...               \r");
    fflush(stdout);

    if (vdbg)
    {
        printf("\nBootVarsCount = %d\n", *BootVarsCount);
        fflush(stdout);
    }
    for (Count = 0; Count < *BootVarsCount; Count++)
    {
        strcpy(BootVar, (BootVarsName + TotalLen));
#if defined(__x86_64__) || defined(_WIN64)
        len = (unsigned)strlen(BootVar);
#else
        len = strlen(BootVar);
#endif
        TotalLen += (len + 1);

        BootVal = valarr;
        memset(BootVal, 0, MAX_BOOTVAL_LENGTH);
        memcpy(BootVal, BootValues[Count], (strlen(BootValues[Count]) + 1));

        if (SetBootConfig(hSession, BootVar, BootVal) != 0)
        {
            printf("\nError: SetBootConfig failed\n");
            fflush(stdout);
            return -1;
        }
    }
    printf("Setting Env variables...               done\n");
    fflush(stdout);
    return 0;
}
/*
 * @fn DispMemAlloc
 * @brief This function Displays the Memory Allocated
 * @param DispCount - Holds the display count of module
 * @return Returns 0 on success
 */
int DispMemAlloc(int DispCount)
{
    DispFwmodHdr[DispCount] = (FlashMH *)malloc(sizeof(FlashMH));
    if (DispFwmodHdr[DispCount] == NULL)
        return -1;
#if defined(__x86_64__) || defined(_WIN64)
    memset(DispFwmodHdr[DispCount], 0, sizeof(DispFwmodHdr[DispCount]));
#else
    memset(DispFwmodHdr[DispCount], 0, sizeof(DispFwmodHdr));
#endif

    DispCurrFwModHdr[DispCount] = (FlashMH *)malloc(sizeof(FlashMH));
    if (DispCurrFwModHdr[DispCount] == NULL)
        return -1;
#if defined(__x86_64__) || defined(_WIN64)
    memset(DispCurrFwModHdr[DispCount], 0, sizeof(DispCurrFwModHdr[DispCount]));
#else
    memset(DispCurrFwModHdr[DispCount], 0, sizeof(DispCurrFwModHdr));
#endif

    return 0;
}

/*
 * @fn DispDualMemAlloc
 * @brief This function Displays the Memory Allocated
 * @param DispCount - Holds the display count of module
 * @return Returns 0 on success
 */
int DispDualMemAlloc(int DispCount)
{
    DispDualFwmodHdr[DispCount] = (FlashMH *)malloc(sizeof(FlashMH));
    if (DispDualFwmodHdr[DispCount] == NULL)
        return -1;
#if defined(__x86_64__) || defined(_WIN64)
    memset(DispDualFwmodHdr[DispCount], 0, sizeof(DispDualFwmodHdr[DispCount]));
#else
    memset(DispDualFwmodHdr[DispCount], 0, sizeof(DispDualFwmodHdr));
#endif

    return 0;
}

int DisplayDualModuleRearrange()
{
    int i = 0, j = 0, TotalCount = 0;

    for (i = 0; i < DispCount; i++)
    {
        if (DispDualMemAlloc(i) != 0)
        {
            return -1;
        }
        strcpy(DispDualFwmodHdr[i]->ModuleName, DispFwmodHdr[i]->ModuleName);
        for (j = 0; j < DualFMH; j++)
        {
            if (spstrncasecmp(DispFwmodHdr[i]->ModuleName, DualFwModHdr[j]->ModuleName, sizeof(DispFwmodHdr[i]->ModuleName)) == 0)
            {
                if ((spstrcasecmp((char *)DualFwModHdr[j]->ModuleName, "boot")) == 0)
                {
                    strcpy((char *)DualDispModuleNameFirm[i], "BootLoader  ");
                }
                else if ((spstrcasecmp((char *)DualFwModHdr[j]->ModuleName, "conf")) == 0 ||
                         (spstrcasecmp((char *)DualFwModHdr[j]->ModuleName, "params")) == 0)
                {
                    strcpy((char *)DualDispModuleNameFirm[i], "ConfigParams");
                }
                else if (spstrcasecmp((char *)DispFwmodHdr[j]->ModuleName, "root") == 0)
                {
                    strcpy((char *)DualDispModuleNameFirm[i], "Root       ");
                }
                else if (spstrcasecmp((char *)DualFwModHdr[j]->ModuleName, "osimage") == 0)
                {
                    strcpy((char *)DualDispModuleNameFirm[i], "Linux OS    ");
                }
                else if (spstrcasecmp((char *)DualFwModHdr[j]->ModuleName, "www") == 0)
                {
                    strcpy((char *)DualDispModuleNameFirm[i], "Web Pages   ");
                }
                else
                {
                    strcpy((char *)DualDispModuleNameFirm[i], "            ");
                }
                memcpy((char *)&DispDualFwmodHdr[i]->Module_Version, (char *)&DualFwModHdr[j]->Module_Version, 2);
                if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
                {
                    memcpy((char *)&DispDualFwmodHdr[i]->ModuleAuxVer, (char *)&DualFwModHdr[j]->ModuleAuxVer, 2);
                }
                else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
                {
                    memcpy((char *)&DispDualFwmodHdr[i]->ModuleAuxVer, (char *)&DualFwModHdr[j]->ModuleAuxVer, 6);
                }

                break;
            }
        }
        if (j == DualFMH)
        {
            strcpy((char *)DualDispModuleNameFirm[i], "            ");
            DispDualFwmodHdr[i]->Module_Version.ModMajorVer = '-';
            *(&(DispDualFwmodHdr[i]->Module_Version.ModMinorVer) + 1) = '-';
            ImageDiffer2 = 1;
        }
    }

    /* Checking if any module is missed from Dual FMH Header */
    TotalCount = DispCount;

    for (i = 0; i < DualFMH; i++)
    {
        for (j = 0; j < TotalCount; j++)
        {
            if (spstrncasecmp(DualFwModHdr[i]->ModuleName, DispFwmodHdr[j]->ModuleName, sizeof(DualFwModHdr[j]->ModuleName)) == 0)
            {
                break;
            }
        }

        if (j == TotalCount)
        {
            if (DispDualMemAlloc(DispCount) != 0)
            {
                return -1;
            }
            strcpy(DispDualFwmodHdr[DispCount]->ModuleName, DualFwModHdr[i]->ModuleName);
            if (spstrcasecmp((char *)DualFwModHdr[j]->ModuleName, "boot") == 0)
            {
                strcpy((char *)DualDispModuleNameFirm[DispCount], "BootLoader  ");
            }
            else if ((spstrcasecmp((char *)DualFwModHdr[j]->ModuleName, "conf")) == 0 ||
                     (spstrcasecmp((char *)DualFwModHdr[j]->ModuleName, "params")) == 0)
            {
                strcpy((char *)DualDispModuleNameFirm[DispCount], "ConfigParams");
            }
            else if (spstrcasecmp((char *)DispFwmodHdr[j]->ModuleName, "root") == 0)
            {
                strcpy((char *)DualDispModuleNameFirm[DispCount], "Root       ");
            }
            else if (spstrcasecmp((char *)DualFwModHdr[j]->ModuleName, "osimage") == 0)
            {
                strcpy((char *)DualDispModuleNameFirm[DispCount], "Linux OS    ");
            }
            else if (spstrcasecmp((char *)DualFwModHdr[i]->ModuleName, "www") == 0)
            {
                strcpy((char *)DualDispModuleNameFirm[DispCount], "Web Pages   ");
            }
            else
            {
                strcpy((char *)DualDispModuleNameFirm[DispCount], "            ");
            }
            memcpy((char *)&DispDualFwmodHdr[DispCount]->Module_Version, (char *)&DualFwModHdr[j]->Module_Version, 2);
            if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
            {
                memcpy((char *)&DispDualFwmodHdr[DispCount]->ModuleAuxVer, (char *)&DualFwModHdr[j]->ModuleAuxVer, 2);
            }
            else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
            {
                memcpy((char *)&DispDualFwmodHdr[DispCount]->ModuleAuxVer, (char *)&DualFwModHdr[j]->ModuleAuxVer, 6);
            }

            if (DispMemAlloc(DispCount) != 0)
            {
                return -1;
            }
            strcpy(DispFwmodHdr[DispCount]->ModuleName, DualFwModHdr[i]->ModuleName);
            strcpy((char *)DispModuleNameFirm[DispCount], "            ");
            DispFwmodHdr[DispCount]->Module_Version.ModMajorVer = '-';
            *(&(DispFwmodHdr[DispCount]->Module_Version.ModMinorVer) + 1) = '-';
            strcpy(DispCurrFwModHdr[DispCount]->ModuleName, DualFwModHdr[i]->ModuleName);
            strcpy((char *)DispModuleNameRom[DispCount], "            ");
            DispCurrFwModHdr[DispCount]->Module_Version.ModMajorVer = '-';
            *(&(DispCurrFwModHdr[DispCount]->Module_Version.ModMinorVer) + 1) = '-';
            DispCount++;
            ImageDiffer2 = 1;
        }
    }

    return 0;
}

/*
 * @fn DisplayModulesRom
 * @brief this function Display the information about the modules of ROM
 * @param Count - Hold the Module No
 * @return Returns 0 on success
 */
int DisplayModulesRom(int Count)
{
    int j, k;
    for (j = 0; j < ModuleCount; j++)
    {
        if (spstrncasecmp(CurrFwModHdr[Count]->ModuleName, FwModuleHdr[j]->ModuleName, sizeof(FwModuleHdr[j]->ModuleName)) == 0)
        {
            for (k = 0; k < DispCount; k++)
            {
                if (spstrncasecmp(CurrFwModHdr[Count]->ModuleName, Modulechklist[k], sizeof(CurrFwModHdr[Count]->ModuleName)) == 0)
                {
                    found = 0x00;
                    break;
                }
            }
            if (found != 0x00)
            {
                if (DispMemAlloc(DispCount) != 0)
                    return -1;
                strcpy(DispCurrFwModHdr[DispCount]->ModuleName, CurrFwModHdr[Count]->ModuleName);
                strcpy(DispModuleNameFirm[DispCount], ModuleNameFirm[Count]);
                memcpy((char *)&DispCurrFwModHdr[DispCount]->Module_Version, (char *)&CurrFwModHdr[Count]->Module_Version, 2);
                if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
                    memcpy((char *)&DispCurrFwModHdr[DispCount]->ModuleAuxVer, (char *)&CurrFwModHdr[Count]->ModuleAuxVer, 2);
                else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
                    memcpy((char *)&DispCurrFwModHdr[DispCount]->ModuleAuxVer, (char *)&CurrFwModHdr[Count]->ModuleAuxVer, 6);

                strcpy(DispFwmodHdr[DispCount]->ModuleName, FwModuleHdr[j]->ModuleName);
                strcpy(DispModuleNameRom[DispCount], ModuleNameRom[j]);
                memcpy((char *)&DispFwmodHdr[DispCount]->Module_Version, (char *)&FwModuleHdr[j]->Module_Version, 2);
                if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
                    memcpy((char *)&DispFwmodHdr[DispCount]->ModuleAuxVer, (char *)&FwModuleHdr[j]->ModuleAuxVer, 2);
                else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
                    memcpy((char *)&DispFwmodHdr[DispCount]->ModuleAuxVer, (char *)&FwModuleHdr[j]->ModuleAuxVer, 6);

                strcpy(Modulechklist[DispCount], CurrFwModHdr[Count]->ModuleName);
                Matchfound = 0x01;
                DispCount++;
            }
        }
    }

    if ((Matchfound != 0x01) && (found != 0x00))
    {
        Matchfound = 0x01;
        found = 0x00;
        if (DispMemAlloc(DispCount) != 0)
            return -1;
        strcpy(DispCurrFwModHdr[DispCount]->ModuleName, CurrFwModHdr[Count]->ModuleName);
        strcpy(DispModuleNameFirm[DispCount], ModuleNameFirm[Count]);
        memcpy((char *)&DispCurrFwModHdr[DispCount]->Module_Version, (char *)&CurrFwModHdr[Count]->Module_Version, 2);
        if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
            memcpy((char *)&DispCurrFwModHdr[DispCount]->ModuleAuxVer, (char *)&CurrFwModHdr[Count]->ModuleAuxVer, 2);
        else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
            memcpy((char *)&DispCurrFwModHdr[DispCount]->ModuleAuxVer, (char *)&CurrFwModHdr[Count]->ModuleAuxVer, 6);

        strcpy(DispFwmodHdr[DispCount]->ModuleName, CurrFwModHdr[Count]->ModuleName);
        strcpy(DispModuleNameRom[DispCount], ModuleNameFirm[Count]);
        DispFwmodHdr[DispCount]->Module_Version.ModMajorVer = '-';
        *(&(DispFwmodHdr[DispCount]->Module_Version.ModMinorVer) + 1) = '-';
        ImageDiffer1 = 0x01;
        DispCount++;
    }
    return 0;
}
/*
 * @fn DisplayModulesFlash
 * @brief this function Display the information about the modules of ExistingImage From Firmware
 * @param Count - Hold the Module No
 * @return Returns 0 on success
 */

int DisplayModulesFlash(int Count)
{
    int j, k;
    for (j = 0; j < FlashFMH; j++)
    {
        if (spstrncasecmp(FwModuleHdr[Count]->ModuleName, CurrFwModHdr[j]->ModuleName, sizeof(FwModuleHdr[Count]->ModuleName)) == 0)
        {
            for (k = 0; k < DispCount; k++)
            {
                if (spstrncasecmp(FwModuleHdr[Count]->ModuleName, Modulechklist[k], sizeof(FwModuleHdr[Count]->ModuleName)) == 0)
                {
                    found = 0x00;
                    break;
                }
            }
            if (found != 0x00)
            {
                if (DispMemAlloc(DispCount) != 0)
                    return -1;
                strcpy(DispFwmodHdr[DispCount]->ModuleName, FwModuleHdr[Count]->ModuleName);
                strcpy(DispModuleNameRom[DispCount], ModuleNameRom[Count]);
                memcpy((char *)&DispFwmodHdr[DispCount]->Module_Version, (char *)&FwModuleHdr[Count]->Module_Version, 2);
                if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
                    memcpy((char *)&DispFwmodHdr[DispCount]->ModuleAuxVer, (char *)&FwModuleHdr[Count]->ModuleAuxVer, 2);
                else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
                    memcpy((char *)&DispFwmodHdr[DispCount]->ModuleAuxVer, (char *)&FwModuleHdr[Count]->ModuleAuxVer, 6);

                strcpy(DispCurrFwModHdr[DispCount]->ModuleName, CurrFwModHdr[j]->ModuleName);
                strcpy(DispModuleNameFirm[DispCount], ModuleNameFirm[j]);
                memcpy((char *)&DispCurrFwModHdr[DispCount]->Module_Version, (char *)&CurrFwModHdr[j]->Module_Version, 2);
                if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
                    memcpy((char *)&DispCurrFwModHdr[DispCount]->ModuleAuxVer, (char *)&CurrFwModHdr[j]->ModuleAuxVer, 2);
                else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
                    memcpy((char *)&DispCurrFwModHdr[DispCount]->ModuleAuxVer, (char *)&CurrFwModHdr[j]->ModuleAuxVer, 6);

                strcpy(Modulechklist[DispCount], CurrFwModHdr[j]->ModuleName);
                Matchfound = 0x01;
                DispCount++;
            }
        }
    }

    if ((Matchfound != 0x01) && (found != 0x00))
    {
        if (DispMemAlloc(DispCount) != 0)
            return -1;
        strcpy(DispFwmodHdr[DispCount]->ModuleName, FwModuleHdr[Count]->ModuleName);
        strcpy(DispModuleNameRom[DispCount], ModuleNameRom[Count]);
        memcpy((char *)&DispFwmodHdr[DispCount]->Module_Version, (char *)&FwModuleHdr[Count]->Module_Version, 2);
        if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
            memcpy((char *)&DispFwmodHdr[DispCount]->ModuleAuxVer, (char *)&FwModuleHdr[Count]->ModuleAuxVer, 2);
        else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
            memcpy((char *)&DispFwmodHdr[DispCount]->ModuleAuxVer, (char *)&FwModuleHdr[Count]->ModuleAuxVer, 6);

        strcpy(DispCurrFwModHdr[DispCount]->ModuleName, FwModuleHdr[Count]->ModuleName);
        strcpy(DispModuleNameFirm[DispCount], ModuleNameRom[Count]);
        DispCurrFwModHdr[DispCount]->Module_Version.ModMajorVer = '-';
        *(&(DispCurrFwModHdr[DispCount]->Module_Version.ModMinorVer) + 1) = '-';
        ImageDiffer1 = 0x01;
        DispCount++;
    }

    return 0;
}
/*
 * @fn DisplayModulesRearrange
 * @brief Thsi Function Rearrange the information of modules of Rom and ExistingImage from firmware
 * @return Returns 0 on success
 */
int DisplayModulesRearrange()
{
    int Count;

    for (Count = 0; Count < ModCount; Count++)
    {
        Matchfound = 0x00;
        found = 0x01;

        if ((Count < FlashFMH) && (Count < ModuleCount))
        {
            if (spstrncasecmp(FwModuleHdr[Count]->ModuleName, CurrFwModHdr[Count]->ModuleName, sizeof(FwModuleHdr[Count]->ModuleName)) == 0)
            {
                if (DispMemAlloc(DispCount) != 0)
                    return -1;
                strcpy(DispFwmodHdr[DispCount]->ModuleName, FwModuleHdr[Count]->ModuleName);
                strcpy(DispModuleNameRom[DispCount], ModuleNameRom[Count]);
                memcpy((char *)&DispFwmodHdr[DispCount]->Module_Version, (char *)&FwModuleHdr[Count]->Module_Version, 2);
                if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
                    memcpy((char *)&DispFwmodHdr[DispCount]->ModuleAuxVer, (char *)&FwModuleHdr[Count]->ModuleAuxVer, 2);
                else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
                    memcpy((char *)&DispFwmodHdr[DispCount]->ModuleAuxVer, (char *)&FwModuleHdr[Count]->ModuleAuxVer, 6);

                strcpy(DispCurrFwModHdr[DispCount]->ModuleName, CurrFwModHdr[Count]->ModuleName);
                strcpy(DispModuleNameFirm[DispCount], ModuleNameFirm[Count]);
                memcpy((char *)&DispCurrFwModHdr[DispCount]->Module_Version, (char *)&CurrFwModHdr[Count]->Module_Version, 2);
                if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
                    memcpy((char *)&DispCurrFwModHdr[DispCount]->ModuleAuxVer, (char *)&CurrFwModHdr[Count]->ModuleAuxVer, 2);
                else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
                    memcpy((char *)&DispCurrFwModHdr[DispCount]->ModuleAuxVer, (char *)&CurrFwModHdr[Count]->ModuleAuxVer, 6);

                strcpy(Modulechklist[DispCount], FwModuleHdr[Count]->ModuleName);
                DispCount++;
            }
            else
            {
                if (DisplayModulesFlash(Count) != 0)
                    return -1;
                Matchfound = 0x00;
                found = 0x01;
                if (DisplayModulesRom(Count) != 0)
                    return -1;
            }
        }
        else
        {
            if ((Count >= FlashFMH) && (Count < ModuleCount))
            {
                if (DisplayModulesFlash(Count) != 0)
                    return -1;
            }
            else
            {
                if (DisplayModulesRom(Count) != 0)
                    return -1;
            }
        }
    }
    return 0;
}

int IsfwuploadSelectorValid(char uploadselector)
{
    return ((uploadselector == AUTO_INACTIVE_IMAGE) || (uploadselector == IMAGE_1) || (uploadselector == IMAGE_2) || (uploadselector == IMAGE_BOTH));
}

/*
 * @fn FirmwareInfo
 * @brief this function get the information of existing image from firmware
 * @param hSession - Current Session Pointer
 * @param imgName - Rom Image Name
 * @return Returns 0 on success
 */
int FirmwareInfo(IPMI20_SESSION_T *hSession, char *imgName)
{
    int i = 0, j = 0, k = 0, l = 0, LastSecFirmSize = 0;
    AMIYAFUGetFMHInfoRes_T *pFMHInfoRes;
    char *FMHRes = NULL, ModuleName[16] = {0};
    int DualRes = 0, NumFMH1 = 0, NumFMH2 = 0, fwuploadselector = -1;
    FlashMH tmpFMH;
    bool CurrFwModConf = FALSE, FwModConf = FALSE;

    if (SPIDevice != BMC_FLASH)
    {
        return 0;
    }

    SplitFlashOffset = 0;

    FMHRes = malloc(3000);
    if (FMHRes == NULL)
    {
        return -1;
    }
    memset(FMHRes, 0, 3000);

    pFMHInfoRes = (AMIYAFUGetFMHInfoRes_T *)FMHRes;

    if (FlashModHeadInfo(hSession, (AMIYAFUGetFMHInfoRes_T *)FMHRes) != 0)
    {
        if (vdbg)
            printf("Error in retrieving FMH Headers \n");
        free(FMHRes);
        return -1;
    }
    NumFMH1 = pFMHInfoRes->NumFMH & 0x00FF;
    NumFMH2 = (pFMHInfoRes->NumFMH & 0xFF00) >> 8;

    DualRes = GetDualImageSupport(hSession, GETFWUPLOADSELECTOR, &fwuploadselector);
    if (DualRes == 0)
    {
        DualImageSup = TRUE;
        if (IsfwuploadSelectorValid(fwuploadselector) == FALSE)
        {
            printf("Invalid Firmware Upload selector: %d\n", fwuploadselector);
            exit(YAFU_GET_DUAL_IMAGE_FAILED);
        }

        if (fwuploadselector == AUTO_INACTIVE_IMAGE) // This case is possible only from the command line
        {
            if (GetDualImageSupport(hSession, GETCURACTIVEIMG, &ActiveImg) == 0)
            {
                if (ActiveImg == IMAGE_1)
                    FwUploadImg = IMAGE_2;
                else if (ActiveImg == IMAGE_2)
                    FwUploadImg = IMAGE_1;
                if ((Parsing.Info == 0x01) && (NumFMH2 == 0x00))
                {
                    printf("Image%d is Corrupted or Not Present...\n", FwUploadImg);
                }
            }
            else
            {
                printf("Unable to get Dual Image Configurations\n");
                exit(YAFU_GET_DUAL_IMAGE_FAILED);
            }
        }
        else
        {
            FwUploadImg = fwuploadselector;
        }
    }

    if (Parsing.ImgInfo == 0)
    {
        /*Used upload signed key Image command with size zero , it returns Request data length invalid(0xc7) for
        signed image support and returns invalid command(0xc1) in case signed image support not enabled*/

        // No Need To check Encryption methods for Recovery Mode.
        if (!RecoveryFlashMode)
        {
            if (featuresList.signed_hashed_image_support == 1)
            // if ((CheckSigedImageSupport (hSession, RECIEVE_TIME_OUT) & 0xFF) == CC_REQ_INV_LEN)
            {
                if ((NewImageSize % EraseBlkSize != SIGNED_HASHED_LEGACY_IMG_BYTES) && (NewImageSize % EraseBlkSize != SIGNED_HASHED_IMG_BYTES))
                {
                    printf("Signed Image Support is Enabled and Uploaded image is not a signed Image\n");
                    exit(YAFU_CC_NOT_SIGNED_IMAGE);
                }
            }
            else
            {
                if (NewImageSize % EraseBlkSize != 0 && BMC_FLASH == SPIDevice)
                {
                    printf("Signed Image Support is not Enabled and Uploaded image is signed\n");
                    exit(YAFU_CC_IMAGE_SIGNED);
                }
            }
        }

        if ((NewImageSize % EraseBlkSize == SIGNED_HASHED_LEGACY_IMG_BYTES) || (NewImageSize % EraseBlkSize == SIGNED_HASHED_IMG_BYTES))
        {
            SignedImageSup = 0x01;
        }
        else if ((NewImageSize > FlashSize) && (Parsing.Info == 0) && (BMC_FLASH == SPIDevice))
        {
            printf("WARNING!!The New Image Size is greater than Flash Size!!\n");
            exit(YAFU_GREATER_IMAGE_SIZE);
        }
        sprintf(NewImgBootVer, "%d.%02d.%.2s", FwModuleHdr[0]->Module_Version.ModMajorVer,
                *(&(FwModuleHdr[0]->Module_Version.ModMinorVer) + 1),
                FwModuleHdr[0]->ModuleAuxVer);

        if (NumFMH1 != 0)
        {
            memset(&tmpFMH, 0, sizeof(FlashMH));
            memcpy(&tmpFMH, (FMHRes + sizeof(AMIYAFUGetFMHInfoRes_T)), 64);
            if (tmpFMH.ModuleType == MODULE_BOOTLOADER)
            {
                sprintf(Img1BootVer, "%d.%02d.%.2s", tmpFMH.Module_Version.ModMajorVer,
                        *(&(tmpFMH.Module_Version.ModMinorVer) + 1),
                        tmpFMH.ModuleAuxVer);
            }
        }

        if (NumFMH2 != 0)
        {
            memset(&tmpFMH, 0, sizeof(FlashMH));
            memcpy(&tmpFMH, (FMHRes + (NumFMH1 * sizeof(FlashMH)) + sizeof(AMIYAFUGetFMHInfoRes_T)), 64);
            if (tmpFMH.ModuleType == MODULE_BOOTLOADER)
            {
                sprintf(Img2BootVer, "%d.%02d.%.2s", tmpFMH.Module_Version.ModMajorVer,
                        *(&(tmpFMH.Module_Version.ModMinorVer) + 1),
                        tmpFMH.ModuleAuxVer);
            }
        }

        if ((DualRes == 0) && (NumFMH2 != 0))
        {
            if ((FwUploadImg == IMAGE_1) || (FwUploadImg == IMAGE_BOTH))
            {
                ActiveCurImg = IMAGE_1;
                InActiveImg = IMAGE_2;
                FlashFMH = NumFMH1;
                j = 0;
                DualFMH = NumFMH2;
                k = FlashFMH * sizeof(FlashMH);
            }
            else if (FwUploadImg == IMAGE_2)
            {
                ActiveCurImg = IMAGE_2;
                InActiveImg = IMAGE_1;
                FlashFMH = NumFMH2;
                j = FlashFMH * sizeof(FlashMH);
                DualFMH = NumFMH1;
                k = 0;
            }
        }
        else
        {
            FlashFMH = NumFMH1;
        }

        for (i = 0; i < FlashFMH; i++)
        {
            CurrFwModHdr[i] = (FlashMH *)malloc(sizeof(FlashMH));
            if (CurrFwModHdr[i] == NULL)
            {
                return -1;
            }
            memset(CurrFwModHdr[i], 0, sizeof(CurrFwModHdr[i]));
            memcpy(CurrFwModHdr[i], (FMHRes + j + sizeof(AMIYAFUGetFMHInfoRes_T)), 64);
            j += 64;
        }

        for (i = 0; i < DualFMH; i++)
        {
            DualFwModHdr[i] = (FlashMH *)malloc(sizeof(FlashMH));
            if (DualFwModHdr[i] == NULL)
            {
                return -1;
            }
            memset(DualFwModHdr[i], 0, sizeof(DualFwModHdr[i]));
            memcpy(DualFwModHdr[i], (FMHRes + k + sizeof(AMIYAFUGetFMHInfoRes_T)), 64);
            k += 64;
        }

        j = 0;
        for (i = 0; i < NumFMH1; i++)
        {
            PrimFwModHdr[i] = (FlashMH *)malloc(sizeof(FlashMH));
            if (PrimFwModHdr[i] == NULL)
            {
                return -1;
            }
            memset(PrimFwModHdr[i], 0, sizeof(CurrFwModHdr[i]));
            memcpy(PrimFwModHdr[i], (FMHRes + j + sizeof(AMIYAFUGetFMHInfoRes_T)), 64);
            j += 64;
        }

        ModCount = ModuleCount > FlashFMH ? ModuleCount : FlashFMH;

        if (Parsing.SkipFMHCheck == 0x01)
        {
            printf("Skipped Checking for FMH...\n");
            FlashFMH = 0;
            return 0;
        }
    }
    else if ((Parsing.ImgInfo == 1) || (Parsing.ImgInfo == 2))
    {
        if ((DualRes == 0) && (NumFMH2 == 0) && (SPIDevice != CPLD_FLASH))
        {
            printf("This Options is not supported at this time\n");
            return -1;
        }
        for (i = 0; i < NumFMH1; i++)
        {
            if (FwModuleHdr[k] == NULL)
            {
                FwModuleHdr[k] = (FlashMH *)malloc(sizeof(FlashMH));
                if (FwModuleHdr[k] == NULL)
                {
                    return -1;
                }
            }
            memset(FwModuleHdr[k], 0, sizeof(FwModuleHdr[k]));
            memcpy(FwModuleHdr[k], (FMHRes + j + sizeof(AMIYAFUGetFMHInfoRes_T)), 64);
            j += 64;
            if ((Parsing.ImgInfo == 2) && (FwModuleHdr[k]->ModuleType == 2))
            {
                k++;
            }
            else if (Parsing.ImgInfo == 1)
            {
                k++;
            }
        }

        for (i = 0; i < NumFMH2; i++)
        {
            if (CurrFwModHdr[l] == NULL)
            {
                CurrFwModHdr[l] = (FlashMH *)malloc(sizeof(FlashMH));
                if (CurrFwModHdr[l] == NULL)
                {
                    return -1;
                }
            }
            memset(CurrFwModHdr[l], 0, sizeof(CurrFwModHdr[l]));
            memcpy(CurrFwModHdr[l], (FMHRes + j + sizeof(AMIYAFUGetFMHInfoRes_T)), 64);
            j += 64;
            if ((Parsing.ImgInfo == 2) && (CurrFwModHdr[l]->ModuleType == 2))
            {
                l++;
            }
            else if (Parsing.ImgInfo == 1)
            {
                l++;
            }
        }

        if (Parsing.ImgInfo == 1)
        {
            ModuleCount = NumFMH1;
            FlashFMH = NumFMH2;
        }
        else
        {
            if (Parsing.SplitImg == 1)
            {
                ModuleCount = k;
            }
            else
            {
                ModuleCount = k; // Only Firmware Module to be displayed
            }

            if (NumFMH2 != 0)
            {
                FlashFMH = k;
            }
        }
        ModCount = ModuleCount > FlashFMH ? ModuleCount : FlashFMH;
    }

    for (i = 0; i < ModCount; i++)
    {
        if (i < ModuleCount)
        {
            strcpy(ModuleName, (char *)FwModuleHdr[i]->ModuleName);
            ModuleName[8] = '\0';
            if (spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "boot") == 0)
            {
                strcpy((char *)ModuleNameRom[i], "BootLoader  ");
                BootRomAllocAddr = FwModuleHdr[i]->AllocatedSize;
                BootRomAddr = FwModuleHdr[i]->ModuleLocation;
            }
            else if (spstrcasecmp(ModuleName, "boot_img") == 0)
                BootImgRomAddr = FwModuleHdr[i]->FmhLocation;
            else if (((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "conf")) == 0 ||
                      (spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "params")) == 0) &&
                     (FwModConf == FALSE))
            {
                strcpy((char *)ModuleNameRom[i], "ConfigParams");
                ConfigRomAllocAddr = FwModuleHdr[i]->AllocatedSize;
                ConfigRomAddr = FwModuleHdr[i]->FmhLocation;
                FwModConf = TRUE;
            }
            else if (spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "extlog") == 0)
            {
                strcpy((char *)ModuleNameRom[i], "ExtendedLog ");
                ExtlogRomAddr = FwModuleHdr[i]->FmhLocation;
            }
            else if (spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "root") == 0)
                strcpy((char *)ModuleNameRom[i], "Root       ");
            else if (spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "osimage") == 0)
                strcpy((char *)ModuleNameRom[i], "Linux OS    ");
            else if (spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "www") == 0)
                strcpy((char *)ModuleNameRom[i], "Web Pages   ");
            else
                strcpy((char *)ModuleNameRom[i], "            ");
        }

        if (i < FlashFMH)
        {
            strcpy(ModuleName, (char *)CurrFwModHdr[i]->ModuleName);
            ModuleName[8] = '\0';
            if (spstrcasecmp((char *)CurrFwModHdr[i]->ModuleName, "boot") == 0)
            {
                strcpy((char *)ModuleNameFirm[i], "BootLoader  ");
                BootFirmAllocAddr = CurrFwModHdr[i]->AllocatedSize;
                BootFirmAddr = CurrFwModHdr[i]->ModuleLocation;
            }
            else if (spstrcasecmp(ModuleName, "boot_img") == 0)
                BootImgFirmAddr = CurrFwModHdr[i]->FmhLocation;
            else if (((spstrcasecmp((char *)CurrFwModHdr[i]->ModuleName, "conf")) == 0 ||
                      (spstrcasecmp((char *)CurrFwModHdr[i]->ModuleName, "params")) == 0) &&
                     (CurrFwModConf == FALSE))
            {
                strcpy((char *)ModuleNameFirm[i], "ConfigParams");
                ConfigFirmAllocAddr = CurrFwModHdr[i]->AllocatedSize;
                ConfigFirmAddr = CurrFwModHdr[i]->FmhLocation;
                CurrFwModConf = TRUE;
            }
            else if (spstrcasecmp((char *)CurrFwModHdr[i]->ModuleName, "extlog") == 0)
            {
                strcpy((char *)ModuleNameFirm[i], "ExtendedLog ");
                ExtlogFirmAllocAddr = CurrFwModHdr[i]->AllocatedSize;
                ExtlogFirmAddr = CurrFwModHdr[i]->FmhLocation;
            }
            else if (spstrcasecmp((char *)CurrFwModHdr[i]->ModuleName, "root") == 0)
                strcpy((char *)ModuleNameFirm[i], "Root       ");

            else if (spstrcasecmp((char *)CurrFwModHdr[i]->ModuleName, "osimage") == 0)
                strcpy((char *)ModuleNameFirm[i], "Linux OS    ");

            else if (spstrcasecmp((char *)CurrFwModHdr[i]->ModuleName, "www") == 0)
                strcpy((char *)ModuleNameFirm[i], "Web Pages   ");
            else
                strcpy((char *)ModuleNameFirm[i], "            ");
        }
    }

    if ((Parsing.SplitImg == 0x01) && (Parsing.Info != 1) && (Parsing.ImgInfo == 0))
    {
        for (i = 0; i < ModCount; i++)
        {
            if (spstrcasecmp((char *)CurrFwModHdr[i]->ModuleName, (char *)FwModuleHdr[0]->ModuleName) != 0)
            {
                SplitFlashOffset += CurrFwModHdr[i]->AllocatedSize;
            }
            else
            {
                break;
            }
        }

        if (SplitFlashOffset != 0)
        {
            j = 0;
            for (i = 0; i < ModCount; i++)
            {
                if (j < ModuleCount)
                {
                    if (spstrcasecmp((char *)FwModuleHdr[j]->ModuleName, (char *)CurrFwModHdr[i]->ModuleName) == 0)
                    {
                        if (FwModuleHdr[j]->AllocatedSize != CurrFwModHdr[i]->AllocatedSize)
                        {
                            printf("The Module %s size is different from the existing Image.\nplease do full firmware flashing \n", FwModuleHdr[j]->ModuleName);
                            return -2;
                        }
                        else
                        {
                            j++;
                            continue;
                        }
                    }
                }
            }
        }
    }

    if ((Parsing.Info != 1) && (Parsing.ImgInfo == 0))
    {
        /* Calculating free space addr by adding first module to last -1 module and free space size is calculated by subtracting
        free space addr (last to previous section size ) and last section size from image size*/
        for (i = 0; i < ModCount; i++)
        {
            if (i + 2 <= ModuleCount)
            {
                FreeSpaceRomAddr += FwModuleHdr[i]->AllocatedSize;
            }

            if (i + 2 <= FlashFMH)
            {
                FreeSpaceFirmAddr += CurrFwModHdr[i]->AllocatedSize;
            }
        }

        LastSecFirmSize = CurrFwModHdr[FlashFMH - 1]->AllocatedSize;

        if (Parsing.SplitImg == 0x01 && SplitFlashOffset != 0)
            FreeSpaceFirmSize = (CurrFwModHdr[FlashFMH - 1]->FmhLocation + CurrFwModHdr[FlashFMH - 1]->AllocatedSize) - FreeSpaceFirmAddr - LastSecFirmSize;
        else
            FreeSpaceFirmSize = NewImageSize - FreeSpaceFirmAddr - LastSecFirmSize;
    }

    if (ModuleCount <= 2)
    {
        if (spstrcasecmp((char *)FwModuleHdr[0]->ModuleName, "boot") == 0)
            BootOnly = 0x01;
    }

    if ((Parsing.SplitImg == 0x01 && ModuleCount == 2 && Parsing.BootPreserve == 0x01 && Parsing.FlashModImg != 1 && Parsing.Info != 1))
    {
        if (spstrcasecmp((char *)FwModuleHdr[0]->ModuleName, "boot") == 0)
        {
            if (Parsing.versioncmpflash == 0)
            {
                printf("preserve-boot is enabled. Updating only Uboot version is not recommended exiting... \n");
                FreeFMHMemory();
                exit(0);
            }
        }
    }

    if (DisplayModulesRearrange() != 0)
    {
        if (vdbg)
            printf("Error in Dislay Modules rearrange \n");
        return -1;
    }

    // Calling the Dual Image Respone Rearrange
    if (DualFMH != 0)
    {
        if (DisplayDualModuleRearrange() != 0)
        {
            if (vdbg)
                printf("Error in Dislay Modules rearrange DualFMH\n");
            return -1;
        }
    }

    free(FMHRes);
    return 0;
}
/*
 * @fn ModLocationChkAndUpgrade
 * @brief This function is to Upgrade the Firmware by module
 * @param hSession - Current Session Pointer
 * @param Module - Module No to Upgrade
 * @return Returns 0 on success
 */
int ModLocationChkAndUpgrade(IPMI20_SESSION_T *hSession, int Module, int CurNumModule, int MaxNumModule)
{
    int Updateconfig = 0, i = 0, ret1 = 0;
    int FmhLocation = 0, AllocatedSize = 0;
    char *BootVarsName = NULL;
    int retval = 0;
    char FlashChoice[MAX_INPUT_LEN];
    char ModuleName[sizeof(FwModuleHdr[Module]->ModuleName) + 1] = "\0";

    if (NoOfImg == IMAGE_1)
    {
        for (i = 0; i < FlashFMH; i++)
        {
            if ((strncmp("conf", FwModuleHdr[Module]->ModuleName, sizeof(FwModuleHdr[Module]->ModuleName)) == 0) &&
                (strncmp(FwModuleHdr[Module]->ModuleName, CurrFwModHdr[i]->ModuleName, sizeof(FwModuleHdr[Module]->ModuleName)) == 0) && g_confflag)
            {
                g_confflag = 0;
                continue;
            }
            if (strncmp(FwModuleHdr[Module]->ModuleName, CurrFwModHdr[i]->ModuleName, sizeof(FwModuleHdr[Module]->ModuleName)) == 0)
            {
                FmhLocation = CurrFwModHdr[i]->FmhLocation;
                AllocatedSize = CurrFwModHdr[i]->AllocatedSize;
                g_confflag = 0;
                break;
            }
        }
        if (i == FlashFMH)
        {
            if ((DualImageSup == TRUE) && (FlashBothImg == TRUE))
            {
                printf("Ignoring Module \"%s\" as it is not available in current Flash Image.%d\n", FwModuleHdr[Module]->ModuleName, ActiveCurImg);
            }
            else
            {
                printf("Ignoring Module \"%s\" as it is not available in current Flash Image\n", FwModuleHdr[Module]->ModuleName);
            }
            return 0;
        }
    }

    if ((DualImageSup == TRUE) && (FlashBothImg == TRUE) && (NoOfImg == IMAGE_2))
    {
        for (i = 0; i < DualFMH; i++)
        {
            if (strncmp(FwModuleHdr[Module]->ModuleName, DualFwModHdr[i]->ModuleName, sizeof(FwModuleHdr[Module]->ModuleName)) == 0)
            {
                FmhLocation = CurrFwModHdr[i]->FmhLocation;
                AllocatedSize = CurrFwModHdr[i]->AllocatedSize;
                break;
            }
        }
        if (i == DualFMH)
        {
            printf("Ignoring Module \"%s\" as it is not available in current Flash Image.%d\n", FwModuleHdr[Module]->ModuleName, InActiveImg);
            return 0;
        }
    }

    if ((FwModuleHdr[Module]->FmhLocation != FmhLocation) || (FwModuleHdr[Module]->AllocatedSize != AllocatedSize))
    {
        printf("The Module %s location or Allocated size  is different from the one in the Image\n ", FwModuleHdr[Module]->ModuleName);
        if (Parsing.Interactive == 0x01 && Parsing.InteractiveFull != 0x01)
        {
            return -1;
        }

        EraseAndFlash(hSession, 0, 0, 1);
        if (RecoveryFlashMode == 0x00)
        {
            if (ECFStatus(hSession) == -1)
            {
                oldVersion = 0x01;
                if (vdbg)
                    printf("Older Version !Doing Firmware upgrade block by block\n");
            }
            else
            {
                oldVersion = 0x00;
                if (vdbg)
                    printf("Newer Version ! Doing Full Firmware upgrade \n");
            }
        }
        Parsing.Full = 0x01;
        while (1)
        {
            printf("So,Type (Y/y) to do Full Firmware Upgrade or (N/n) to exit\n");
            printf("Enter your Option : ");
            scanf(" %[^\n]%*c", FlashChoice);
            if (strcmp(FlashChoice, "y") == 0 || strcmp(FlashChoice, "Y") == 0)
            {
                for (i = 0; i < FlashFMH; i++)
                {
                    if ((spstrcasecmp((char *)CurrFwModHdr[i]->ModuleName, "conf") == 0) ||
                        (spstrcasecmp((char *)CurrFwModHdr[i]->ModuleName, "params")) == 0)
                        Updateconfig = 0x01;
                }
                if (Updateconfig == 0x01)
                {
                    Config = Parsing.ConfigPreserve;
                }
                if (Parsing.InteractiveFull == TRUE)
                {
                    Boot = Parsing.BootPreserve;
                }
                printf("Doing Full Firmware upgrade \n");
                WarningMessage();
                if (Boot == 0x00)
                {
                    BootVarsName = malloc(400);
                    if (BootVarsName == NULL)
                    {
                        printf("Error in mem alloc of BootVars \n");
                        return -1;
                    }
                    memset(BootVarsName, 0, 400);
                    if (PreserveUbootEnvVariables(hSession, BootVarsName, &BootVarsCount) != 0)
                    {
                        printf("Error in PreserveEnvVariables \n");
                        return -1;
                    }
                    if (ImgOpt == -1)
                    {
                        RegenerateUBootVars(&env, bvnArr, BootVarsCount, EraseBlkSize);
                    }
                }
                if ((retval = FlashFullFirmwareImage(hSession, fp, Config, Boot)) != 0)
                {
                    if (retval == 1)
                    {
                        while (1)
                        {
                            printf("So,Type (Y/y) to do Full Firmware Upgrade By Blocks  \n");
                            printf("or (N/n) to exit\n");
                            printf("Enter your Option : ");
                            scanf(" %[^\n]%*c", FlashChoice);
                            if (strcmp(FlashChoice, "n") == 0 || strcmp(FlashChoice, "N") == 0)
                            {
                                printf("Error in Flashing Full Firmware Image \n");
                                retval = -1;
                                goto outM;
                            }
                            else if (strcmp(FlashChoice, "y") == 0 || strcmp(FlashChoice, "Y") == 0)
                            {
                                Parsing.Full = 0x02;
                                break;
                            }
                            else
                                printf("Please enter a valid option \n");
                        }
                        if (FlashFullFirmwareImage(hSession, fp, Config, Boot) != 0)
                        {
                            printf("Error in Flashing Full Firmware Image \n");
                            retval = -1;
                            goto outM;
                        }
                    }
                    else
                    {
                        printf("Error in Flashing Full Firmware Image \n");
                        goto outM;
                    }
                }
            outM:
                if (oldVersion != 0x01 || Parsing.Full != 0x02)
                {
                    if ((Boot == 0x00) && (Parsing.versioncmpflash == 0x00))
                    {
                        if (ImgOpt == -1)
                        {
                            SetBlkUBootVars(hSession, &env, EraseBlkSize);
                        }
                        else
                        {
                            if (SettingEnvUbootVariables(hSession, BootVarsName, &BootVarsCount) != 0)
                            {
                                printf("Error in SettingEnvUbootVariables \n");
                                return -1;
                            }
                        }

                        free(BootVarsName);
                    }
                }

                if (DeactivateFlshMode(hSession) != 0)
                {
                    printf("\nError in Deactivate Flash mode\n");
                    RestoreFlashDevice(&hSession);
                    return -1;
                }

                if (Parsing.RebootFirmware == 0x01)
                {
                    if (((featuresList.mmc_boot_support) && (Parsing.DoMMCImgUpdate == 1) && (IsMMCUpdated == 0)))
                    {

                        ret1 = DoMMCUpdate(&hSession);
                        if (ret1 != 0)
                        {
                            printf("Error in Updating MMC Image\n");
                            return -1;
                        }
                        return 0;
                    }
                    else
                    {
                        ResetFunction(hSession);
                    }
                }

                exit(0);
            }
            else if (strcmp(FlashChoice, "n") == 0 || strcmp(FlashChoice, "N") == 0)
            {
                if (Parsing.InteractiveFull == 0x01)
                {
                    return -1;
                }
                exit(0);
            }
        }
    }

    if ((Boot == 0) && (strcmp(FwModuleHdr[Module]->ModuleName, "boot") == 0))
    {
        BootVarsName = malloc(400);
        if (BootVarsName == NULL)
        {
            printf("Error in mem alloc of BootVars \n");
            return -1;
        }
        memset(BootVarsName, 0, 400);
        if (PreserveUbootEnvVariables(hSession, BootVarsName, &BootVarsCount) != 0)
        {
            printf("Error in PreserveEnvVariables \n");
            return -1;
        }
    }

    strncpy(ModuleName, FwModuleHdr[Module]->ModuleName, sizeof(FwModuleHdr[Module]->ModuleName));
    ModuleName[sizeof(FwModuleHdr[Module]->ModuleName)] = '\0';

    printf("Updating the module %s .....\r", ModuleName);
    fflush(stdout);

    if (ModuleUpgrade(hSession, fp, Module, 1, CurNumModule, MaxNumModule) != 0)
    {
        printf("\nError in Module upgrade \n");
        return -1;
    }

    printf("\rUpdating the module %s ..... Done\n", ModuleName);
    fflush(stdout);

    if ((Boot == 0) && (strcmp(FwModuleHdr[Module]->ModuleName, "boot") == 0) && (Parsing.versioncmpflash == 0x00))
    {
        if (SettingEnvUbootVariables(hSession, BootVarsName, &BootVarsCount) != 0)
        {
            printf("Error in SettingEnvUbootVariables \n");
            return -1;
        }
        BootVarsCount = 0;
        free(BootVarsName);
    }

    return 0;
}
/*
 * @fn AutoFirmwareFlashing
 * @brief This function is to Automatically Upgrade the Firmware by comparing module verison
 * @param hSession - Current Session Pointer
 * @return Returns 0 on success
 */
int AutoFirmwareFlashing(IPMI20_SESSION_T *hSession)
{
    int Count = 0;
    int UpdatedModule = 0;
    char FlashChoice[MAX_INPUT_LEN];
    int FwDiffVer = 0;
    int ret1 = 0;

    if (DisplayModuleDiffer() == 0)
    {
        while (1)
        {
            if (Parsing.Info == 0x01)
                printf("The Images are different. \n ");
            else
                printf("The Images are different. Use '-info' Option to know more \n");
            printf("So,Type (Y/y) to do Full Firmware Upgrade or (N/n) to exit\n");
            printf("Enter your Option : ");
            scanf(" %[^\n]%*c", FlashChoice);
            if (strcmp(FlashChoice, "y") == 0 || strcmp(FlashChoice, "Y") == 0)
            {
                Parsing.Full = 0x01;
                break;
            }
            else if (strcmp(FlashChoice, "n") == 0 || strcmp(FlashChoice, "N") == 0)
                exit(0);
        }
        return 0;
    }
    WarningMessage();

    if (ActivateFlashMode(hSession) != 0)
    {
        printf("\nError in activate flash mode \n");
        return -1;
    }

    for (Count = 0; Count < ModCount; Count++)
    {
        FwDiffVer = 0;

        if ((FwModuleHdr[Count]->Module_Version.ModVersion != CurrFwModHdr[Count]->Module_Version.ModVersion))
        {
            FwDiffVer = 1;
        }

        if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
        {
            if (strncmp(FwModuleHdr[Count]->ModuleAuxVer, CurrFwModHdr[Count]->ModuleAuxVer, 2) != 0) // same version
            {
                FwDiffVer = 1;
            }
        }
        else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
        {
            if (strncmp(FwModuleHdr[Count]->ModuleAuxVer, CurrFwModHdr[Count]->ModuleAuxVer, 6) != 0) // same version
            {
                FwDiffVer = 1;
            }
        }

        if (FwDiffVer == 1)
        {
            if ((ModLocationChkAndUpgrade(hSession, Count, Count, ModCount) != 0))
            {
                return -1;
            }
            UpdatedModule = 1;
            if (FullFlashDone == 0x01)
            {
                UpdatedModule = 0x00;
                break;
            }
        }
        else
            printf("The module %s is not changed \n", FwModuleHdr[Count]->ModuleName);
    }
    if ((UpdatedModule == 1) && (Parsing.RebootFirmware == 0x01))
    {
        if (((featuresList.mmc_boot_support) && (Parsing.DoMMCImgUpdate == 1) && (IsMMCUpdated == 0)))
        {
            ret1 = DoMMCUpdate(&hSession);
            if (ret1 != 0)
            {
                printf("Error in Updating MMC Image\n");
                return -1;
            }
            return 0;
        }
        else
        {
            ResetFunction(hSession);
        }
    }
    return 0;
}

/*
 * @fn DisplayModuleDiffer
 * @brief This function is to Display Module Differ
 * @return Returns 0 on success, -1 on failure
 */
int DisplayModuleDiffer()
{
    int ImgDiff1 = 0, ImgDiff2 = 0;

    ImgDiff1 = ((ModuleCount != FlashFMH) || (ImageDiffer1 == 0x01)) ? TRUE : FALSE;

    if ((DualImageSup == TRUE) && (FlashBothImg == TRUE))
    {
        ImgDiff2 = ((ModuleCount != DualFMH) || (ImageDiffer2 == 0x01)) ? TRUE : FALSE;

        if (!ImgDiff1)
        {
            printf("The Image Modules count are same for Image-%d\n", ActiveCurImg);

            if (!ImgDiff2)
            {
                printf("The Image Modules count are same for Image-%d\n", InActiveImg);
            }
            else
            {
                printf("The Image Modules count are different for Image-%d\n", InActiveImg);
                return 0;
            }
        }
        else
        {
            printf("The Image Modules count are Different for Image-%d\n", ActiveCurImg);

            if (!ImgDiff2)
            {
                printf("The Image Modules count are same for Image-%d\n", InActiveImg);
            }
            else
            {
                printf("The Image Modules count are different for Image-%d\n", InActiveImg);
            }

            return 0;
        }
    }
    else if (ImgDiff1)
    {
        printf("The Image Modules count are different for Image \nSo,");
        return 0;
    }
    return -1;
}

/*
 * @fn CalculateOffset
 * @brief This function is to calculate offset by given module name
 * @param ModuleName - Current Session Pointer
 * @param SizetoCopy - size of the module
 * @param CurrFMHLoc - Location to existing firmware
 *.@param FMHLocation- Location of current image
 * @return Returns 0 on success, 1 on faliure
 */
#if defined(__x86_64__) || defined(WIN64)
int CalculateOffset(char *ModuleName, unsigned int *SizetoCopy, INT32U *CurrFMHLoc, INT32U *FMHLocation)
#else
int CalculateOffset(char *ModuleName, unsigned long *SizetoCopy, INT32U *CurrFMHLoc, INT32U *FMHLocation)
#endif
{
    int i = 0;
    *SizetoCopy = *CurrFMHLoc = *FMHLocation = 0;

    for (i = 0; i < ModCount; i++)
    {
        CurrFwModHdr[i]->ModuleName[8] = '\0';
        if (strcmp((char *)CurrFwModHdr[i]->ModuleName, ModuleName) == 0)
        {
            if (Bkupflag == 1 && strcmp(ModuleName, "conf") == 0)
            {
                Bkupflag++;
                continue;
            }
            *CurrFMHLoc = CurrFwModHdr[i]->FmhLocation;
            break;
        }
    }

    /*To find FMH location in  image and it can be used as file read position, in case of split image*/
    for (i = 0; i < ModuleCount; i++)
    {
        if (strcmp((char *)FwModuleHdr[i]->ModuleName, ModuleName) == 0)
        {
            if (Bkupflag != 0 && strcmp(ModuleName, "conf") == 0)
            {
                Bkupflag = 0;
                continue;
            }
            if (strcmp(ModuleName, "conf") == 0)
                Bkupflag = 1;

            *FMHLocation = FwModuleHdr[i]->FmhLocation;
            *SizetoCopy = FwModuleHdr[i]->AllocatedSize;
            break;
        }
    }
    return 0;
}

/*
 * @fn ModuleUpgradeByUser
 * @brief This function is to Flash the Firmware module given by user in command line
 * @param hSession - Current Session Pointer
 *@param FlashModCnt - Module count given by user
 * @return Returns 0 on success, 1 on failure, -1 on failure and needs to restart device
 */
int NonInteractiveUpgrade(IPMI20_SESSION_T *hSession, int FlashModCnt)
{

#if defined(__x86_64__) || defined(WIN64)
    unsigned int SizetoCopy = 0;
#else
    unsigned long SizetoCopy = 0;
#endif

    int FMHFound, i = 0, j = 0, k = 0, count = 0, flag = 0, ret = -1, ret1 = 0;
    INT8U confflag = 0;
    INT32U CurrFMHLoc = 0, FMHLocation = 0;
    char *BootVarsName = NULL;
    char *ModName[8];
    /* To check module name given by user is available in image file */
    for (i = 0; i < FlashModCnt; i++, k++)
    {
        FMHFound = 0;
        confflag = 0;
        for (j = 0; j < ModuleCount; j++)
        {
            FwModuleHdr[j]->ModuleName[8] = (char)'\0';
            if (strncmp("conf", FwModuleHdr[j]->ModuleName, sizeof(FwModuleHdr[j]->ModuleName)) == 0)
                confflag++;

            if ((strcmp((char *)FwModuleHdr[j]->ModuleName, FlashModName[k]) == 0) || ((strncmp("conf", FwModuleHdr[j]->ModuleName, sizeof(FwModuleHdr[j]->ModuleName)) == 0) && (strncmp(FlashModName[k], "bkupconf", MAX_INPUT_LEN) == 0) && (confflag == 2)))
            {
                ModName[count] = (char *)malloc(strlen(FlashModName[k]) + 1);
                strncpy(ModName[count], FlashModName[k], strlen(FlashModName[k]) + 1);
                FMHFound = 1;
                count++;
            }
        }
        if (!FMHFound)
        {
            printf("Error: The given module [%s] is not found in image file\n", FlashModName[k]);
            return 1;
        }
    }

    if (count == 0)
        return 0;

    /* To check location of  module name given by user is available in SPI*/
    for (i = 0; i < count; i++)
    {
        FMHFound = 0;
        for (k = 0; k < ModCount; k++)
        {
            CurrFwModHdr[k]->ModuleName[8] = (char)'\0';
            if ((strcmp((char *)CurrFwModHdr[k]->ModuleName, ModName[i]) == 0))
            {
                FMHFound = 1;
            }

            if (FMHFound == 1)
                break;
        }

        if (FMHFound == 1)
        {
            for (j = 0; j < ModuleCount; j++)
            {
                if (spstrcasecmp((char *)FwModuleHdr[j]->ModuleName, (char *)CurrFwModHdr[k]->ModuleName) == 0)
                {
                    if (FwModuleHdr[j]->AllocatedSize != CurrFwModHdr[k]->AllocatedSize)
                    {
                        printf("The Module %s size is different from the one in the Image.\nplease do full firmware image flashing \n", (char *)CurrFwModHdr[k]->ModuleName);
                        return 1;
                    }
                }
            }
        }
    }

    if (NoOfImg == IMAGE_1)
    {
        WarningMessage();
    }

    if (ActivateFlashMode(hSession) != 0)
    {
        printf("\nError in activate flash mode \n");
        ret = 1;
    }

    if (SPIDevice == BMC_FLASH)
    {
        if (ReadRngFirmwareRelBase(hSession, &CurReleaseID[0], &CurCodeBaseVersionID[0]) == 0)
        {
            ImgOpt = CheckReleaseIDCodeVersion();
        }

        if ((ImgOpt == -1) && (Parsing.FlashModImg != 0))
        {
            printf("Preserving Individual Configurations are not allowed due to Code Base Version Mismatch\n");
            FreeFMHMemory();
            Close_Session(&hSession);
            return -1;
        }
    }

    for (i = 0; i < count; i++)
    {
        if (CalculateOffset(ModName[i], &SizetoCopy, &CurrFMHLoc, &FMHLocation) != 0)
        {
            continue;
        }

        if (strcmp(ModName[i], "boot") == 0)
        {
            BootVarsName = malloc(400);
            if (BootVarsName == NULL)
            {
                printf("Error in mem alloc of BootVars \n");
                return -1;
            }
            memset(BootVarsName, 0, 400);
            if (PreserveUbootEnvVariables(hSession, BootVarsName, &BootVarsCount) != 0)
            {
                printf("Error in PreserveEnvVariables \n");
                return -1;
            }
        }

        if (flag == 1 && strcmp(ModName[i], "conf") == 0)
            printf("Updating the secondary %s module....\r", ModName[i]);
        else
            printf("Updating the module %s....\r", ModName[i]);
        fflush(stdout);

        if (FlashModule(hSession, fp, SizetoCopy, CurrFMHLoc, FMHLocation, ModName[i]) != 0)
        {
            fclose(fp);
            return -1;
        }
        if (flag == 1 && strcmp(ModName[i], "conf") == 0)
            printf("Updating the secondary %s module.... done\n", ModName[i]);
        else
            printf("Updating the module %s.... done\n", ModName[i]);
        fflush(stdout);

        if ((strcmp(ModName[i], "boot") == 0) && (Parsing.versioncmpflash == 0x00))
        {
            if (SettingEnvUbootVariables(hSession, BootVarsName, &BootVarsCount) != 0)
            {
                printf("Error in SettingEnvUbootVariables \n");
                return -1;
            }
            BootVarsCount = 0;
            free(BootVarsName);
        }

        if (strcmp(ModName[i], "conf") == 0)
            flag = 1;
    }

    if (DeactivateFlshMode(hSession) != 0)
    {
        printf("\nError in Deactivate Flash mode");
        RestoreFlashDevice(&hSession);
        return -1;
    }

    if (NoOfImg == ImgCount)
    {
        fclose(fp);
        if (RestoreFlashDevice(hSession) < 0)
        {
            printf("Error in Closing the mapped flash info\n");
            return -1;
        }

/* if online flasjing support is enabled and conf is flashed then reboot BMC */
#ifdef CONFIG_SPX_FEATURE_GLOBAL_ON_LINE_FLASHING_SUPPORT
        if ((PreserveFlag & BIT1) == 0)
        {
            Parsing.RebootFirmware = 0x01;
        }
#endif

        /*If only boot alone flashed then shouldnt reboot BMC. This case will occur in split image */
        /*  if((PreserveFlag&BIT0) == 0)
             Parsing.RebootFirmware = 0x00;*/

        if (Parsing.RebootFirmware == 0x01)
        {

            if (((featuresList.mmc_boot_support) && (Parsing.DoMMCImgUpdate == 1) && (IsMMCUpdated == 0)))
            {
                ret1 = DoMMCUpdate(&hSession);
                if (ret1 != 0)
                {
                    printf("Error in Updating MMC Image\n");
                    return -1;
                }
                return 0;
            }
            else
            {
                ResetFunction(hSession);
            }
            return 0;
        }
    }
    for (i = 0; i < count; i++)
    {
        if (ModName[i] != NULL)
            free(ModName[i]);
    }

    return 0;
}

/*
 * @fn InteractiveUpgrade
 * @brief This function is to Flash the Firmware by interactively
 * @param hSession - Current Session Pointer
 * @param imgName -' Name of Image to Flash
 * @return Returns 0 on success
 */
int InteractiveUpgrade(IPMI20_SESSION_T *hSession, char *imgName, FlashMH *CurrFirmModHdr[], int NumFMH, int ImageDiffer)
{
    int i = 0, j = 0, ret1 = 0;
    int ChoiceNo;
    char ModuleNo[15] = {0};
    int Module = 0;
    int UpdateConfig = 0, UpdateBootloader = 0;
    int modfound = 0, ModSelected = 0;
    char Confchoice[MAX_INPUT_LEN] = {0}, FlashChoice[MAX_INPUT_LEN] = {0}, ModChoice[MAX_INPUT_LEN] = {0};
    int Isalpha = 0;
    int confflag = 0;
    if (NoOfImg == IMAGE_1)
    {
        if (DisplayFirmwareInfo() != 0)
        {
            return -1;
        }
    }

    if ((ModuleCount != NumFMH) || (ImageDiffer == 0x01))
    {
        if ((DualImageSup == TRUE) && (FlashBothImg == TRUE))
        {
            printf("The Image Modules count are different for Image -%d\nSo,", NoOfImg);
        }
        else
        {
            printf("The Image Modules count are different for Image \nSo,");
        }
        if (Parsing.DiffImage == 0)
        {
            do
            {
                printf("Type (Y/y) to do Interactive Firmware Upgrade or (N/n) to exit\n");
                printf("Enter your Option : ");
                scanf(" %[^\n]%*c", Confchoice);
                if (strcmp(Confchoice, "y") == 0 || strcmp(Confchoice, "Y") == 0)
                {
                    break;
                }
                if (strcmp(Confchoice, "n") == 0 || strcmp(Confchoice, "N") == 0)
                {
                    fclose(fp);
                    exit(0);
                }
                printf("Invalid Option\n");
            } while (1);
        }
        else
        {
            printf("Continuing with Firmware Update \n");
        }
    }
    else
    {
        if ((DualImageSup == TRUE) && (FlashBothImg == TRUE))
        {
            printf("The Image Modules count are Same for Image -%d\n", NoOfImg);
        }
    }

    if (Parsing.InteractiveFull == 0)
    {
        while (1)
        {
            Isalpha = 0;
            printf("For Full Firmware upgrade,Please  type (0) alone\n");
            printf("For Module Upgrade enter the total no. of Modules to Upgrade \n");
            printf("Enter your choice : ");
            scanf(" %[^\n]%*c", FlashChoice);
            for (i = 0; FlashChoice[i] != '\0'; i++)
            {
                if (isalpha(FlashChoice[i]))
                {
                    Isalpha = 1;
                    break;
                }
            }
            if (Isalpha == 1)
            {
                printf("%s is invalid\n", FlashChoice);
            }
            else
            {
                ChoiceNo = atoi(FlashChoice);
                if ((ChoiceNo == 0) || (ChoiceNo >= 1 && ChoiceNo <= ModuleCount))
                {
                    break;
                }
                else if (ChoiceNo > ModuleCount)
                {
                    printf("\nModules to update is greater than modules present. \n");
                    printf("Enter valid no: of  modules to update \n");
                    continue;
                }
                else
                {
                    printf("Invalid Option\n");
                }
            }
        }
    }
    else
    {
        printf("Continuing with Full Firmware Upgrade in Interactive Mode \n");
        ChoiceNo = 0;
    }

    if (ChoiceNo == 0)
    {
        for (i = 0; i < ModuleCount; i++)
        {
            if ((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "conf")) == 0 ||
                (spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "params")) == 0)
                UpdateConfig = 0x01;
            if ((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "boot")) == 0)
                UpdateBootloader = 0x01;
        }
        if (UpdateConfig == 0x01)
        {
            if (Parsing.ConfigPreserve == 1)
            {
                Config = 0x00;
            }
            else if (Parsing.ConfigPreserve == 0)
            {
                do
                {
                    printf("Please type (Y/y) to preserve config or (N/n) for not preserving config : ");
                    scanf(" %[^\n]%*c", Confchoice);
                    if (strcmp(Confchoice, "y") == 0 || strcmp(Confchoice, "Y") == 0)
                    {
                        Config = 0x00;
                        Parsing.ConfigPreserve = 1;
                        break;
                    }
                    if (strcmp(Confchoice, "n") == 0 || strcmp(Confchoice, "N") == 0)
                    {
                        Config = 0x01;
                        break;
                    }
                    printf("Invalid Option\n");
                } while (1);
            }
        }

        if (UpdateBootloader == 0x01)
        {
            if (Parsing.BootPreserve == 0)
            {
                Boot = 0x01;
            }
            else if (Parsing.BootPreserve == 1)
            {
                do
                {
                    printf("Please type (Y/y) to preserve Bootloader or (N/n) for not preserving Bootloader : ");
                    scanf(" %[^\n]%*c", Confchoice);
                    if (strcmp(Confchoice, "y") == 0 || strcmp(Confchoice, "Y") == 0)
                    {
                        Boot = 0x00;
                        break;
                    }
                    if (strcmp(Confchoice, "n") == 0 || strcmp(Confchoice, "N") == 0)
                    {
                        Boot = 0x01;
                        Parsing.BootPreserve = 0x00;
                        break;
                    }
                    printf("Invalid Option\n");
                } while (1);
            }
        }

        for (i = 0; i < ModuleCount; i++)
        {
            if (((Boot == 0) && (spstrcasecmp(FwModuleHdr[i]->ModuleName, "boot") == 0)) ||
                ((Config == 0) && ((spstrcasecmp(FwModuleHdr[i]->ModuleName, "conf") == 0) ||
                                   (spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "params")) == 0)))
            {
                // Either Boot or conf module which user don't want to update.
                continue;
            }

            for (j = 0; j < NumFMH; j++)
            {
                if (spstrncasecmp(FwModuleHdr[i]->ModuleName, CurrFirmModHdr[j]->ModuleName, sizeof(FwModuleHdr[i]->ModuleName)) == 0)
                {
                    break;
                }
            }
            if (j == NumFMH)
            {
                if ((DualImageSup == TRUE) && (FlashBothImg == TRUE))
                {
                    printf("Ignoring Module \"%s\" as it is not available in current Flash Image-%d.\n", FwModuleHdr[i]->ModuleName, NoOfImg);
                }
                else
                {
                    printf("Ignoring Module \"%s\" as it is not available in current Flash Image \n", FwModuleHdr[i]->ModuleName);
                }
            }
            else
            {
                if (vdbg)
                    printf("Included Module %s in number %d\n", FwModuleHdr[i]->ModuleName, i + 1);
                ModuleNo[ChoiceNo] = i + 1; // Modules starts from 1.
                ChoiceNo++;
            }
        }
        if (ChoiceNo == 0)
        {
            if (FlashBothImg == TRUE)
            {
                printf("No Module to Update..in Image-%d\n", NoOfImg);
            }
            else
            {
                printf("No Module to Update..\n");
            }
            fclose(fp);
            return 0;
        }
    }
    else
    {
        for (i = 0; i < ChoiceNo; i++)
        {
            while (1)
            {
                printf("Enter the Module Name to Update : ");
                scanf(" %[^\n]%*c", ModChoice);

                for (j = 0, confflag = 0; j < ModuleCount; j++)
                {
                    if (strncmp("conf", FwModuleHdr[j]->ModuleName, sizeof(FwModuleHdr[j]->ModuleName)) == 0)
                        confflag++;
                    if ((strncmp(ModChoice, FwModuleHdr[j]->ModuleName, sizeof(FwModuleHdr[j]->ModuleName)) == 0) || ((strncmp(ModChoice, "bkupconf", MAX_INPUT_LEN) == 0) && (strncmp("conf", FwModuleHdr[j]->ModuleName, sizeof(FwModuleHdr[j]->ModuleName)) == 0) && (confflag == 2)))
                    {
                        ModSelected = j + 1; // Modules starts from 1
                        break;
                    }
                }
                if (j == ModuleCount)
                {
                    printf("Please enter a valid module Name to update \n");
                    continue;
                }
                else
                {
                    for (j = 0; j < i; j++)
                    {
                        if (ModuleNo[j] == ModSelected)
                        {
                            modfound = 1;
                        }
                    }
                    if (modfound == 0)
                    {
                        ModuleNo[i] = ModSelected;
                        break;
                    }
                    else
                    {
                        printf("Module Already selected, Please select other Module\n");
                        modfound = 0;
                        continue;
                    }
                }
            }
        }
    }

    if (NoOfImg == IMAGE_1)
    {
        WarningMessage();
    }

    if (ActivateFlashMode(hSession) != 0)
    {
        printf("\nError in activate flash mode \n");
        return -1;
    }

    if (SPIDevice == BMC_FLASH)
    {
        if (ReadRngFirmwareRelBase(hSession, &CurReleaseID[0], &CurCodeBaseVersionID[0]) == 0)
        {
            ImgOpt = CheckReleaseIDCodeVersion();
        }
    }

    for (i = 0, g_confflag = 0; i < ChoiceNo; i++)
    {
        Module = ModuleNo[i] - 1;
        for (j = 0; j < Module; j++)
        {
            if (strncmp("conf", FwModuleHdr[j]->ModuleName, sizeof(FwModuleHdr[Module]->ModuleName)) == 0)
                g_confflag++;
        }
        if (ModLocationChkAndUpgrade(hSession, Module, i, ChoiceNo) != 0)
        {
            fclose(fp);
            return -1;
        }
    }

    if (DeactivateFlshMode(hSession) != 0)
    {
        printf("\nError in Deactivate Flash mode");
        return -1;
    }

    if (NoOfImg == ImgCount)
    {
        fclose(fp);
        if (Parsing.RebootFirmware == 0x01)
        {
            if (((featuresList.mmc_boot_support) && (Parsing.DoMMCImgUpdate == 1) && (IsMMCUpdated == 0)))
            {
                ret1 = DoMMCUpdate(&hSession);
                if (ret1 != 0)
                {
                    printf("Error in Updating MMC Image\n");
                    return -1;
                }
                return 0;
            }
            else
            {
                ResetFunction(hSession);
            }
        }
    }
    return 0;
}
/*
 *@fn Platformchecking
 *@brief This Function Compares the Both pathform of Exiting and New Firmware
 *@Return 0 when Success
 */
int Platformchecking(FlashMH *CurrentFwModHdr[], int NumFMH)
{
    int Count;
    int FirmModFound = 0;
    char FlashFirmModName[15], ImageFirmModName[15];
    char FlashChoice[MAX_INPUT_LEN];

    memset(&FlashFirmModName[0], 0, sizeof(FlashFirmModName));
    memset(&ImageFirmModName[0], 0, sizeof(ImageFirmModName));

    for (Count = 0; Count < ModuleCount; Count++)
    {
        if (FwModuleHdr[Count]->ModuleType == MODULE_FMH_FIRMWARE)
        {
            strncpy(ImageFirmModName, FwModuleHdr[Count]->ModuleName, 8);
        }
    }

    if (NumFMH > 0)
    {
        for (Count = 0; Count < NumFMH; Count++)
        {
            if (CurrentFwModHdr[Count]->ModuleType == MODULE_FMH_FIRMWARE)
            {
                FirmModFound = 0x01;
                strncpy(FlashFirmModName, CurrentFwModHdr[Count]->ModuleName, 8);
            }
        }
    }

    if (NumFMH > 0)
    {
        for (Count = 0; Count < NumFMH; Count++)
        {
            if (CurrentFwModHdr[Count]->ModuleType == 0x02)
            {
                FirmModFound = 0x01;
                strncpy(FlashFirmModName, CurrentFwModHdr[Count]->ModuleName, 8);
                if ((spstrcasecmp(FlashFirmModName, ImageFirmModName) == 0))
                {
                    if ((NewImageSize > FlashSize) && (SignedImageSup == 0))
                    {
                        printf("The Rom Image size = %d MB\n", (int)(NewImageSize / 1048576));
                        printf("The Current Fw  size = %d MB\n", (int)(FlashSize / 1048576));
                        printf("The Image Sizes is greater than flash size \n");
                        return -1;
                    }
                    return 0;
                }
            }
        }
    }

    if (Parsing.Interactive == 0x01)
    {
        // Interactive update, so there is a chance of difference in platform
        printf("WARNING !!! Image to be flashed is for Platform %s \n", ImageFirmModName);
        if (FirmModFound == 0x01)
        {
            if ((FlashBothImg == TRUE) && (DualImageSup == TRUE))
            {
                printf("WARNING !!! Image-%d in the flash is for Platform %s \n", NoOfImg, FlashFirmModName);
            }
            else
            {
                printf("WARNING !!! Image in the flash is for Platform %s \n", FlashFirmModName);
            }
        }
        if (Parsing.PlatformCheck == 0)
        {
            while (1)
            {
                printf("Type (Y/y) to Proceed Firmware Upgrade or (N/n) to exit\n");
                printf("Enter your Option : ");
                scanf(" %[^\n]%*c", FlashChoice);
                if (spstrcasecmp(FlashChoice, "y") == 0)
                {
                    return 0;
                }
                else if (spstrcasecmp(FlashChoice, "n") == 0)
                {
                    return -1;
                }
                else
                {
                    printf("Please Enter a valid option \n");
                    continue;
                }
            }
        }
        else
        {
            printf("Continuing with Firmware Update \n");
            return 0;
        }
    }

    if (NoOfImg == IMAGE_1)
    {
        DisplayFirmwareInfo();
    }

    printf("WARNING !!! Image to be flashed is for Platform %s \n", ImageFirmModName);
    if (FirmModFound == 0x01)
    {
        if ((FlashBothImg == TRUE) && (DualImageSup == TRUE))
        {
            printf("WARNING !!! Image-%d in the flash is for Platform %s \n", NoOfImg, FlashFirmModName);
        }
        else
        {
            printf("WARNING !!! Image in the flash is for Platform %s \n", FlashFirmModName);
        }
    }
    else if (NumFMH > 0)
    {
        printf("WARNING !!! Image in the flash is corrupted \n");
    }

    if (Parsing.PlatformCheck == 0)
    {
        while (1)
        {
            printf("Type (Y/y) to do Full Firmware Upgrade or (N/n) to exit\n");
            printf("Enter your Option : ");
            scanf(" %[^\n]%*c", FlashChoice);
            if (spstrcasecmp(FlashChoice, "y") == 0)
            {
                Parsing.Full = 0x01;
                Parsing.BootPreserve = Parsing.ConfigPreserve = 0x00;
                break;
            }
            else if (spstrcasecmp(FlashChoice, "n") == 0)
                exit(0);
            else
            {
                printf("Please Enter a valid option \n");
                continue;
            }
        }
    }
    else
    {
        printf("Continuing with Full Firmware Update \n");
        Parsing.Full = 0x01;
        Parsing.BootPreserve = Parsing.ConfigPreserve = 0x00;
    }
    return 0;
}

/*
 *@fn ConfirmDualImageSupport
 *@brief Checks for the Dual Image Support
 *@param hSession - Current Session Pointer
 *@return Returns 0 when Dual Image Support is found
 *@return Returns -1 if Dual Image support is not found
 */
int ConfirmDualImageSupport(IPMI20_SESSION_T *hSession)
{
    int Parameter = 0, DualImageRes = 0;

    Parameter = GETCURACTIVEIMG;
    if (GetDualImageSupport(hSession, Parameter, &DualImageRes) != 0)
    {
        return -1;
    }

    return 0;
}

/*
 *@fn Toggling of FMH's to manipulate
 *@brief Helps to set configuration value of each preserve-option
 *@param hSession - Current Session Pointer
 *@return Returns '0' on success
 *            Returns '-1' on failure
 */
void ToggleDualFMHInfo()
{
    FlashMH *ToggleInfo[MAX_MODULE];
    int NumFMH = 0, MaxFMH = 0;

    MaxFMH = FlashFMH > DualFMH ? FlashFMH : DualFMH;

    for (NumFMH = 0; NumFMH < MaxFMH; NumFMH++)
    {
        if (CurrFwModHdr[NumFMH] != NULL)
        {
            ToggleInfo[NumFMH] = CurrFwModHdr[NumFMH];
            CurrFwModHdr[NumFMH] = DualFwModHdr[NumFMH];
            DualFwModHdr[NumFMH] = ToggleInfo[NumFMH];
            if (((NumFMH == (FlashFMH - 1)) && (FlashFMH > DualFMH)))
            {
                CurrFwModHdr[NumFMH] = NULL;
            }
        }
        else if (DualFwModHdr[NumFMH] != NULL)
        {
            CurrFwModHdr[NumFMH] = DualFwModHdr[NumFMH];
            DualFwModHdr[NumFMH] = NULL;
        }
    }

    NumFMH = FlashFMH;
    FlashFMH = DualFMH;
    DualFMH = NumFMH;

    NumFMH = ImageDiffer1;
    ImageDiffer1 = ImageDiffer2;
    ImageDiffer2 = NumFMH;

    ActiveCurImg = IMAGE_1;
    InActiveImg = IMAGE_2;
}

/*
 *@fn GetDualImageConfig
 *@brief Helps in retreiving Dual Image Configuration
 *@param hSession - Current Session Pointer
 *@return Returns '0' on success
 *            Returns '-1' on failure
 */
int GetDualImageConfig(IPMI20_SESSION_T *hSession)
{
    int Parameter = 0, DualImageRes = 0, Ret = 0;
    int FwImgSelect = 0;
    char Choice[MAX_INPUT_LEN] = {0};

    Parameter = GETFWUPLOADSELECTOR;

    if (GetDualImageSupport(hSession, Parameter, &FwUploadImg) == 0)
    {
        if ((Parsing.ImgSelect != -1)) // && (FwUploadImg != Parsing.ImgSelect))
        {
            FwUploadImg = Parsing.ImgSelect;
            if (Parsing.ImgSelect == IMAGE_BOTH)
            {
                FlashBothImg = TRUE;
                ImgCount = FLASH_BOTH_IMAGES;
                FwImgSelect = IMAGE_BOTH;
                Parsing.BootPreserve = 0x00;
            }
            else
            {
                FwImgSelect = Parsing.ImgSelect;
            }
            Parameter = SETFWUPLOADSELECTOR;
            if (SetDualImageConfig(hSession, Parameter, FwImgSelect) != 0)
            {
                printf("Error in setting Dual Image Configuration \n");
                Ret = -1;
                goto exit;
            }
        }
        else
            FwUploadImg = AUTO_INACTIVE_IMAGE; // default flash inactive image.

        if (FwUploadImg == IMAGE_1)
        {
            printf("Image To be updated is (Image-1)\n");
        }
        else if (FwUploadImg == IMAGE_2)
        {
            printf("Image to be updated is (Image-2)\n");
        }
        else if (FwUploadImg == IMAGE_BOTH)
        {
            printf("Both the images will be updated\n");
        }
        else if (FwUploadImg == AUTO_INACTIVE_IMAGE)
        {
            Parameter = GETCURACTIVEIMG;
            Ret = GetDualImageSupport(hSession, Parameter, &DualImageRes);
            if (Ret == 0)
            {
                if (DualImageRes == IMAGE_1)
                {
                    printf("Image to be updated is Inactive Image (Image-2)\n");
                }
                else if (DualImageRes == IMAGE_2)
                {
                    printf("Image to be updated is Inactive Image (Image-1)\n");
                }
            }
        }

        if (((non_interactive) || (Parsing.ImgSelect != -1)) || (Parsing.ReselectImage))
        {
            Ret = 0;
            goto exit;
        }
        while (1)
        {
            printf("Please type (Y/y) to proceed or (N/n) to change the Image:");
            scanf(" %[^\n]%*c", Choice);
            if ((spstrcasecmp(Choice, "y") == 0) || (spstrcasecmp(Choice, "n") == 0))
            {
                break;
            }
        }

        if (spstrcasecmp(Choice, "n") == 0)
        {
            while (1)
            {
                Parameter = GETCURACTIVEIMG;
                Ret = GetDualImageSupport(hSession, Parameter, &DualImageRes);
                if (Ret == 0)
                {
                    if (DualImageRes == IMAGE_1)
                    {
                        printf("Please type Option '0' : to Flash Inactive Image Image-2\n");
                    }
                    else if (DualImageRes == IMAGE_2)
                    {
                        printf("Please type Option '0' to Flash Inactive Image Image-1\n");
                    }
                    printf("%36s'1' : to Flash Image-1 \n", "");
                    printf("%36s'2' : to Flash Image-2 \n", "");
                    printf("%36s'3' : to Flash both Images :", "");
                    scanf(" %[^\n]%*c", Choice);
                    if (spstrcasecmp(Choice, "0") == 0)
                    {
                        DualImageReq = AUTO_INACTIVE_IMAGE;
                    }
                    else if (spstrcasecmp(Choice, "1") == 0)
                    {
                        DualImageReq = IMAGE_1;
                    }
                    else if (spstrcasecmp(Choice, "2") == 0)
                    {
                        DualImageReq = IMAGE_2;
                    }
                    else if (spstrcasecmp(Choice, "3") == 0)
                    {
                        DualImageReq = IMAGE_BOTH; // Set the Image Req to be 1 as we need to start flashing from 1'st image
                        FlashBothImg = TRUE;
                        ImgCount = FLASH_BOTH_IMAGES;
                        Parsing.BootPreserve = 0x00;
                    }
                    else
                    {
                        continue;
                    }
                    Parameter = SETFWUPLOADSELECTOR;
                    if (SetDualImageConfig(hSession, Parameter, DualImageReq) != 0)
                    {
                        printf("Error in setting Dual Image Configuration \n");
                        Ret = -1;
                        goto exit;
                    }
                    FwUploadImg = DualImageReq;
                    break;
                }
                else
                {
                    printf("Error in Retrieving Dual Image Configurations \n");
                    Ret = -1;
                    goto exit;
                }
            }
        }
        else
        {
            Parameter = SETFWUPLOADSELECTOR;
            DualImageReq = AUTO_INACTIVE_IMAGE;
            if (SetDualImageConfig(hSession, Parameter, DualImageReq) != 0)
            {
                printf("Error in setting Dual Image Configuration \n");
                Ret = -1;
                goto exit;
            }
            FwUploadImg = DualImageReq;
        }
    }
    else if (spstrcasecmp(Choice, "y") == 0)
    {
        Ret = 0;
        goto exit;
    }
    else
    {
        printf("Error in Retrieving Dual Image Configurations \n");
        Ret = -1;
        goto exit;
    }

    /*runningImg:
            #ifdef CONFIG_SPX_FEATURE_INDIVIDUAL_CONF_SECTION
            if((Parsing.RebootFirmware == 0x00) && (Parsing.ImgSelect == 1 || DualImageReq == 1))
            {
                printf("Going to flash active image... \n");
                while(1)
                {
            if(!non_interactive)
            {
                        printf("Firmware will reboot after flashing... If you want to proceed type (Y/y) or (N/n) to exit \n");
                        scanf("%s",Choice);
                        if((spstrcasecmp(Choice,"y") == 0) || (spstrcasecmp(Choice,"n") == 0))
                        {
                             break;
                        }
            }
            else
            {
                strcpy(Choice,"y");
                printf("Firmware will reboot after flashing... \n");
                break;
            }
                }

                if(spstrcasecmp(Choice,"y") == 0)
                    Parsing.RebootFirmware = 0x01;
                else
                {
                    exit(0);
                }
            }
            #endif */

exit:
    return Ret;
}

/*
 *@fn FreeFMHMemory
 *@brief Helps to Free Up Memory
 */
void FreeFMHMemory()
{
    int NumFMH = 0, MaxFMH = 0;

    MaxFMH = ModuleCount > FlashFMH ? ModuleCount : FlashFMH;
    MaxFMH = MaxFMH > DualFMH ? MaxFMH : DualFMH;

    for (NumFMH = 0; NumFMH < MaxFMH; NumFMH++)
    {
        if (FwModuleHdr[NumFMH] != NULL)
        {
            free(FwModuleHdr[NumFMH]);
            FwModuleHdr[NumFMH] = NULL;
        }
        if (CurrFwModHdr[NumFMH] != NULL)
        {
            free(CurrFwModHdr[NumFMH]);
            CurrFwModHdr[NumFMH] = NULL;
        }
        if (DualFwModHdr[NumFMH] != NULL)
        {
            free(DualFwModHdr[NumFMH]);
            DualFwModHdr[NumFMH] = NULL;
        }
        if (PrimFwModHdr[NumFMH] != NULL)
        {
            free(PrimFwModHdr[NumFMH]);
            PrimFwModHdr[NumFMH] = NULL;
        }
    }

    if (ImgOpt == -1)
    {
        if (env.data != NULL)
        {
            free(env.data);
        }
    }

#ifdef ICC_OS_LINUX
    LoadOpenIPMIDrivers();
#endif
}

/*
 *@fn CheckPreserveConfStatus
 *@brief Helps to set configuration value of each preserve-option
 *@param hSession - Current Session Pointer
 *@return Returns '0' on success
 *            Returns '-1' on failure
 */

int CheckPreserveConfStatus(IPMI20_SESSION_T *hSession)
{
    unsigned short status = 0, tempstatus = 0;
    int retval, tempmask = 0;
    char choice[MAX_INPUT_LEN];
    int i = 0;
    unsigned short enabledstatus = 0;
    YafuPreserveConf_T AMIYafu_PreserveConf[] =
        {
            {"SDR", 1},
            {"FRU", 1},
            {"SEL", 1},
            {"IPMI", 1},           // have to preserve all INI files and IPMI.conf files
            {"Network", 1},        // to preserve all network related files
            {"NTP", 1},            // to preserve all NTP related files
            {"SNMP", 1},           // to preserve all SNMP related files
            {"SSH", 1},            // to preserve all SSH related files
            {"KVM", 1},            // to preserve KVM & Vmedia related files
            {"Authentication", 1}, // to preserve Authentication related files
            {"Syslog", 1},         // to preserve Syslog related files
            {"CMX", 1},
            {"Web", 1},
            {"EXTLOG", 1}};

    retval = GetAllPreserveConfStatus(hSession, &status, &enabledstatus);
    if (0 != retval)
    {
        if (-1 == retval && 1 == vdbg)
        {
            printf("\n File Override Feature not Supported, Exiting \n");
        }

        return retval;
    }

    for (i = 0; i < (sizeof(AMIYafu_PreserveConf) / sizeof(AMIYafu_PreserveConf[0])); i++)
    {
        tempmask = ((Parsing.ConfigPreserveBitMask >> i) & 0x1);
        tempstatus = ((enabledstatus >> i) & 0x1);
        if (0 == tempstatus && 1 == tempmask)
        {
            AMIYafu_PreserveConf[i].Status = 0;
            if (Parsing.IgnoreExisting != 0x01)
            {
                strcpy(choice, "y");
                if (!non_interactive)
                {
                    printf(" Preserve %s configuration option not supported. Type (Y/y) to Continue or (N/n) to exit : ", AMIYafu_PreserveConf[i].Config_Name);
                    scanf(" %[^\n]%*c", choice);
                }

                if (spstrcasecmp(choice, "y") == 0)
                {
                    Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask ^ (1 << i);
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                Parsing.ConfigPreserveBitMask = Parsing.ConfigPreserveBitMask ^ (1 << i);
            }
        }
    }

    if (Parsing.IgnoreExisting != 0x01)
    {
        for (i = 0; i < (sizeof(AMIYafu_PreserveConf) / sizeof(AMIYafu_PreserveConf[0])); i++)
        {
            tempstatus = ((status >> i) & 0x1);
            if (tempstatus)
            {
                tempmask = ((Parsing.ConfigPreserveBitMask >> i) & 0x1);
                if (0 == tempmask && 1 == AMIYafu_PreserveConf[i].Status)
                {
                    strcpy(choice, "n");
                    if (!non_interactive)
                    {
                        printf(" %s is already Configured to be Preserved. Type (Y/y) to Confirm or (N/n) to Skip : ", AMIYafu_PreserveConf[i].Config_Name);
                        scanf(" %[^\n]%*c", choice);
                    }

                    if (spstrcasecmp(choice, "n") == 0)
                    {
                        status = status ^ (1 << i);
                    }
                }
            }
        }
        Parsing.ConfigPreserveBitMask = status | Parsing.ConfigPreserveBitMask;
    }

    retval = SetAllPreserveConfStatus(hSession, Parsing.ConfigPreserveBitMask);
    if (0 != retval)
    {
        return retval;
    }

    return 0;
}

/*

*@fn Check_DualImagebootVersions
*@brief used to check both images boot versions in dual image case.
*@param hSession - Current Session Pointer
*@return Returns 1 - if boot versions mis-matches.
*                0 - boot versions identical.
*/
int Check_DualImagebootVersions(IPMI20_SESSION_T *pSession)
{
    int upimg = -1, actimg = -1, flashimg = -1;
    unsigned char doFlashBothFWImg = FALSE;

    if ((FwUploadImg == AUTO_INACTIVE_IMAGE) || (FwUploadImg == IMAGE_2))
    {
        if (strlen(Img2BootVer) == 0)
            Parsing.BootPreserve = 0x00;
    }

    if (FwUploadImg != IMAGE_BOTH)
    {

        if (GetDualImageSupport(pSession, GETFWUPLOADSELECTOR, &upimg) == 0)
        {
            if (upimg == AUTO_INACTIVE_IMAGE)
            {
                if (GetDualImageSupport(pSession, GETCURACTIVEIMG, &actimg) == 0)
                {
                    if (actimg == IMAGE_1)
                        flashimg = IMAGE_2;
                    else if (actimg == IMAGE_2)
                        flashimg = IMAGE_1;
                }
            }
            else
                flashimg = upimg;
        }

        if (flashimg == IMAGE_1)
        {
            /* Image1 bootloader is common for both images,if new image bootloader verion varies need to flash both images*/
            if (strcmp(Img1BootVer, NewImgBootVer))
                doFlashBothFWImg = TRUE;
        }
        else if (flashimg == IMAGE_2)
        {
            if (strlen(Img1BootVer) == 0)
            {
                /* both firmwares should be flashed if image1 bootloader corrupts*/
                doFlashBothFWImg = TRUE;
            }

            /* both firmwares should be flashed if image1 bootloader and  new image bootloader verions mismatches*/
            if (strcmp(Img1BootVer, NewImgBootVer))
                doFlashBothFWImg = TRUE;
        }

        /* Both images present */
        if (strlen(Img1BootVer) && strlen(Img2BootVer))
        {
            /* flashing both images if boot version mismatches with  any of firmware */
            if ((strcmp(Img1BootVer, NewImgBootVer)) || (strcmp(Img2BootVer, NewImgBootVer)))
                doFlashBothFWImg = TRUE;
        }
    } // end of  if(FwUploadImg != IMAGE_BOTH)

    return doFlashBothFWImg;
}

int Close_Session(IPMI20_SESSION_T *pSession)
{
#ifndef MSDOS
    if (LIBIPMI_CloseSession(pSession) < 0)
    {
        printf("Error in Closing the IPMI Session\n");
        return -1;
    }
#endif
    return 0;
}

/*
 * @fn RetrieveFeatureList
 * @brief this function is used to Get all Feature Status supported by BMC.
 * @param hSession - Current Session Pointer
 * @return Returns - Non
 */
int RetrieveFeatureList(IPMI20_SESSION_T *pSession)
{
    short int extstatus = 0;
    featuresList.dual_image_support = GetFeatureStatus(pSession, "CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT");
    featuresList.common_conf = GetFeatureStatus(pSession, "CONFIG_SPX_FEATURE_COMMON_CONF_SECTION");
    featuresList.full_firmware_upgrade_wt_version_cmp = GetFeatureStatus(pSession, "CONFIG_SPX_FEATURE_FULL_FIRMUP_WITH_VERSION_CMP");
    featuresList.fwupdate_section_based_flash = GetFeatureStatus(pSession, "CONFIG_SPX_FEATURE_SECTION_BASED_FLASHING");
    featuresList.do_not_upgrade_both_firmware_on_uboot_mismatch = GetFeatureStatus(pSession, "CONFIG_SPX_FEATURE_DO_NOT_UPGRADE_BOTH_FIRMWARE_ON_UBOOT_MISMATCH");
    featuresList.signed_hashed_image_support = GetFeatureStatus(pSession, "CONFIG_SPX_FEATURE_SIGNED_HASHED_IMAGE_SUPPORT");
    featuresList.online_flashing_support = GetFeatureStatus(pSession, "CONFIG_SPX_FEATURE_GLOBAL_ON_LINE_FLASHING_SUPPORT");
    featuresList.preserve_config = GetFeatureStatus(pSession, "CONFIG_SPX_FEATURE_PRESERVE_CONF_SUPPORT");
    featuresList.extendedlog_support = GetFeatureStatus(pSession, "CONFIG_SPX_FEATURE_EXTENDEDLOG_SUPPORT");
    featuresList.mmc_boot_support = GetFeatureStatus(pSession, "CONFIG_SPX_FEATURE_MMC_BOOT");

    if (featuresList.preserve_config && featuresList.extendedlog_support)
    {
        extstatus = GetPreserveConfStatus(pSession, EXTLOGPARAM);
        if (extstatus != 0 && ((Parsing.ConfigPreserveBitMask >> EXTLOGPARAM) & 1))
        {
            Parsing.ConfigPreserveBitMask &= 0xDFFF; // section for extlog not exist in firmware.
            Parsing.ExtlogPreserve = 0x01;
        }
        else if (extstatus == 0)
        {
            extflag = 1;
            PreserveFlag &= 0xFD; // Preserve Config BIT1 has to be Masked
        }
    }
}

Update_Parse_Configurable_Arguments()
{
    int position = 0;

    if (Parsing.RebootFirmware != 0x01)
    {
        if ((featuresList.dual_image_support) && (featuresList.online_flashing_support))
        {
            printf("Warnig!: The BMC Will not be rebooted if the online flashing support is enabled in Firmware \n");
            Parsing.RebootFirmware == 0x0;
        }
        else
        {
            Parsing.RebootFirmware == 0x01;
        }
    }

    if (Parsing.SkipFMHCheck != 0x00)
    {
        if (!featuresList.dual_image_support)
        {
            printf(" ' -skip-fmh ' option is supported only when the dual Image support is enabled in Firmware\n");
            exit(0);
        }
    }

    if (Parsing.SkipCRCValidation != 0x00)
    {
        if (!featuresList.dual_image_support)
        {
            printf(" ' -skip-crc' option is supported only when the dual Image support is enabled in Firmware\n");
            exit(0);
        }
    }

    if (Parsing.ConfigPreserve == 0x01)
    {
        if (!featuresList.preserve_config)
        {
            printf(" 'preserve-xxx' option is supported only when the preserveconfig support is enabled in Firmware\n");
            exit(0);
        }
    }

    if (Parsing.ConfigPreserveBitMask != 0)
    {
        if (!featuresList.preserve_config)
        {
            printf(" 'preserve-xxx' option is supported only when the preserveconfig support is enabled in Firmware\n");
            exit(0);
        }
    }

    if (Parsing.ExtlogPreserve == 0x01 || ((Parsing.ConfigPreserveBitMask >> EXTLOGPARAM) & 1))
    {
        if (!featuresList.extendedlog_support)
        {
            printf(" '-preserve-extlog '  option will preserve extlog , only when the extendedlog  is enabled in Firmware\n");
            exit(0);
        }
    }
}

int CheckReleaseIDCodeVersion(void)
{
    // just to handle firmware module not having FW_RELEASEID & FW_BASECODEVERSIONID
    if ((strlen(UpCodeBaseVersionID) == 0) && (spstrncasecmp(CurCodeBaseVersionID, "3.X", 3) == 0))
    {
        return -1;
    }
    else if ((strlen(CurCodeBaseVersionID) == 0) && (spstrncasecmp(UpCodeBaseVersionID, "3.X", 3) == 0))
    {
        return -1;
    }
    else if ((strlen(UpCodeBaseVersionID) != 0) && (strlen(CurCodeBaseVersionID) != 0) &&
             (spstrncasecmp(CurCodeBaseVersionID, UpCodeBaseVersionID, strlen(CurCodeBaseVersionID)) != 0))
    {
        return -1;
    }

    return 0;
}

void print_MMCUpdateInfo()
{
    printf("\n");
    printf("-------------------------------------------------\n");
    printf("YAFUFlash -Updating MMC Image\n ");
    printf("-------------------------------------------------\n");
    printf("\n");
}

int DoMMCUpdate(IPMI20_SESSION_T *hSession)
{

    UINT32 SizetoAlloc = 0;
    int ret1 = 0, i = 0;
    INT32U UpgradedBytes = 0, SizeToFlash = 0, Offset = 0, seekpos = 0;
    UINT16 Datalength = 0;
    unsigned long SizeToRead = 0, WriteMemOff = 0;
    int count = 0;
    int bytesread = 0, rembytes;
    char *Buf;
    unsigned long long int offset1 = 0;
    unsigned long long int NewImageSize1 = 0;
    DOMMCUpdate = 1;

    print_MMCUpdateInfo();

    SPIDevice = 8; // For MMC Image Update
    if (SwitchFlashDevice(hSession, &EraseBlkSize, &FlashSize) < 0)
    {
        printf("Error in identifying the Flash information\n");
#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif
        Close_Session(hSession);
        return -1;
    }

    if (GetImageSize(Parsing.MMCImgName) != 0)
    {
        printf("Error in reading the file\n");
#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif
        exit(-1);
    }
    if (SaveFwFile() != 0)
    {
        printf("Error while parsing the file\n");
#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif
        return -1;
    }

    if (ActivateFlashMode(hSession) != 0) // my change
    {
        printf("Error in activate flash mode\n");
        Close_Session(hSession);
        printf("Freeing up memory4\n");
        FreeFMHMemory();
        return -1;
    }

    SizetoAlloc = NewImageSize;
    ret1 = MemoryAllocation(hSession, SizetoAlloc);
    if (ret1 != 0)
    {
        fprintf(stderr, "Error in Memory Allocation  %x\n", (unsigned int)(SizetoAlloc));
        return -1;
    }

    SizeToFlash = NewImageSize;
    count = (SizeToFlash / MAX_SIZE_TO_READ_MMC);
    for (i = 0; i < count; i++)
    {
        SizeToRead = MAX_SIZE_TO_READ_MMC;

        Buf = bufArr_MMC;
        memset(Buf, 0, SizeToRead);

        if (fseek(fp, seekpos, SEEK_SET) != 0)
            printf("Error in fseek\n");

        bytesread = fread(Buf, 1, SizeToRead, fp);
        if (bytesread != SizeToRead)
        {
            printf("Error in fread bytesread=%x\n", bytesread);
            return -1;
        }

        WriteMemOff = MMCAddofAllocMem;
        Datalength = bytesread;
        
        printf("WritetoMemory  , DoMMCUpdate, line = %d\n", __LINE__);
        if (WritetoMemory(hSession, WriteMemOff, Datalength, Buf) != 0)
        {
            printf("Error in Uploading Firmware Image\n");
            return -1;
        }

        offset1 = Offset;
        NewImageSize1 = NewImageSize;

        printf("Uploading Firmware Image : %d%%\r", (int)((offset1 * 100) / NewImageSize1));
        Offset += SizeToRead;
        seekpos += SizeToRead;
        MMCAddofAllocMem += SizeToRead;
    }

    rembytes = (SizeToFlash % MAX_SIZE_TO_READ_MMC);
    if (rembytes != 0)
    {
        if (fseek(fp, seekpos, SEEK_SET) != 0)
            printf("Error in fseek\n");

        bytesread = fread(Buf, 1, rembytes, fp);
        if (bytesread != rembytes)
        {
            printf("Error in fread i=%x\n", i);
            return -1;
        }

        WriteMemOff = MMCAddofAllocMem;
        Datalength = bytesread;

        printf("WritetoMemory  , DoMMCUpdate, line = %d\n", __LINE__);
        if (WritetoMemory(hSession, WriteMemOff, Datalength, Buf) != 0)
        {
            printf("Error in Uploading Firmware Image\n");
            return -1;
        }
        printf("Upgrading Firmware Image : %d%%\r", (int)((offset1 * 100) / NewImageSize1));
    }
    printf("Uploading Firmware Image: 100%%... done\n ");

    if (EraseAndFlash(hSession, 0, 0, NewImageSize) != 0)
    {
        printf("Error in Erase and Flash...\n");
        return -1;
    }

    if (ECFStatus(hSession) != 0)
    {
        printf("Error while getting Flash Progress Status\n");
        return -1;
    }

    if (VerifyFlash(hSession, 0, 0, NewImageSize) != 0)
    {
        printf("Error in Verifying Flash\n");
        return -1;
    }

    if (VerifyStatus(hSession) != 0)
    {
        printf("Error while getting Verify Progress Status\n");
        return -1;
    }

    if (DeactivateFlshMode(hSession) != 0)
    {
        printf("Error in Deactivating the flash Mode\n");
        return -1;
    }

    IsMMCUpdated = 1;
    ResetFunction(hSession);
    return 0;
}

GetImgBufInfo_T GetImgBufINfo;
ImageHeaderRcrd_T ImageHeaderRcrd;

/*Function to Get the Raw data from file*/
int GetImageBufferInfo(char *imageFilename, GetImgBufInfo_T *GetImgBufINfo)
{
    unsigned int BytesRead = 0;
    pImageFile = fopen(imageFilename, "rb");

    if (pImageFile == NULL)
    {
        printf("Cannot open image file %s", imageFilename);
        return -1;
    }

    /* Get the raw data in file */
    fseek(pImageFile, 0, SEEK_END);
    GetImgBufINfo->imageSize = ftell(pImageFile);
    GetImgBufINfo->pImageData = malloc(sizeof(unsigned char) * GetImgBufINfo->imageSize);
    rewind(pImageFile);

    if (GetImgBufINfo->pImageData != NULL)
    {
        BytesRead = fread(GetImgBufINfo->pImageData, sizeof(unsigned char), GetImgBufINfo->imageSize, pImageFile);
        if (BytesRead != GetImgBufINfo->imageSize)
        {
            printf("GetImageBufferInfo:: Error In Reading Image\n");
            return -1;
        }
    }
    else
    {
        printf("Error Buffer is Empty\n");
        return -1;
    }

    fclose(pImageFile);

    return 0;
}

int ValidateSignature(GetImgBufInfo_T *GetImgBufINfo)
{

    ImageHeaderRcrd_T *ImageHeaderRcrd = (ImageHeaderRcrd_T *)
                                             GetImgBufINfo->pImageData;

    if (strncmp(ImageHeaderRcrd->Signature, FWUPG_IMAGE_SIGNATURE, FWUPG_HEADER_SIGNATURE_LENGTH) != 0)
    {
        /*Valid Image set the global variable*/
        return -1;
    }

    return 0;
}
int GetUpgradeActionRecord(GetImgBufInfo_T *GetImgBufInfo)
{

    unsigned char *pImagePtr;
    unsigned char componentIdByte = 0x0;
    unsigned char componentId = 0x00, componentId1 = 0x00;
    unsigned char *pData;

    UpgAction_Hdr_T *pActionRecord;
    GetImgBufInfo_T *GetImgBufINfo = GetImgBufInfo;
    ImageHeaderRcrd_T *pImageHeader = (ImageHeaderRcrd_T *)GetImgBufINfo->pImageData;
    UpgActRecDes_T *pFwImage;

    /* Put pointer after image header */
    pImagePtr = (unsigned char *)(GetImgBufINfo->pImageData + sizeof(ImageHeaderRcrd_T) + (pImageHeader->OEMDataLength)); //+ sizeof(unsigned char));/*checksum*/

    while (pImagePtr < (GetImgBufINfo->pImageData + GetImgBufINfo->imageSize - FWUPG_MD5_SIGNATURE_LENGTH))
    {

        /* Getting the  action record */
        pActionRecord = (UpgAction_Hdr_T *)pImagePtr;

        /*Getting the Component ID*/
        componentIdByte = pActionRecord->Components; //

        pFwImage = (UpgActRecDes_T *)(pImagePtr + sizeof(UpgAction_Hdr_T));

        componentId = 0;
        while ((componentIdByte >>= 1) != 0)
        {
            componentId++;
        }

        GetImgBufINfo->componentId = componentId;
        componentId1 = componentId;

        pData = ((unsigned char *)pFwImage + sizeof(UpgActRecDes_T));

        /* Getting the firmware length */
        firmwareLength[componentId1] = pFwImage->length[0];
        firmwareLength[componentId1] |= (pFwImage->length[1] << 8) & 0xff00;
        firmwareLength[componentId1] |= (pFwImage->length[2] << 16) & 0xff0000;
        firmwareLength[componentId1] |= (pFwImage->length[3] << 24) & 0xff000000;

        if ((componentId1 != BOOT_COMPONENT) || (componentId1 != APP_COMPONENT) || (componentId1 != MMC_COMPONENT))
        {
            InvalidComponentFound++;
        }

        switch (componentId)
        {

        case BOOT_COMPONENT:
            IsBootComponentPresent = 1;
            break;
        case APP_COMPONENT:
            IsAppComponentPresent = 1;
            break;
        case MMC_COMPONENT:
            IsMMCComponenttPresent = 1;
            break;
        }
        Buffer_HPM[componentId1] = pData;
        /*Pointing to Next Upgrade Action Record*/
        pImagePtr = pData + firmwareLength[componentId1];
    } // end of while

    return 0;
}

int CreateSPIImage()
{
    FILE *fp;
    int BytesWritten = 0;
    INT8U issigned;
    INT16U hash_size = 0;
    fp = fopen("rom_component.ima", "wb");
    if (fp == NULL)
    {
        printf("Failed to Create SPI Image\n");
        return -1;
    }

    memcpy(&issigned, Buffer_HPM[BOOT_COMPONENT] + (2 * sizeof(INT32U)), sizeof(issigned));
    if (issigned)
    {
        memcpy(&hash_size, Buffer_HPM[BOOT_COMPONENT] + (2 * sizeof(INT32U)) + sizeof(INT8U), sizeof(hash_size));
    }
    BytesWritten = fwrite(Buffer_HPM[BOOT_COMPONENT] + SKIP_OEM_DATA, 1, firmwareLength[BOOT_COMPONENT] - SKIP_OEM_DATA - hash_size, fp);
    if (BytesWritten !=
        (firmwareLength[BOOT_COMPONENT] - SKIP_OEM_DATA - hash_size))
    {
        printf("Errro in writing to file\n");
        return -1;
    }

    memcpy(&issigned, Buffer_HPM[APP_COMPONENT] + (2 * sizeof(INT32U)), sizeof(issigned));
    if (issigned)
    {
        memcpy(&hash_size, Buffer_HPM[APP_COMPONENT] + (2 * sizeof(INT32U)) + sizeof(INT8U), sizeof(hash_size));
    }
    BytesWritten = fwrite(Buffer_HPM[APP_COMPONENT] + SKIP_OEM_DATA, 1, firmwareLength[APP_COMPONENT] - SKIP_OEM_DATA - hash_size, fp);
    if (BytesWritten != firmwareLength[APP_COMPONENT] - SKIP_OEM_DATA - hash_size)
    {
        printf("Errro in writing to file\n");
        return -1;
    }
    fclose(fp);
    return 0;
}

int CreateMMCImage()
{
    FILE *fp;
    int BytesWritten = 0;
    INT8U issigned;
    INT16U hash_size = 0;

    fp = fopen("mmc_component.ima", "wb");
    if (fp == NULL)
    {
        printf("Failed to Create SPI Image\n");
        return -1;
    }

    memcpy(&issigned, Buffer_HPM[MMC_COMPONENT] + (2 * sizeof(INT32U)), sizeof(issigned));
    if (issigned)
    {
        memcpy(&hash_size, Buffer_HPM[MMC_COMPONENT] + (2 * sizeof(INT32U)) + sizeof(INT8U), sizeof(hash_size));
    }
    BytesWritten = fwrite(Buffer_HPM[MMC_COMPONENT] + SKIP_OEM_DATA, 1, firmwareLength[MMC_COMPONENT] - SKIP_OEM_DATA - hash_size, fp);
    if (BytesWritten != firmwareLength[MMC_COMPONENT] - SKIP_OEM_DATA - hash_size)
    {
        printf("Errro in writing to file\n");
        return -1;
    }

    fclose(fp);
    return 0;
}

int main(int argc, char *argv[])
{
    int errVal = 0, Updateconfig = 0, check = 0, Ret = 0;
    INT16U i = 0;
    char username[64] = {0}, pass[64] = {0}, Choice[MAX_INPUT_LEN] = {0};
    char *BootVarsName = 0;
    IPMI20_SESSION_T hSession;
#ifndef MSDOS
    char *pPass = NULL, *pUser = NULL;
    int Enable_success = 0;
    INT8U byPrivLevel;
    INT16U wRet = 0, j = 0;
#endif
#if defined(__x86_64__) || defined(WIN64)
    unsigned int ImageSize = 0;
#else
    unsigned long ImageSize = 0;
#endif
    int retval = 0;
    char FlashChoice[MAX_INPUT_LEN] = {0};
    int FwModVer = 0, FwAuxVer = 0, FwVer = 0;
    int DiffBtVer = 0, Parameter = 0, activated = 0;
    char choice[MAX_INPUT_LEN] = {0};
    INT16U ExstModVersion = 0;
    INT8U ExstAuxVersion[6] = {0};
    INT32U ExstFMHLocation = 0, ExstFMHAllocatedSize = 0, ExstFMHModuleSize = 0;
    INT32U ExstModMajorVersion = 0, ExstModMinorVersion = 0;
    char FwModuleName[MAX_NAME_LEN + 1] = {0};
    int NumFMH = 0, ImageDiffer = 0;
    FlashMH **FMHModInfo = NULL;
    int ret1 = 0;
    char Filename[MAX_FILE_NAME_SIZE];
    char MMCImgName[MAX_FILE_NAME_SIZE];
    int Isalpha = 0;
    static int ChoiceNo = 0;
    GetImgBufInfo_T GetImgBufINfo;

    memset((void *)&Parsing, 0, sizeof(UPDATE_INFO));
    memset((void *)&hSession, 0, sizeof(IPMI20_SESSION_T));
    strcpy(username, "\0");
    strcpy(pass, "\0");
    strcpy(device, "127.0.0.1");

    errVal = CommandLineParsing(argc, argv);
    if (errVal != 0)
    {
        return errVal;
    }

    if (byMedium != KCS_MEDIUM) // Create session only for USB or nw media.
    {
#ifndef MSDOS
        byPrivLevel = PRIV_LEVEL_ADMIN;
        if ((byMedium != USB_MEDIUM) && (Parsing.UserNameExist == 0x00) && (Parsing.PasswordExist == 0x00))
        {
            printf("Please enter login information:\n");
            printf("User       : ");
            scanf(" %[^\n]%*c", username);
            GetPasswordInput(pass);
            pUser = username;
            pPass = pass;
        }
        else
        {
            pUser = malloc(MAX_USERNAME_SIZE);
            pPass = malloc(MAX_PASSWORD_SIZE);
            memset(pUser, 0, MAX_USERNAME_SIZE);
            memset(pPass, 0, MAX_PASSWORD_SIZE);
            memcpy(pUser, Parsing.Username, MAX_USERNAME_SIZE);
            memcpy(pPass, Parsing.Password, MAX_PASSWORD_SIZE);
        }

        if (byMedium == NETWORK_MEDIUM)
        {
            byAuthType = AUTHTYPE_RMCP_PLUS_FORMAT;
            printf("\nCreating IPMI session via network with address %s...", device);
        }
#if 0
        else if( byMedium == SERIAL_MEDIUM )
        {
            byAuthType = AUTHTYPE_NONE;
            printf( "\nCreating IPMI session via serial terminal %s...", device );
        }
#endif
        else
        {
            byAuthType = 0;
            pUser = NULL;
            pPass = NULL;
            printf("\nCreating IPMI session via USB...");
        }

        do
        {
            Enable_success = 0;
            wRet = LIBIPMI_Create_IPMI20_Session(&hSession, device, Parsing.Portnum, pUser, pPass,
                                                 byAuthType, 1, 1, &byPrivLevel,
                                                 byMedium, 0xc2, 0x20, NULL, 0, 3000);
            if (wRet != LIBIPMI_E_SUCCESS)
            {
                if (byMedium == USB_MEDIUM)
                {
                    byMedium = KCS_MEDIUM; // changing the medium to KCS for temporay KCS  for enable USB
                    retval = OnEnableUSBDevice();
                    byMedium = USB_MEDIUM; // Reverting  the medium to USB
                    if (retval == 0)
                    {
                        Enable_success = 1;
                        SleepMs(10000);
                        continue;
                    }
                    else
                    {
                        printf("Failed\n");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    printf("Failed\n");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                hSession.g_NetFnOEM = OemNetFn;
                printf("Done\n");
                break;
            }
        } while (Enable_success);

        if (byMedium == NETWORK_MEDIUM)
        {
            if (byPrivLevel < PRIV_LEVEL_ADMIN)
            {
                printf("\nInsufficient privilege level\n");
                printf("So,Your Access denied\n");
                return -1;
            }
        }
#endif
    }
    else
    {
#ifdef WINDOWS

        /* Load Driver */
        //    if( m_win_driver == MS_DRV ){
        //	//printf("MS IPMI Driver\n");
        //    }else{
        FLASH_LoadDriver();
//    }
#endif

#ifdef ICC_OS_LINUX
        CheckAndUnloadIPMIDriver();
#endif

        SetBMCPorts((WORD)0xCA2, (WORD)0xCA3);
        OpenIfc();
    }

    Print_copyrights();
    // Get all features supported by BMC
    if (!RecoveryFlashMode) // Not supports in Recovery flash
        RetrieveFeatureList(&hSession);

    if (0 == GetImageBufferInfo(Parsing.FileName, &GetImgBufINfo))
    {
        /*Checking the Input Image is SPI Image or HPM Format Image*/
        if (0 == ValidateSignature(&GetImgBufINfo))
        {
            IsValidHPMImage = 1;
            Parsing.DoSPIImgUpdate = 0;
        }
        else
        {
            if ((Parsing.UpdateMMCImage) || (Parsing.UpdateSPIImage))
            {
                printf("Error:The option is not valid..!!\n");
                return 0;
            }
            else
            {
                /* Do Nothing , the input Image is SPI Image*/
            }
        }
    }

    if (((IsValidHPMImage) || (Parsing.UpdateMMCImage) || (Parsing.DoMMCImgUpdate)) && (!featuresList.mmc_boot_support))
    {
        printf("Error:MMC Boot Support is not enabled\n");
        return -1;
    }

    if (IsValidHPMImage)
    {
        /* Getting the Upgrade Action Record From the Buffer*/
        if (0 != GetUpgradeActionRecord(&GetImgBufINfo))
        {
            printf("Error in Getting Upgrade Action Record\n");
            return -1;
        }

        if ((IsAppComponentPresent) && (IsBootComponentPresent))
        {
            if (0 != CreateSPIImage())
            {
                printf("Error in creating SPI Image\n");
                return -1;
            }
            ValidCoponentFound = 1;
            memcpy(&Parsing.FileName[0], "rom_component.ima", MAX_FILE_NAME_SIZE);
        }

        if (IsMMCComponenttPresent)
        {
            if (0 != CreateMMCImage())
            {
                printf("Error in creating SPI Image\n");
                return -1;
            }
            ValidCoponentFound = 1;
            memcpy(&Parsing.MMCImgName[0], "mmc_component.ima", MAX_FILE_NAME_SIZE);
        }

        if (InvalidComponentFound)
        {
            if (!ValidCoponentFound)
            {
                printf("Only APP,BOOT and MMC Components are allowed to flash\n");
                printf("Error Invalid Component Found\n");
                return -1;
            }
        }
    }

    if ((IsValidHPMImage) && (Parsing.DoMMCImgUpdate))
    {
        printf("Error: -sd opiton cannot be clubbed with HPM Image\n...Exiting");
    }

    if ((Parsing.UpdateMMCImage) || (Parsing.UpdateSPIImage))
    {
        if (Parsing.UpdateMMCImage)
        {
            if (!IsMMCComponenttPresent)
            {
                printf("Error:The MMC Component is not present in Firmware Image...Exiting!!!!\n");
                return -1;
            }

            if (Parsing.DoMMCImgUpdate)
            {
                printf("Error: -sd opiton cannot be clubbed with -mmc option\n...Exiting\n");
                return -1;
            }
        }
        if (Parsing.UpdateSPIImage)
        {
            if (!(IsAppComponentPresent) || (!IsBootComponentPresent))
            {
                printf("The APP or BOOT Component is not present in Firmware Image...Exiting!!!!\n");
                return -1;
            }

            if (Parsing.DoSPIImgUpdate)
            {
                printf("Error: -spi opiton cannot be clubbed with SPI Image Flashing\n...Exiting\n");
                return -1;
            }
        }
    }

    if ((Parsing.UpdateMMCImage == 1) && (Parsing.UpdateSPIImage == 1))
    {
        Parsing.DoMMCImgUpdate = 1;
    }
    else if ((Parsing.UpdateMMCImage == 1) && (0 == Parsing.UpdateSPIImage))
    {
        Parsing.DoMMCImgUpdate = 1;
        ret1 = DoMMCUpdate(&hSession);
        if (ret1 != 0)
        {
            printf("Error in Updating MMC Image\n");
            return -1;
        }
        return 0;
    }
    else if ((Parsing.UpdateMMCImage == 0) && (1 == Parsing.UpdateSPIImage))
    {
        Parsing.DoMMCImgUpdate = 0;
    }

    if (featuresList.online_flashing_support)
    {
        if (Parsing.ImgInfo == 0x02)
        {
            DisplayFirmwareVersion(&hSession);
            Close_Session(&hSession);
            return 0;
        }
    }

    /* Modules may be flashed already and only need to activate it now */
    if (ActivateOnly == 1)
        goto activate;

    if (Parsing.SignKey == 0x01)
    {
        retval = ReplaceSignedImageKey(&hSession, Parsing.SignKeyFile, RECIEVE_TIME_OUT);

        if (retval == 0)
        {
            printf("Public Key Replaced Successfully\n");
            Ret = 0;
        }
        else if (retval == 0x80)
        {
            printf("Invalid Public Key\n");
            Ret = -1;
        }
        else
        {
            printf(" Replacing the Public Key is not Supported at this time\n");
            Ret = -1;
        }
#ifndef MSDOS
        LIBIPMI_CloseSession(&hSession);
#endif

#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif

        return Ret;
    }

    if (GetStatus(&hSession) != 0)
    {
        FreeFMHMemory();
        return -1;
    }

    Update_Parse_Configurable_Arguments();

    if (featuresList.dual_image_support)
    {
        if ((Parsing.ImgSelect != -1) && (Parsing.ReselectImage))
        {
            printf(STR_INVALID_IMAGE_SELECTOR_OPTION);
            PrintHelp();
            return -1;
        }
    }

    if (Parsing.ImgInfo == 0 && SPIDevice == BMC_FLASH)
    {
        Ret = ConfirmDualImageSupport(&hSession);
        if (Ret != 0)
        {
            /* Dual Image support is not enabled in target platform. Assuming going to flash single image */
            goto switch_device;
        }
        else if ((Ret != 0) && (Parsing.ImgSelect != -1))
        {
            printf("-img-select is valid only when Dual Image Support is enabled\n");
            FreeFMHMemory();
            exit(YAFU_GET_DUAL_IMAGE_FAILED);
        }

        DualImageSup = TRUE;

        // In Recovery Flash it should start flash from Image_1
        if (RecoveryFlashMode)
        {
            goto switch_device;
        }

        if (Parsing.Info == 0x00)
        {
            if (GetDualImageConfig(&hSession) != 0)
            {
                FreeFMHMemory();
                Close_Session(&hSession);
                exit(YAFU_GET_DUAL_IMAGE_FAILED);
            }

            if ((SignedImageSup == 1) && (Parsing.Interactive == 1))
            {
                printf("WARNING !!! Full Firmware will be transferred to BMC to Check Sign in Image\n");
            }

            if (DualImageSettings(&hSession, Parsing.ConfigPreserve) != 0)
            {
                printf("Error in Dual Image Settings \n");
                FreeFMHMemory();
                Close_Session(&hSession);
                exit(YAFU_GET_DUAL_IMAGE_FAILED);
            }
        }
    }

    if (SPIDevice == ME_FLASH)
    {
        errVal = StartDoMEUpdate(&hSession);

        if (NETWORK_MEDIUM == byMedium)
            Close_Session(&hSession);

#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif
        return errVal;
    }

switch_device:
#ifndef MSDOS
    if (SwitchFlashDevice(&hSession, &EraseBlkSize, &FlashSize) < 0)
    {
        printf("Error in identifying the Flash information\n");
#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif
        Close_Session(&hSession);
        return -1;
    }
#endif
    EraseBlkSize = 0;
    FlashSize = 0;
    if (FlashInfo(&hSession, &EraseBlkSize, &FlashSize) != 0)
    {
        printf("Error in Getting FlashInfo \n");
#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif
        Close_Session(&hSession);
        return -1;
    }

    if (Parsing.ImgInfo == 0)
    {
        if (GetImageSize(Parsing.FileName) != 0)
        {
            printf("Error in reading the file\n");
#ifdef ICC_OS_LINUX
            LoadOpenIPMIDrivers();
#endif
            exit(-1);
        }

        if (SaveFwFile() != 0)
        {
            printf("Error while parsing the file\n");
#ifdef ICC_OS_LINUX
            LoadOpenIPMIDrivers();
#endif
            return -1;
        }

        if (EraseBlkSize == 0)
            printf("Error!! The EraseBlkSize = 0.\n");

        if ((NewImageSize % EraseBlkSize) == SIGNED_HASHED_LEGACY_IMG_BYTES)
        {
            ImageSize = NewImageSize - SIGNED_HASHED_LEGACY_IMG_BYTES;
        }
        else if ((NewImageSize % EraseBlkSize) == SIGNED_HASHED_IMG_BYTES)
        {
            ImageSize = NewImageSize - SIGNED_HASHED_IMG_BYTES;
        }
        else
        {
            ImageSize = NewImageSize;
        }

        if ((SPIDevice == BMC_FLASH) && (VerifyImageChecksum(ImageSize, Buffer, ModuleCount) != 0))
        {
            printf("The %s File is Corrupted\n", Parsing.FileName);
            Close_Session(&hSession);
            exit(YAFU_IMAGE_CHKSUM_VERIFY_FAILED);
        }
    }

    retval = FirmwareInfo(&hSession, Parsing.FileName);
    if (retval != 0)
    {
        if (retval == -1)
            printf("Error in Getting Firmware Information \n");
#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif
        return -1;
    }

    if (SPIDevice != BMC_FLASH)
    {
        // Parsing.BiosUpdate = 1;//tmp set this flag.
        Parsing.Full = 0;           // Parsing.Full is used for BMC Firmware Update
        Parsing.RebootFirmware = 0; // We dont want to reboot BMC after performing CPLD update

        if (ActivateFlashMode(&hSession) != 0)
        {
            printf("Error in activate flash mode\n");
            Close_Session(&hSession);
            FreeFMHMemory();
            return -1;
        }

        if (CPLD_FLASH == SPIDevice)
            printf("Beginning CPLD Update...\n");
        else
            printf("Beginning BIOS Update...\n");

        if ((retval = FlashFullFirmwareImage(&hSession, fp, Parsing.ConfigPreserve, Parsing.BootPreserve)) != 0)
        {
            if (CPLD_FLASH == SPIDevice)
                printf("\nError in updating CPLD image\n");
            else
                printf("\nError in updating BIOS image\n");
        }

        if (DeactivateFlshMode(&hSession) != 0)
        {
            printf("\nError in Deactivate Flash mode\n");
            Close_Session(&hSession);
            FreeFMHMemory();
            return -1;
        }

        Close_Session(&hSession);
        FreeFMHMemory(); // Freeing up used memory
        return 0;
    }

/* If individual conf feature is enabled and active image is going to flash then override the flag
to reboot since it goes to init7
   If only uboot is flashing then no need to check the below condition
*/
#ifdef CONFIG_SPX_FEATURE_INDIVIDUAL_CONF_SECTION
    if ((Parsing.RebootFirmware == 0x00) && (BootOnly == 0x00) && (Parsing.ImgSelect == 1 || DualImageReq == FwUploadImg))
    {
        printf("Going to flash active image... \n");
        while (1)
        {
            if (!non_interactive)
            {
                printf("Firmware will reboot after flashing... If you want to proceed type (Y/y) or (N/n) to exit \n");
                scanf(" %[^\n]%*c", Choice);
                if ((spstrcasecmp(Choice, "y") == 0) || (spstrcasecmp(Choice, "n") == 0))
                {
                    break;
                }
            }
            else
            {
                strcpy(Choice, "y");
                printf("Firmware will reboot after flashing... \n");
                break;
            }
        }

        if (spstrcasecmp(Choice, "y") == 0)
            Parsing.RebootFirmware = 0x01;
        else
        {
            exit(0);
        }
    }
#endif

    if (Parsing.ConfigPreserveBitMask != 0x00 || extflag == 1)
    {
        if (CheckPreserveConfStatus(&hSession) != 0)
        {
            printf(" Error in Getting Preserve Configuration\n");
            FreeFMHMemory();
            Close_Session(&hSession);
            return -1;
        }
        PreserveFlag &= 0xFD; // Preserve Config BIT1 has to be Masked
    }

    for (i = 0; i < ModuleCount; i++)
    {
        if ((spstrcasecmp(FwModuleHdr[i]->ModuleName, "conf") == 0) || (Parsing.SplitImg == 0x01))
        {
            CommonConf = FALSE;
            break;
        }
    }

    // No concept of Preserve Configurations in Recovery Flash
    if (!RecoveryFlashMode && Parsing.Info == 0x00 && DualImageSup == TRUE && SPIDevice == BMC_FLASH && Parsing.ImgInfo == 0x00)
    {
        if (Parsing.ConfigPreserve != 1 && CommonConf == TRUE)
        {
            printf("WARNING !!! Images of both flash share the same Configuration area. Not preserving will restore to default factory settings.\n");
            while (1)
            {
                if (non_interactive || Parsing.NotPreserveCfg)
                    break;
                printf("Please type (Y/y) to proceed or (N/n) to exit : ");
                scanf(" %[^\n]%*c", choice);
                if (spstrcasecmp(choice, "y") == 0)
                {
                    break;
                }
                else if (spstrcasecmp(choice, "n") == 0)
                {
                    FreeFMHMemory();
                    Close_Session(&hSession);
                    exit(0);
                }
            }
        }
    }

    if (FwUploadImg == IMAGE_2 && FlashBothImg == TRUE)
    {
        // Do the changes to switch the structure to proceed further
        ToggleDualFMHInfo();
    }

    if (RecoveryFlashMode)
    {
        if (NewImageSize % EraseBlkSize == SIGNED_HASHED_LEGACY_IMG_BYTES)
        {
            NewImageSize -= SIGNED_HASHED_LEGACY_IMG_BYTES;
        }
        else if (NewImageSize % EraseBlkSize == SIGNED_HASHED_IMG_BYTES)
        {
            NewImageSize -= SIGNED_HASHED_IMG_BYTES;
        }
    }

    // flashing both firmware images if BMC in uboot mode(Recovery flash)
    if (RecoveryFlashMode && (Parsing.Info == 0) && (Parsing.ImgInfo == 0) && (Parsing.ImgSelect == -1) && (DualImageSup == TRUE))
    {
        Parsing.Full = 0x01;
        Parsing.BootPreserve = 0x00;
        FlashBothImg = TRUE;
        ImgCount = IMAGE_2;

        goto dualfwflash;
    }

    if ((Parsing.SkipFMHCheck == 0x01) || (Parsing.SkipCRCValidation == 0x01))
    {
        Parsing.Full = 0x01;
        Parsing.Info = 0x01;
        if (FwUploadImg == IMAGE_BOTH)
        {
            ImgCount = IMAGE_2;
            FlashBothImg = TRUE;
        }
        else
        {
            ImgCount = IMAGE_1;
        }
        goto dualfwflash;
    }

    if (vdbg)
    {
        printf("Validating the Checksum for the Image\n");
    }

    if ((Parsing.Info == 0x01) || (Parsing.ImgInfo != 0))
    {
        if (FlashFMH != 0)
        {
            if (DisplayFirmwareInfo() != 0)
            {
                FreeFMHMemory();
                Close_Session(&hSession);
                return -1;
            }
            if (vdbg)
            {
                DisplayAltFirmwareInfo();
            }
        }
        else
        {
            DisplayExistingFirmwareInfo(0);
            if (vdbg)
            {
                DisplayExistingFirmwareInfo(1);
            }
        }
        Close_Session(&hSession);
        FreeFMHMemory();
        return 0;
    }

    // update both images if uboot versions mismatch in dual image case.
    if (DualImageSup && (!featuresList.do_not_upgrade_both_firmware_on_uboot_mismatch))
    {
        if (Check_DualImagebootVersions(&hSession) == 1)
        {
            printf("U-boot versions mis-match hence Flashing Both Images...\n");
            FlashBothImg = TRUE;
            ImgCount = FLASH_BOTH_IMAGES;
            Parsing.BootPreserve = 0x00;
            if (SetDualImageConfig(&hSession, SETFWUPLOADSELECTOR, IMAGE_BOTH) != 0)
            {
                printf("Error in setting Dual Image Configuration \n");
                Close_Session(&hSession);
                FreeFMHMemory();
                return -1;
            }
        }
    }

dualfwflash:

    for (NoOfImg = 1; NoOfImg <= ImgCount; NoOfImg++)
    {
        if (NoOfImg == IMAGE_1)
        {
            NumFMH = FlashFMH;
            FMHModInfo = CurrFwModHdr;
            ImageDiffer = ImageDiffer1;
        }
        else if (NoOfImg == IMAGE_2)
        {
            //            Parameter = SETFWUPLOADSELECTOR;
            //            SetDualImageConfig(&hSession,Parameter, IMAGE_2);
            NumFMH = DualFMH;
            FMHModInfo = DualFwModHdr;
            ImageDiffer = ImageDiffer2;
            EFStart = 0;
            Progressdis = 0;
            ECFPercent = 0;
            VerifyPercent = 0;
        }

        if (RecoveryFlashMode)
        {
            goto FullFW;
        }

        if ((Parsing.SkipFMHCheck != 0x01) && (Parsing.SkipCRCValidation != 0x01) && (BMC_FLASH == SPIDevice))
        {
            if (Platformchecking(FMHModInfo, NumFMH) != 0)
            {
                FlashSelected = 0;
                Close_Session(&hSession);
                FreeFMHMemory();
                return -1;
            }

            if (Parsing.Interactive == 0x01)
            {
                Mode = 1;
                if ((ModuleCount != NumFMH) || (ImageDiffer == 0x01))
                {
                    goto FullFW;
                }
                else if (InteractiveUpgrade(&hSession, Parsing.FileName, FMHModInfo, NumFMH, ImageDiffer) != 0)
                {
                    if (((featuresList.mmc_boot_support) && (Parsing.DoMMCImgUpdate == 1) && (IsMMCUpdated == 0)))
                    {
                        ret1 = DoMMCUpdate(&hSession);
                        if (ret1 != 0)
                        {
                            printf("Error in Updating MMC Image\n");
                            return -1;
                        }
                        return 0;
                    }
                    else
                    {
                        ResetFunction(&hSession);
                    }
                }
                if ((DualImageSup == TRUE) && (FlashBothImg == TRUE) && (NoOfImg < ImgCount))
                {
                    continue;
                }
                FreeFMHMemory();
                return 0;
            }

            if ((NewImageSize != FlashSize) && (Parsing.SplitImg == 0) && (DualImageSup == FALSE))
            {
                if (BMC_FLASH == SPIDevice) // BMC
                {
                    printf("The Rom Image size = %d MB\n", (int)(NewImageSize / 1048576));
                    printf("The Current flash size = %d MB\n", (int)(FlashSize / 1048576));
                    if ((Parsing.ConfigPreserve == 0x01) && (ConfigRomAllocAddr != ConfigFirmAllocAddr) || (ConfigFirmAddr != ConfigRomAddr))
                    {
                        printf("Preserving the configurations is not possible as conf location/size differs\n");
                        while (1)
                        {
                            if (!non_interactive)
                            {
                                printf("So type Y/y to proceed with the upgrade process with out preserving configuration\n");
                                printf("N/n to exit\n");
                                scanf(" %[^\n]%*c", choice);
                            }
                            else
                            {
                                strcpy(FlashChoice, "y");
                                printf("Continuing with upgrade process without preserving configuration\n");
                            }
                            if (spstrcasecmp(choice, "y") == 0)
                            {
                                Parsing.ConfigPreserve == 0x00;
                                break;
                            }
                            else if (spstrcasecmp(choice, "n") == 0)
                            {
                                return -1;
                            }
                            else
                            {
                                printf("Please enter the proper option\n");
                            }
                        }
                    }
                }
            }

            if (Parsing.FlashModImg == 0x01)
            {
                if (Parsing.FlashModCnt != 0x00)
                {
                    Ret = NonInteractiveUpgrade(&hSession, Parsing.FlashModCnt);
                    if (Ret == -1)
                    {
                        if (((featuresList.mmc_boot_support) && (Parsing.DoMMCImgUpdate == 1) && (IsMMCUpdated == 0)))
                        {
                            ret1 = DoMMCUpdate(&hSession);
                            if (ret1 != 0)
                            {
                                printf("Error in Updating MMC Image\n");
                                return -1;
                            }
                            return 0;
                        }
                        else
                        {
                            ResetFunction(&hSession);
                        }
                    }
                    if ((DualImageSup == TRUE) && (FlashBothImg == TRUE) && (NoOfImg < ImgCount))
                    {
                        continue;
                    }
                    FreeFMHMemory();
                    return 0;
                }
                else
                {
                    printf("Module name is not given with -flash- \n");
                    printf("So,Type (Y/y) to do Full Firmware Upgrade or (N/n) to exit\n");
                    printf("Enter your Option : ");
                    scanf(" %[^\n]%*c", FlashChoice);
                    if (spstrcasecmp(FlashChoice, "y") == 0)
                    {
                        goto fwflash;
                    }
                    else if (spstrcasecmp(FlashChoice, "n") == 0)
                    {
                        FlashSelected = 0;
                        FreeFMHMemory();
                        exit(0);
                    }
                }
            }

            if ((Parsing.SplitImg == 0x01 && SplitFlashOffset != 0) || BootOnly == 0x01)
            {
                /* In case of split image module count will get differ so skipping it */
                goto fwflash;
            }

        FullFW:
            if ((ModuleCount != NumFMH) || (ImageDiffer == 0x01))
            {
                if ((DualImageSup == TRUE) && (FlashBothImg == TRUE))
                {
                    printf("The Image Modules count are different for Image-%d \nSo,", NoOfImg);
                }
                else
                {
                    printf("\rThe Image Modules count are different for Image \nSo,");
                }
                while (1)
                {
                    if (Parsing.Info == 0x01)
                    {
                        printf("The Images are different. \n ");
                    }
                    else
                    {
                        printf("The Images are different. Use '-info' Option to know more \n");
                    }

                    if ((!non_interactive) && (Parsing.DiffImage == 0) && (!RecoveryFlashMode) && (Parsing.PlatformCheck == 0))
                    {
                        printf("So,Type (Y/y) to do Full Firmware Upgrade or (N/n) to exit\n");
                        printf("Enter your Option : ");
                        scanf(" %[^\n]%*c", FlashChoice);
                    }
                    else
                    {
                        strcpy(FlashChoice, "y");
                        printf(" Continuing with Full Firmware Update \n");
                    }
                    if (spstrcasecmp(FlashChoice, "y") == 0)
                    {
                        Parsing.Full = 0x01;
                        SkipFreeArea = 1;
                        goto fwflash;
                        break;
                    }
                    else if (spstrcasecmp(FlashChoice, "n") == 0)
                    {
                        FlashSelected = 0;
                        FreeFMHMemory();
                        exit(0);
                    }
                }
            }

            for (i = 0; i < ModuleCount; i++)
            {
                ExstModVersion = FMHModInfo[i]->Module_Version.ModVersion;
                memcpy(ExstAuxVersion, FMHModInfo[i]->ModuleAuxVer, 6);
                ExstFMHLocation = FMHModInfo[i]->FmhLocation;
                ExstFMHAllocatedSize = FMHModInfo[i]->AllocatedSize;
                ExstFMHModuleSize = FMHModInfo[i]->ModuleSize;

#ifdef CONFIG_SPX_FEATURE_YAFUFLASH_FLASH_UBOOT_ON_ACTIVE_IMAGE
                strncpy(FwModuleName, (char *)FMHModInfo[i]->ModuleName, MAX_NAME_LEN);
                if ((spstrcasecmp(FwModuleName, "boot") == 0) ||
                    (spstrcasecmp(FwModuleName, "boot_img") == 0))
                {
                    ExstModVersion = PrimFwModHdr[i]->Module_Version.ModVersion;
                    memcpy(ExstAuxVersion, PrimFwModHdr[i]->ModuleAuxVer, 6);
                    ExstFMHLocation = PrimFwModHdr[i]->FmhLocation;
                    ExstFMHAllocatedSize = PrimFwModHdr[i]->AllocatedSize;
                    ExstFMHModuleSize = PrimFwModHdr[i]->ModuleSize;
                }
#endif

                FwModVer = 0;
                FwAuxVer = 0;
                FwVer = 0;
                check = 1;
                if ((FwModuleHdr[i]->Module_Version.ModVersion == ExstModVersion))
                {
                    FwModVer = 1;
                }

                if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
                {
                    if (strncmp(FwModuleHdr[i]->ModuleAuxVer, ExstAuxVersion, 2) == 0)
                    {
                        FwAuxVer = 1;
                    }
                }
                else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
                {
                    if (strncmp(FwModuleHdr[i]->ModuleAuxVer, ExstAuxVersion, 6) == 0)
                    {
                        FwAuxVer = 1;
                    }
                }

                if (FwAuxVer == 1 && FwModVer == 1)
                {
                    FwVer = 1;
                }
                else
                {
                    FwVer = FwModVer;
                }

                if (FwModuleHdr[i]->FmhLocation != ExstFMHLocation)
                    printf("\rThe Module %s location is different from the one in the Image", FwModuleHdr[i]->ModuleName);
                else if (FwModuleHdr[i]->AllocatedSize != ExstFMHAllocatedSize)
                    printf("\rThe Module %s allocated size is different from the one in the Image", FwModuleHdr[i]->ModuleName);
                else if (FwModuleHdr[i]->ModuleSize != ExstFMHModuleSize)
                    printf("\rThe Module %s size is different from the one in the Image", FwModuleHdr[i]->ModuleName);
                else if (FwModVer == 0)
                    printf("\rThe Module %s major or minor version is different from the one in the image", FwModuleHdr[i]->ModuleName);
                else if (FwAuxVer == 0)
                    printf("\rThe Module %s aux-version is different from the one in the image", FwModuleHdr[i]->ModuleName);
                else
                    continue;

                if ((DualImageSup == TRUE) && (FlashBothImg == TRUE))
                {
                    printf("-%d\n ", NoOfImg);
                }
                else
                {
                    printf("\n");
                }

                if (IsValidHPMImage)
                {
                    goto fwflash;
                }

                while (1)
                {
                    check = 2;
                    if ((!non_interactive) && (Parsing.ModLocation == 0) && (!RecoveryFlashMode) && (Parsing.DiffImage == 0))
                    {
                        printf("So,Type (Y/y) to do Full Firmware Upgrade or (N/n) to exit\n");
                        printf("Enter your Option : ");
                        scanf(" %[^\n]%*c", FlashChoice);
                    }
                    else
                    {
                        strcpy(FlashChoice, "y");
                        SkipFreeArea = 1;
                        printf(" Continuing with Full Firmware Update \n");
                    }
                    if (spstrcasecmp(FlashChoice, "y") == 0)
                    {
                        Parsing.Full = 0x01;
                        SkipFreeArea = 1;
                        if (strcmp(FwModuleHdr[i]->ModuleName, "boot") == 0)
                            Parsing.BootPreserve = 0x00;
                        goto fwflash;
                    }
                    else if (spstrcasecmp(FlashChoice, "n") == 0)
                    {
                        FlashSelected = 0;
                        FreeFMHMemory();
                        exit(0);
                    }
                }
            }

            if (check == 1)
            {
                if (NoOfImg == IMAGE_1)
                {
                    DisplayFirmwareInfo();
                }
                if ((DualImageSup == TRUE) && (FlashBothImg == TRUE))
                {
                    printf("Existing Image-%d and Current Image are Same\n", NoOfImg);
                }
                else
                {
                    printf("Existing Image and Current Image are Same\n");
                }
                if (IsValidHPMImage)
                {
                    goto fwflash;
                }
                while (1)
                {
                    if ((!non_interactive) && (Parsing.SameImage == 0) && (!RecoveryFlashMode))
                    {
                        if (Blockselect)
                            printf("So,Type (Y/y) to do Full Firmware Upgrade By Block or (N/n) to exit\n");
                        else
                            printf("So,Type (Y/y) to do Full Firmware Upgrade or (N/n) to exit\n");
                        if (1 == Parsing.versioncmpflash)
                        {
                            Parsing.versioncmpflash = 2;
                        }
                        printf("Enter your Option : ");
                        scanf(" %[^\n]%*c", FlashChoice);
                    }
                    else
                    {
                        strcpy(FlashChoice, "y");
                        printf(" Continuing with Full Firmware Update \n");
                    }
                    if (spstrcasecmp(FlashChoice, "y") == 0)
                    {
                        Parsing.Full = 0x01;
                        goto fwflash;
                    }
                    else if (spstrcasecmp(FlashChoice, "n") == 0)
                    {
                        if (activated)
                        {
                            ResetFunction(&hSession);
                            Close_Session(&hSession);
                        }
                        FlashSelected = 0;
                        FreeFMHMemory();
                        exit(0);
                    }
                }
            }
        }
    fwflash:

        if ((Parsing.UpdateMMCImage == 0) && (Parsing.UpdateSPIImage == 0) && (IsValidHPMImage))
        {
            while (1)
            {
                printf("Please provide the components to Upgrade\n");
                printf("0-Update Both MMC and SPI\n\
				 	  1-Only SPI Image\n\
				 	  2- Only MMC Image\n");
                printf("Enter your Option : ");
                scanf(" %[^\n]%*c", FlashChoice);
                Isalpha = 0;
                for (i = 0; FlashChoice[i] != '\0'; i++)
                {
                    if (isalpha(FlashChoice[i]))
                    {
                        Isalpha = 1;
                        break;
                    }
                }
                if (Isalpha == 1)
                {
                    printf("%s is invalid\n", FlashChoice);
                }
                else
                {
                    ChoiceNo = atoi(FlashChoice);
                    if ((ChoiceNo >= 0) && (ChoiceNo <= 2))
                    {
                        if (ChoiceNo == 0)
                        {
                            Parsing.DoMMCImgUpdate = 1;
                        }
                        else if (ChoiceNo == 1)
                        {
                            Parsing.DoMMCImgUpdate = 0;
                        }
                        else
                        {
                            ret1 = DoMMCUpdate(&hSession);
                            if (ret1 != 0)
                            {
                                printf("Failed to Update MMC Image\n");
                                return -1;
                            }
                            return 0;
                        }

                        break;
                    }
                    else
                    {
                        printf("Enter valid no: of  components to update \n");
                        continue;
                    }
                }
            }
        } /* end of If*/

        if (Parsing.Full == 0x01)
        {
            if (!activated)
            {
                if (RecoveryFlashMode == 0x00)
                {
                    EraseAndFlash(&hSession, 0, 0, 1);

                    if ((Parsing.SkipCRCValidation == 0x01) || (Parsing.SkipFMHCheck == 0x01))
                    {
                        goto skip_crc;
                    }

                    if (ECFStatus(&hSession) == -1)
                    {
                        oldVersion = 0x01;
                        if (vdbg)
                        {
                            printf("Older Version !Doing Firmware upgrade block by block\n");
                        }
                    }
                    else
                    {
                        oldVersion = 0x00;
                        if (vdbg)
                        {
                            printf("Newer Version ! Doing Full Firmware upgrade \n");
                        }
                    }
                }
                else
                {
                    Parsing.Full = 0x02;
                    oldVersion = 0x01;
                    if (vdbg)
                        printf("Firmware is in UBOOT Mode !Doing Firmware upgrade block by block \n");
                }
            }
            for (i = 0; i < ModuleCount; i++)
            {
                if ((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "conf")) == 0 ||
                    (spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "params")) == 0)
                {
                    Updateconfig = 0x01;
                }
            }
            if (Updateconfig != 0x01)
            {
                Parsing.ConfigPreserve = 0x00;
            }

        skip_crc:
            if (!activated)
            {
                if (ActivateFlashMode(&hSession) != 0)
                {
                    printf("\nError in activate flash mode");
                    FlashSelected = 0;
                    FreeFMHMemory();
                    return -1;
                }
                activated = 1;
            }

            if (SPIDevice == BMC_FLASH)
            {
                if (ReadRngFirmwareRelBase(&hSession, &CurReleaseID[0], &CurCodeBaseVersionID[0]) == 0)
                {
                    ImgOpt = CheckReleaseIDCodeVersion();
                }

                if ((ImgOpt == -1) && (Parsing.SplitImg == 1))
                {
                    printf("Split Image Flashing is not allowed due to Code Base Version Mismatch\n");
                    FreeFMHMemory();
                    Close_Session(&hSession);
                    return -1;
                }
                else if ((ImgOpt == -1) && (Parsing.ExtlogPreserve == 1))
                {
                    printf("Extended Log Preserving is not allowed due to Code Base Version Mismatch\n");
                    FreeFMHMemory();
                    Close_Session(&hSession);
                    return -1;
                }
                else if ((ImgOpt == -1) && (Parsing.ConfigPreserveBitMask != 0))
                {
                    printf("Preserving Individual Configurations are not allowed due to Code Base Version Mismatch\n");
                    FreeFMHMemory();
                    Close_Session(&hSession);
                    return -1;
                }
            }

            if (Parsing.BootPreserve != 0x00)
            {
                for (i = 0; i < ModuleCount; i++)
                {
                    if ((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName, "boot")) == 0)
                    {
                        ExstModMajorVersion = FMHModInfo[i]->Module_Version.ModMajorVer;
                        memcpy(ExstAuxVersion, FMHModInfo[i]->ModuleAuxVer, 6);
                        ExstModMinorVersion = *(&(FMHModInfo[i]->Module_Version.ModMinorVer) + 1);

#ifdef CONFIG_SPX_FEATURE_YAFUFLASH_FLASH_UBOOT_ON_ACTIVE_IMAGE
                        ExstModMajorVersion = PrimFwModHdr[i]->Module_Version.ModMajorVer;
                        memcpy(ExstAuxVersion, PrimFwModHdr[i]->ModuleAuxVer, 6);
                        ExstModMinorVersion = *(&(PrimFwModHdr[i]->Module_Version.ModMinorVer) + 1);
#endif

                        DiffBtVer = 0;
                        if ((*(&(FwModuleHdr[i]->Module_Version.ModMinorVer) + 1) != ExstModMinorVersion) || (FwModuleHdr[i]->Module_Version.ModMajorVer != ExstModMajorVersion))
                        {
                            DiffBtVer = 1;
                        }
                        if (FMH_MAJOR >= 1 && FMH_MINOR == 6)
                        {
                            if (strncmp(FwModuleHdr[i]->ModuleAuxVer, ExstAuxVersion, 2) != 0)
                            {
                                DiffBtVer = 1;
                            }
                        }
                        else if (FMH_MAJOR >= 1 && FMH_MINOR >= 7)
                        {
                            if (strncmp(FwModuleHdr[i]->ModuleAuxVer, ExstAuxVersion, 6) != 0)
                            {
                                DiffBtVer = 1;
                            }
                        }

                        if (DiffBtVer == 1)
                        {
                            while (1)
                            {
                                if ((!non_interactive) && (Parsing.BootVersion == 0))
                                {
                                    printf("UBOOT Versions is different Updating of UBOOT is recommended  \n");
                                    printf("So,Type (Y/y) to Update UBOOT\n");
                                    printf("or (N/n) to Skip\n");
                                    printf("Enter your Option : ");
                                    scanf(" %[^\n]%*c", FlashChoice);
                                }
                                else
                                {
                                    strcpy(FlashChoice, "y");
                                    printf(" Continuing Full Firmware Upgrade with update of UBOOT \n");
                                }
                                if (spstrcasecmp(FlashChoice, "n") == 0)
                                {
                                    break;
                                }
                                else if (spstrcasecmp(FlashChoice, "y") == 0)
                                {
                                    Parsing.BootPreserve = 0x00;
                                    SkipFreeArea = 1;
                                    break;
                                }
                                else
                                {
                                    printf("Please enter a valid option \n");
                                }
                            }
                        }

                        if (RecoveryFlashMode == 0x00)
                        {
                            if (ModuleUpgrade(&hSession, fp, i, 0, 0xFF, 0xFF) != 0)
                            {
                                while (1)
                                {
                                    if ((!non_interactive) && (Parsing.BootVersion == 0))
                                    {
                                        printf("UBoot image is different Updating of UBOOT is recommended  \n");
                                        printf("So,Type (Y/y) to Update UBOOT\n");
                                        printf("or (N/n) to Skip\n");
                                        printf("Enter your Option : ");
                                        scanf(" %[^\n]%*c", FlashChoice);
                                    }
                                    else
                                    {
                                        strcpy(FlashChoice, "y");
                                        printf(" Continuing Full Firmware Upgrade with update of UBOOT \n");
                                    }
                                    if (spstrcasecmp(FlashChoice, "n") == 0)
                                    {
                                        break;
                                    }
                                    else if (spstrcasecmp(FlashChoice, "y") == 0)
                                    {
                                        Parsing.BootPreserve = 0x00;
                                        SkipFreeArea = 1;
                                        break;
                                    }
                                    else
                                    {
                                        printf("Please enter a valid option \n");
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            } // End of if(Parsing.BootPreserve != 0x00)

            if (NoOfImg == IMAGE_1)
            {
                WarningMessage();
            }
            if (Parsing.BootPreserve == 0x00)
            {
                BootVarsName = bvnArr;
                memset(BootVarsName, 0, sizeof(bvnArr));
                if (PreserveUbootEnvVariables(&hSession, BootVarsName, &BootVarsCount) != 0)
                {
                    printf("Error in PreservingEnvVariables \n");
                    FlashSelected = 0;
                    FreeFMHMemory();
                    return -1;
                }

                if (ImgOpt == -1)
                {
                    RegenerateUBootVars(&env, BootVarsName, BootVarsCount, EraseBlkSize);
                }
            }
            if ((Blockselect == 1) && (SignedImageSup == 1))
            {
                printf("Block by Block Mode is not Supported for signed image\n");
                FreeFMHMemory();
                return -1;
            }
            else if (Blockselect == 1)
                Parsing.Full = 0x02;

            if ((retval = FlashFullFirmwareImage(&hSession, fp, Parsing.ConfigPreserve, Parsing.BootPreserve)) != 0)
            {
                if ((retval == 1) && (Parsing.Full == 1))
                {
                    if (non_interactive)
                    {
                        printf("Firmware Upgrade is Upgrading By Blocks  \n");
                    }
                    strcpy(FlashChoice, "y");
                    if (spstrcasecmp(FlashChoice, "y") == 0)
                    {
                        Parsing.Full = 0x02;
                    }
                    if ((Parsing.Full == 0x02) && (SignedImageSup == 1))
                    {
                        printf("Block by Block Mode is not Supported for signed image\n");
                        FreeFMHMemory();
                        return -1;
                    }
                    if (FlashFullFirmwareImage(&hSession, fp, Parsing.ConfigPreserve, Parsing.BootPreserve) == 0)
                        goto out;
                }
                printf("Error in Flashing Full Firmware Image \n");
                goto out;
            }
        }
    out:
        if (featuresList.online_flashing_support)
        {
            /*if online flashing support is enabled and only boot alone is flashed then shouldnt reboot BMC.
            This case will occur in split image */
            if ((PreserveFlag & BIT0) == 0)
                Parsing.RebootFirmware = 0x00;
            /* if online flashing support is enabled and conf is flashed then reboot BMC */
            else if ((PreserveFlag & BIT1) == 0 || (PreserveFlag & BIT2) == 0)
            {
                Parsing.RebootFirmware = 0x01;
            }
        }

        if (oldVersion != 0x01 || Parsing.Full != 0x02)
        {
            if ((Parsing.BootPreserve == 0x00) && (EnvWritten == 0))
            {
                if (ImgOpt != -1)
                {
                    BootVarsName = bvnArr;
                    if (SettingEnvUbootVariables(&hSession, BootVarsName, &BootVarsCount) != 0)
                    {
                        printf("Error in SettingEnvUbootVariables \n");
                        FlashSelected = 0;
                        FreeFMHMemory();
                        return -1;
                    }
                }
                else
                {
                    SetBlkUBootVars(&hSession, &env, EraseBlkSize);
                }
            }
        }
    } // end of for loop to flash number of images
    if (activated)
    {
        if (DeactivateFlshMode(&hSession) != 0)
        {
            printf("\nError in Deactivate Flash mode\n");
            RestoreFlashDevice(&hSession);
            FreeFMHMemory();
            return -1;
        }
        if (RestoreFlashDevice(&hSession) < 0)
        {
            printf("Error in Closing the mapped flash info\n");
            FlashSelected = 0;
            FreeFMHMemory();
            return -1;
        }

        if (Parsing.RebootFirmware == 0x01)
        {
            if (((featuresList.mmc_boot_support) && (Parsing.DoMMCImgUpdate == 1) && (IsMMCUpdated == 0)))
            {
                ret1 = DoMMCUpdate(&hSession);
                if (ret1 != 0)
                {
                    printf("Error in Updating MMC Image\n");
                    return -1;
                }
                return 0;
            }
            else
            {
                ResetFunction(&hSession);
            }
        }

        if (retval == -1)
        {
            FlashSelected = 0;
            FreeFMHMemory();
            Close_Session(&hSession);
            return -1;
        }
    }

activate:

    if (Parsing.ActivateNode != 0x00)
    {
        printf("Starting to activate the devices .... \n");
        fflush(stdout);
        retval = ActivateFlashDevice(&hSession, Parsing.ActivateNode);
        if (retval != 0)
        {
            // return value of -1 would indicate Configure/Activationn of BMC or BIOS module failed
            // return value of -2 would indicate Configure/Activation of CPLD failed
            if (retval == -1)
            {
                printf("\nERROR: Unable to activate all the flashed modules.\n");
                Close_Session(&hSession);
                return -1;
            }
            else if (retval == -2)
            {
                printf("\nERROR: Configuring the CPLD Failed... Trying to ReFlash/ReConfigure the CPLD.. Please Wait...\n");

                // Setting the flag for a full upgrade
                Parsing.Full = 0x01;
                SPIDevice = CPLD_FLASH;

                // Initiating the flash sequence again.
                // The Memory pointer will be NULL. BMC will identify if CPLD failed and will use backup Image now
                // If Flash fails again, we give up....
                retval = FlashAndVerify(&hSession, 0, 0, 0, 0xff);
                if (retval != 0)
                {
                    printf("\nERROR: ReFlash of the CPLD Failed.... !! CPLD May be Un-Stable...\n");
                    Close_Session(&hSession);
                    return -1;
                }

                // Activate/Configure the CPLD again, if re-flash succeeds
                // If Reconfigure Fails, we give up....
                retval = ActivateFlashDevice(&hSession, Parsing.ActivateNode);
                if (retval != 0)
                {
                    printf("\nERROR: ReConfigure of the CPLD Failed.... !! CPLD May be Un-Stable...\n");
                    Close_Session(&hSession);
                    return -1;
                }
            }
            else if (retval == -3)
            {
                printf("\nERROR: Flashing/Activation already in Progress....\n");
                Close_Session(&hSession);
                return 0;
            }
            else if (retval == -4)
            {
                printf("Activating the given device is not supported....\n");
                Close_Session(&hSession);
                return 0;
            }
        } // end of ActivateFlashDevice()

        printf("Activating the modules Done ....\n");
        printf("***********************************************************\n");
        printf("**** IF BMC IS FLASHED/ACTIVATED, BMC WILL BE REBOOTED ****\n");
        printf("**** USER MAY LOSE ACCESS TO BMC FOR AROUND 2 MINUTES  ****\n");
        printf("***********************************************************\n");
        Close_Session(&hSession);
        return 0;
    } // end of if(Parsing.ActivateNode)

    FreeFMHMemory(); // Freeing up used memory
    Close_Session(&hSession);
    return 0;
}

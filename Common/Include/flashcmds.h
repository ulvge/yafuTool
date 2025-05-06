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
* Filename: flashcmds.c
*
* Author   : Winston <winstonv@amiindia.co.in>
*
******************************************************************/


#ifndef __FLASH_CMDS_H__
#define __FLASH_CMDS_H_

#ifndef MSDOS
#include "libipmi_session.h"
#include "libipmi_struct.h"
#include "libipmi_errorcodes.h"
#include "IPMI_AMIDevice.h"
#include "libipmi_AMIOEM.h"
#include "AMIRestoreDefaults.h"
#endif
#include "IPMIDefs.h"
#include "IPMI_AMIDevice.h"

#define OEM_DELL_NETFN                          (0x2E)
#define OEM_AMI_NETFN                           NETFN_AMI
#define OEM_MIN_RANGE                           "0x30"
#define OEM_MAX_RANGE                           "0x3F"

#define TEMP_MAX_REQ_SIZE  256

#define MAX_REQ_SIZE            (0x5000)
#define MAX_RES_SIZE            (0x3000)



#define YAFU_CC_NORMAL                                          0X00
#define YAFU_CC_INVALID_DATLEN                  0Xcc
#define YAFU_CC_ALLOC_ERR                                      0x02
#define YAFU_CC_DEV_OPEN_ERR                            0x03
#define YAFU_CC_SEEK_ERR                                       0x04
#define YAFU_CC_READ_ERR                                       0x05
#define YAFU_CC_WRITE_ERR                                     0x06
#define YAFU_CC_MEM_ERASE_ERR                           0x07
#define YAFU_CC_IN_DEACTIVATE                       0X08
#define YAFU_OFFSET_NOT_IN_ERASE_BOUNDARY     0x09
#define YAFU_SIZE_NOT_IN_ERASE_BOUNDARY  0X10
#define YAFU_FLASH_ERASE_FAILURE                 0X11
#define YAFU_ERR_STATE                                     0x13
#define YAFU_ECF_PROGRESS       0x14
#define YAFU_ECF_SUCCESS        0x15
#define YAFU_VERIFY_SUCCESS     0x16
#define YAFU_VERIFY_PROGRESS 0x17
#define YAFU_CC_GET_MEM_ERR     0x18
#define YAFU_CC_INVALID_SIGN_IMAGE 0x20
#define YAFU_CC_SIGNED_SUPP_NOT_ENABLED 0x21
#define YAFU_CC_DEV_IN_FIRMWARE_UPDATE_MODE 0xd1
#define YAFU_CC_FLASH_SAME_IMAGE            0x24
#define YAFU_CC_FLASHER_NOT_READY           0x25
#ifdef MSDOS
#define IPMI20_SESSION_T BYTE
#endif

/* This Error Codes are specific to Yafu Tool*/
#define YAFU_FW_MOD_NOT_FOUND 0x1
#define YAFU_GREATER_IMAGE_SIZE 0x2
#define YAFU_GET_DUAL_IMAGE_FAILED 0x3
#define YAFU_IMAGE_CHKSUM_VERIFY_FAILED 0x4
#define YAFU_FILE_OPEN_ERR 0x5
#define YAFU_INVALID_NAME 0x6
#define YAFU_NAME_LONG 0x7
#define YAFU_CC_IMAGE_SIZE_INVALID 0x8
#define YAFU_COMMAND_TIMEOUT_ERR 0x9
#define YAFU_INVALID_IP 0x10
#define YAFU_CC_IMAGE_SIGNED       0x0A
#define YAFU_CC_NOT_SIGNED_IMAGE   0x0B
#define YAFU_INVALID_PORT_NUMBER 0x0C
#define CC_REQ_INV_LEN             0xC7
#define CFG_PROJ_USED_FLASH_START 0x0

#define MAX_MODULE                       16
#define MAX_GETBOOTVAR 400
#define MAX_GETBOOTVAL 400
#define YAFU_HEADER_SIZE          12
//#define MAX_IPMI_MESSAGE_SIZE							1024 * 60 // 0xf000
//#define MAX_SIZE_TO_READ                0xF000
#define MAX_WRITEBUFLEN				(1024 * 60)		//MAX_IPMI_MESSAGE_SIZE // 参考上面的分包大小
#define GETCURACTIVEIMG   0x7
#define SETFWUPLOADSELECTOR         0x3
#define GETFWUPLOADSELECTOR         0x4

extern INT16U ECFPercent;
extern INT16U VerifyPercent;
extern INT16U EFStart;
extern INT16U Progressdis;
extern INT16U SilentFlash;
extern int  EnvWritten;
extern INT8U OemNetFn;

extern int FlashFullFirmwareImage(IPMI20_SESSION_T *hSession,FILE *img,int Config,int Boot);
#if defined (__x86_64__) || defined (WIN64)
int FlashModule(IPMI20_SESSION_T *hSession,FILE *img,unsigned int SizetoCopy,INT32U CurrFMHLoc,INT32U FMHLocation,INT8U *ModName);
#else
int FlashModule(IPMI20_SESSION_T *hSession,FILE *img, unsigned long SizetoCopy,INT32U CurrFMHLoc,INT32U FMHLocation,INT8U *ModName);
#endif

#if defined (__x86_64__) || defined (WIN64)
extern int FlashInfo(IPMI20_SESSION_T *hSession,unsigned int *EraseBlkSize,unsigned int *FlashSize);
#else
extern int FlashInfo(IPMI20_SESSION_T *hSession,unsigned long *EraseBlkSize,unsigned long *FlashSize);
#endif
extern int ActivateFlashMode(IPMI20_SESSION_T *hSession);
#if defined (__x86_64__) || defined (WIN64)
extern int MemoryAllocation (IPMI20_SESSION_T *hSession,unsigned int SizeToAlloc );
extern int WritetoMemory (IPMI20_SESSION_T *hSession,unsigned int AddofAlloc ,INT16U Datalen,char *Buf);
extern int EraseAndFlash(IPMI20_SESSION_T *hSession,unsigned int WriteMemOff,unsigned int FlashOffset,unsigned int Sizetocpy);
#else
extern int MemoryAllocation (IPMI20_SESSION_T *hSession,unsigned long SizeToAlloc );
extern int WritetoMemory (IPMI20_SESSION_T *hSession,unsigned long AddofAlloc ,INT16U Datalen,char *Buf);
extern int EraseAndFlash(IPMI20_SESSION_T *hSession,unsigned long WriteMemOff,unsigned long FlashOffset,unsigned long Sizetocpy);
#endif
extern int ECFStatus(IPMI20_SESSION_T *hsession_t);
extern int GetYafuVersion(IPMI20_SESSION_T *hsession_t);
extern int VerifyStatus(IPMI20_SESSION_T *hsession_t);
extern int GetDualImageSupport(IPMI20_SESSION_T *hSession, int Parameter,int *DualImageRes);
extern int SetDualImageConfig(IPMI20_SESSION_T *hSession,int Parameter,int DualImageReq);
extern int DualImageSettings(IPMI20_SESSION_T *hSession,int preserveconf);
#if defined (__x86_64__) || defined (WIN64)
extern int VerifyFlash(IPMI20_SESSION_T *hSession,unsigned int MemOffset,unsigned int Flashoffset,unsigned int Sizetoverify);
#else
extern int VerifyFlash(IPMI20_SESSION_T *hSession,unsigned long MemOffset,unsigned long Flashoffset,unsigned long Sizetoverify);
#endif
extern int ResetDevice (IPMI20_SESSION_T *hSession,INT16U WaitTime);
extern int DeactivateFlshMode (IPMI20_SESSION_T *hSession);
extern int protectFlash(IPMI20_SESSION_T *hSession,INT32U Blknum,INT8U Protect);
extern int FreeMemory(IPMI20_SESSION_T *hSession,INT32U WriteMemOff);
extern int  FlashModHeadInfo(IPMI20_SESSION_T *hSession,AMIYAFUGetFMHInfoRes_T* pFMHInfoRes);
extern int  GetStatus (IPMI20_SESSION_T *hSession);
extern int ModuleUpgrade(IPMI20_SESSION_T *hSession,FILE *img,int Module, int FlashFlag,int CurNumModule,int MaxNumModule);
extern int GetBootConfig (IPMI20_SESSION_T *hSession,char *BootVariables,char *BootVar);
extern int SetBootConfig (IPMI20_SESSION_T *hSession,char *BoorVar,char *BootVal);
extern int GetAllBootVars (IPMI20_SESSION_T *hSession,unsigned char *BootVars,INT16U *BootVarsCount);
extern int GetAllPreserveConfStatus( IPMI20_SESSION_T *hSession ,unsigned short *status, unsigned short *enabledstatus);
extern int SetAllPreserveConfStatus(IPMI20_SESSION_T *hSession, unsigned short Status);
extern int OnEnableUSBDevice();
extern int GetFirmwareVersion(IPMI20_SESSION_T *hSession, INT8U *pResponse);
extern int SendMiscellaneousInfo(IPMI20_SESSION_T *hSession, INT8U PreserveFlag, AMIYAFUMiscellaneousRes_T *pRes);
extern int GetPreserveConfStatus(IPMI20_SESSION_T *hSession, unsigned short selector);

#if defined (__x86_64__) || defined (WIN64)
int SearchFlashInfo(IPMI20_SESSION_T *hSession,unsigned int *EraseBlkSize,unsigned int *FlashSize);
#else
int SearchFlashInfo(IPMI20_SESSION_T *hSession,unsigned long *EraseBlkSize,unsigned long *FlashSize);
#endif
extern int CloseFlashInfo(IPMI20_SESSION_T *hSession);

extern int  ReadBackupConfnBkconf(IPMI20_SESSION_T *hSession,int ReadFlag);

#endif /* __FLASH_CMDS_H__ */

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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "main.h"
#include "flashcmds.h"
#include "bmc_ifc.h"
#include "debug.h"
#include "err_codes.h"
#include "meupdate.h"

#ifdef ICC_OS_LINUX
#include <termios.h>
#include <unistd.h>
#endif

#include "AMIRestoreDefaults.h"
#include "fmh.h"
#ifndef MSDOS
#include "libipmi_AMIOEM.h"
#else
#include "IPMI_AMISyslogConf.h"
#include "IPMI_AMIDevice.h"
#include "flashlib.h"
#endif

int RecoveryFlashMode = 0;
int Mode = 0;
SystemIfcReq_T IfcReqHdr;
extern INT8U byMedium;
extern int ModuleCount;
extern FlashMH *CurrFwModHdr[MAX_MODULE];
extern FlashMH *FwModuleHdr[MAX_MODULE];
extern int ImgOpt;
unsigned char *ConfData, *BkupConfData;

extern unsigned int EraseBlkSize;

extern FEATURE_LIST featuresList; // list of features supported by BMC

#if defined(__x86_64__) || defined(WIN64)
unsigned int CalculateChksum(char *data, unsigned int size);
unsigned int AddofAllocMem;
unsigned int ActualCopySize;
unsigned int ActualSizetoCopy;
unsigned int ActualConfig;
#else
unsigned long CalculateChksum(char *data, unsigned long size);
unsigned long AddofAllocMem;
unsigned long ActualCopySize;
unsigned long ActualSizetoCopy;
unsigned long ActualConfig;
#endif

char wbArr[MAX_WRITEBUFLEN]; // memory for WriteBuffer
extern int SPIDevice;
extern int DOMMCUpdate;
extern int IsRunLevelON;
extern int SignedImageSup;

INT16U ECFPercent;
INT16U VerifyPercent;
INT16U EFStart;
INT16U Progressdis;
INT16U SilentFlash;
int EnvWritten;
extern int ModuleType;
extern INT16U BootOnly;

#if defined(__x86_64__) || defined(WIN64)
int SwitchFlashDevice(IPMI20_SESSION_T *hSession, unsigned int *EraseBlkSize, unsigned int *FlashSize)
#else
int SwitchFlashDevice(IPMI20_SESSION_T *hSession, unsigned long *EraseBlkSize, unsigned long *FlashSize)
#endif
{

    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;
    int errVal;
    AMIYAFUSwitchFlashDeviceReq_T *pAMIYAFUSwitchFlashDeviceReq = NULL;
    AMIYAFUSwitchFlashDeviceRes_T pAMIYAFUSwitchFlashDeviceRes;

    pAMIYAFUSwitchFlashDeviceReq = (AMIYAFUSwitchFlashDeviceReq_T *)ReqBuf;

    ReqLen = sizeof(AMIYAFUSwitchFlashDeviceReq_T);
    ResLen = sizeof(AMIYAFUSwitchFlashDeviceRes_T);

    pAMIYAFUSwitchFlashDeviceReq->SPIDevice = SPIDevice;
    if (byMedium == KCS_MEDIUM)
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_SWITCH_FLASH_DEVICE, ReqBuf, ReqLen);
        errVal = SendTimedImbpRequest(&IfcReqHdr, 1000, ResBuf, &ResLen, &CompCode);
        memcpy(&pAMIYAFUSwitchFlashDeviceRes.MTDName, ResBuf, ResLen);
    }
    else
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIYAFUSwitchFlashDevice(hSession, pAMIYAFUSwitchFlashDeviceReq, &pAMIYAFUSwitchFlashDeviceRes, RECIEVE_TIME_OUT);
        CompCode = pAMIYAFUSwitchFlashDeviceRes.CompletionCode;
#endif
    }
    /* Handle the case of Invalid command. If command not available in older fw., this condition occurs */

    if (CompCode == 0xC1)
    {
        return 0;
    }

    if (ACCESN_OK != errVal)
    {
        SET_ERR_CODE(ERR_BMC_COMM);
        return -1;
    }

    *EraseBlkSize = pAMIYAFUSwitchFlashDeviceRes.FlashInfo.FlashEraseBlkSize;
    *FlashSize = pAMIYAFUSwitchFlashDeviceRes.FlashInfo.FlashSize;
    return 0;
}

#if defined(__x86_64__) || defined(WIN64)
int ActivateFlashDevice(IPMI20_SESSION_T *hSession, INT8U Activation)
#else
int ActivateFlashDevice(IPMI20_SESSION_T *hSession, INT8U Activation)
#endif
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[TEMP_MAX_REQ_SIZE];
    int errVal;
    int retval = 0;
    int ReqLen = 0, ResLen = 0;
    BYTE CompCode = 0;
    AMIYAFUActivateFlashDeviceReq_T *pAMIYAFUActivateFlashDeviceReq = NULL;
    AMIYAFUActivateFlashDeviceRes_T pAMIYAFUActivateFlashDeviceRes;

    pAMIYAFUActivateFlashDeviceReq = (AMIYAFUActivateFlashDeviceReq_T *)ReqBuf;
    pAMIYAFUActivateFlashDeviceReq->ActivateNode = Activation;
    ReqLen = sizeof(AMIYAFUGetFlashInfoReq_T);
    ResLen = sizeof(AMIYAFUGetFlashInfoRes_T);

    memset((char *)&pAMIYAFUActivateFlashDeviceRes, 0, sizeof(AMIYAFUActivateFlashDeviceRes_T));
    if (byMedium == KCS_MEDIUM)
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_ACTIVATE_FLASH, ReqBuf, ReqLen);
        errVal = SendTimedImbpRequest(&IfcReqHdr, 1000, ResBuf, &ResLen, &CompCode);
        pAMIYAFUActivateFlashDeviceRes.CompletionCode = CompCode;
    }
    else
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIYAFUActivateFlashDevice(hSession, pAMIYAFUActivateFlashDeviceReq, &pAMIYAFUActivateFlashDeviceRes, 5000);
#endif
    }
    /* Handle the case of Invalid command. If command not available in older fw., this condition occurs */
    if (pAMIYAFUActivateFlashDeviceRes.CompletionCode == 0xC1)
    {
        retval = 0;
        goto exit_gracefully;
    }

    // Failure in Configuring the CPLD will return this error code/completion code
    // This error code will trigger reflash/reconfigure of CPLD
    if (pAMIYAFUActivateFlashDeviceRes.CompletionCode == 0x08)
    {
        retval = -2;
        goto exit_gracefully;
    }

    if (pAMIYAFUActivateFlashDeviceRes.CompletionCode == 0x03)
    {
        retval = -3;
        goto exit_gracefully;
    }

    if (pAMIYAFUActivateFlashDeviceRes.CompletionCode == 0x23)
    {
        retval = -4;
        goto exit_gracefully;
    }

    if ((errVal < 0) || (pAMIYAFUActivateFlashDeviceRes.CompletionCode != 0x00))
        retval = -1;

exit_gracefully:
    return retval;
}

int RestoreFlashDevice(IPMI20_SESSION_T *hSession)
{
    int errVal, ResLen = 0, ReqLen = 0;
    BYTE CompCode;
    BYTE ResBuf[MAX_RES_SIZE];
    BYTE ReqBuf[MAX_REQ_SIZE];

    AMIYAFUSwitchFlashDeviceReq_T *pAMIYAFUSwitchFlashDeviceReq = NULL;
    AMIYAFUSwitchFlashDeviceRes_T *pAMIYAFUSwitchFlashDeviceRes = NULL;

    pAMIYAFUSwitchFlashDeviceRes = (AMIYAFUSwitchFlashDeviceRes_T *)ResBuf;
    pAMIYAFUSwitchFlashDeviceReq = (AMIYAFUSwitchFlashDeviceReq_T *)ReqBuf;

    pAMIYAFUSwitchFlashDeviceReq->SPIDevice = SPIDevice;

    ResLen = sizeof(AMIYAFUSwitchFlashDeviceRes_T);
    ReqLen = sizeof(AMIYAFUSwitchFlashDeviceReq_T);

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIYAFURestoreFlashDevice(hSession, pAMIYAFUSwitchFlashDeviceReq, pAMIYAFUSwitchFlashDeviceRes, RECIEVE_TIME_OUT);
        CompCode = pAMIYAFUSwitchFlashDeviceRes->CompletionCode;
#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_RESTORE_FLASH_DEVICE, ReqBuf, ReqLen);
        errVal = SendTimedImbpRequest(&IfcReqHdr, 1000, ResBuf, &ResLen, &CompCode);

        pAMIYAFUSwitchFlashDeviceRes = (AMIYAFUSwitchFlashDeviceRes_T *)ResBuf;
    }
    /* Handle the case of Invalid command. If command not available in older fw., this condition occurs */
    if (CompCode == 0xC1)
    {
        return 0;
    }
    if (ACCESN_OK != errVal)
    {
        SET_ERR_CODE(ERR_BMC_COMM);
        return -1;
    }

    return 0;
}

/*
 * @fn FlashInfo
 * @brief this function get the Flash information from the Firmware
 * @param hSession - Current Session Pointer
 * @param EraseBlkSize - Eraseable Block Size of Firmware
 * @param FlashSize - Flashable image Size of Firmware
 * @return Returns 0 on success
 */
#if defined(__x86_64__) || defined(WIN64)
int FlashInfo(IPMI20_SESSION_T *hSession, unsigned int *EraseBlkSize, unsigned int *FlashSize)
#else
int FlashInfo(IPMI20_SESSION_T *hSession, unsigned long *EraseBlkSize, unsigned long *FlashSize)
#endif
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen, errVal;
    BYTE CompCode;

    AMIYAFUGetFlashInfoReq_T *pAMIYAFUGetFlashInfoReq = NULL;
    AMIYAFUGetFlashInfoRes_T pAMIYAFUGetFlashInfoRes;

    pAMIYAFUGetFlashInfoReq = (AMIYAFUGetFlashInfoReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUGetFlashInfoReq_T);
    ResLen = sizeof(AMIYAFUGetFlashInfoRes_T);
    pAMIYAFUGetFlashInfoReq->FlashInfoReq.Seqnum = 0x00000001;
    pAMIYAFUGetFlashInfoReq->FlashInfoReq.YafuCmd = CMD_AMI_YAFU_GET_FLASH_INFO;
    pAMIYAFUGetFlashInfoReq->FlashInfoReq.Datalen = 0x00;
    pAMIYAFUGetFlashInfoReq->FlashInfoReq.CRC32chksum = 0x00;

    if (byMedium == KCS_MEDIUM)
    {

        FrameIfcReqHdr(CMD_AMI_YAFU_GET_FLASH_INFO, ReqBuf, ReqLen);
        errVal = SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode);
        memcpy(&(pAMIYAFUGetFlashInfoRes.FlashInfoRes), ResBuf, ResLen);
    }
    else
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIYAFUGetFlashInfo(hSession, pAMIYAFUGetFlashInfoReq, &pAMIYAFUGetFlashInfoRes, RECIEVE_TIME_OUT);
#endif
    }
    if (ACCESN_OK != errVal)
    {
        SET_ERR_CODE(ERR_BMC_COMM);
        return -1;
    }

    if (pAMIYAFUGetFlashInfoRes.FlashInfoRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
    {
        AMIYAFUNotAck pAMIYAFUNotAcknowledge;
        memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)&pAMIYAFUGetFlashInfoRes, sizeof(AMIYAFUNotAck));
        return -1;
    }

    if (CalculateChksum((char *)&pAMIYAFUGetFlashInfoRes.FlashInfo, sizeof(pAMIYAFUGetFlashInfoRes.FlashInfo)) != pAMIYAFUGetFlashInfoRes.FlashInfoRes.CRC32chksum)
        return -1;

    *EraseBlkSize = pAMIYAFUGetFlashInfoRes.FlashInfo.FlashEraseBlkSize;
    *FlashSize = pAMIYAFUGetFlashInfoRes.FlashInfo.FlashSize;
    return 0;
}

int GetFirmwareVersion(IPMI20_SESSION_T *hSession, INT8U *pResponse)
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[TEMP_MAX_REQ_SIZE];
    int errVal = 0;
    int ReqLen = 0, ResLen = 0;
    BYTE CompCode = 0;
    AMIGetFwVersionRes_T *pRes = (AMIGetFwVersionRes_T *)pResponse;
    AMIGetFwVersionReq_T *pAMIGetFwVersionReq = NULL;

    pAMIGetFwVersionReq = (AMIGetFwVersionReq_T *)ReqBuf;
    pAMIGetFwVersionReq->DeviceNode = SPIDevice;
    ReqLen = sizeof(AMIGetFwVersionReq_T);
    ResLen = MAX_FMHLENGTH;

    if (byMedium == KCS_MEDIUM)
    {
        FrameIfcReqHdr(CMD_AMI_GET_FW_VERSION, ReqBuf, ReqLen);
        if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 1000, ResBuf, &ResLen, &CompCode))
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }
        memcpy((char *)&(pRes->Count), (char *)&ResBuf, MAX_FMHLENGTH);
    }
    else
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIGetFwVersion(hSession, pAMIGetFwVersionReq, sizeof(AMIGetFwVersionReq_T), pRes, 5000);
#endif
    }

    if (ACCESN_OK != errVal)
    {
        SET_ERR_CODE(ERR_BMC_COMM);
        return -1;
    }
    return 0;
}

/*
 * @fn ActivateFlashMode
 * @brief This function is to Activate the Flash mode in Firmware
 * @param hSession - Current Session Pointer
 * @return Returns 0 on success
 */
int ActivateFlashMode(IPMI20_SESSION_T *hSession)
{

    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen, retry;
    int retVal;
    BYTE CompCode;

    AMIYAFUActivateFlashModeReq_T *pAMIYAFUActivateFlashReq = NULL;
    AMIYAFUActivateFlashModeRes_T pAMIYAFUActivateFlashRes;
    pAMIYAFUActivateFlashReq = (AMIYAFUActivateFlashModeReq_T *)&ReqBuf;
    ResLen = sizeof(AMIYAFUActivateFlashModeRes_T);
    ReqLen = sizeof(AMIYAFUActivateFlashModeReq_T);

    pAMIYAFUActivateFlashReq->ActivateflashReq.Seqnum = 0x00000001;
    pAMIYAFUActivateFlashReq->ActivateflashReq.YafuCmd = CMD_AMI_YAFU_ACTIVATE_FLASH;
    pAMIYAFUActivateFlashReq->ActivateflashReq.Datalen = 0x02;
    pAMIYAFUActivateFlashReq->Mode = BootOnly << 8;
    pAMIYAFUActivateFlashReq->Mode |= Mode;
    pAMIYAFUActivateFlashReq->ActivateflashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUActivateFlashReq->Mode, sizeof(INT16U));

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        int errVal = 0;

        errVal = IPMICMD_AMIYAFUActivateFlashMode(hSession, pAMIYAFUActivateFlashReq, &pAMIYAFUActivateFlashRes, RECIEVE_TIME_OUT);
        if (pAMIYAFUActivateFlashRes.CompletionCode == YAFU_CC_DEV_IN_FIRMWARE_UPDATE_MODE)
        {
            printf("Warning: Device is already in Firmware Update Mode.\n");
            
            /*pAMIYAFUActivateFlashReq->ActivateflashReq.YafuCmd = CMD_AMI_YAFU_DEACTIVATE_FLASH_MODE;
            pAMIYAFUActivateFlashReq->ActivateflashReq.CRC32chksum = CalculateChksum((char*)&pAMIYAFUActivateFlashReq->Mode, sizeof(INT16U));
            errVal = IPMICMD_AMIYAFUDeactivateFlash(hSession, pAMIYAFUActivateFlashReq, &pAMIYAFUActivateFlashRes, 3000);
            errVal = LIBIPMI_HL_AMIDeactivateFlashMode(hSession, 3000);*/
            errVal = DeactivateFlshMode(hSession);
            printf("Warning: Device exist Firmware Update Mode, errVal2 = %d\n", errVal);
            exit(YAFU_CC_DEV_IN_FIRMWARE_UPDATE_MODE);
        }
        if (errVal != 0)
        {
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
            return -1;
        }

#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_ACTIVATE_FLASH, ReqBuf, ReqLen);

        for (retry = 0; retry <= 3; retry++)
        {
            if (retry == 3)
            {
                SET_ERR_CODE(ERR_BMC_COMM);
                return -1;
            }
            else
            {
                retVal = SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode);
                if (CompCode == YAFU_CC_DEV_IN_FIRMWARE_UPDATE_MODE)
                {
                    printf("Warning: Device is already in Firmware Update Mode..\n");
                    exit(YAFU_CC_DEV_IN_FIRMWARE_UPDATE_MODE);
                }

                if (retVal != ACCESN_OK)
                    SleepMs(8000);
                else
                    break;
            }
        }
        memcpy(&pAMIYAFUActivateFlashRes.ActivateflashRes, ResBuf, ResLen);
    }

    if (pAMIYAFUActivateFlashRes.ActivateflashRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
    {
        AMIYAFUNotAck pAMIYAFUNotAcknowledge;
        memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)&pAMIYAFUActivateFlashRes, sizeof(AMIYAFUNotAck));
        SET_ERR_CODE(ERR_COMP_CODE);
        return -1;
    }
    return 0;
}

/*
 *@fn GetDualImageSupport
 *@brief This function is called to find out the Dual Image Support
 *@param hSession - Current Session Pointer
 *@param Paramter - Parameter for Dual Image Setting command
 *@param DualImageRes  Dual Image Setting Command response
 *@return Returns 0 on success
 *            Returns proper error code on failure
 */
int GetDualImageSupport(IPMI20_SESSION_T *hSession, int Parameter, int *DualImageRes)
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE] = {0};
    BYTE ResBuf[MAX_RES_SIZE] = {0};
    int ResLen, ReqLen, retry;
    BYTE CompCode;
    AMIDualImageSupReq_T *pAMIDualImageSupReq = (AMIDualImageSupReq_T *)ReqBuf;
    AMIDualImageSupRes_T pAMIDualImageSupRes;
#ifndef MSDOS
    int errVal;
#endif

    ResLen = sizeof(AMIDualImageSupRes_T);
    ReqLen = sizeof(AMIDualImageSupReq_T) - 1;

    if (Parameter == GETCURACTIVEIMG)
    {
        pAMIDualImageSupReq->Parameter = GETCURACTIVEIMG;
        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIDualImageSupport(hSession, pAMIDualImageSupReq, &pAMIDualImageSupRes, ReqLen, RECIEVE_TIME_OUT);
            if (errVal != 0)
            {
                if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
                {
                    printf("Exiting as IPMI Session timed out due to inactivity\n");
                    exit(YAFU_COMMAND_TIMEOUT_ERR);
                }
                return errVal;
            }
            else
            {
                if (pAMIDualImageSupRes.CompletionCode != 0)
                    return pAMIDualImageSupRes.CompletionCode;
                *DualImageRes = pAMIDualImageSupRes.BootSelOpt.GetCurActiveImg;
            }
#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_DUAL_IMG_SUPPORT, ReqBuf, ReqLen);

            for (retry = 0; retry <= 3; retry++)
            {

                if (retry == 3)
                {
                    SET_ERR_CODE(ERR_BMC_COMM);
                    return -1;
                }
                else
                {
                    if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode))
                        SleepMs(8000);
                    else
                        break;
                }
            }
            if (CompCode != 0)
                return CompCode;
            memcpy(&(pAMIDualImageSupRes.BootSelOpt), ResBuf, ResLen);
            *DualImageRes = pAMIDualImageSupRes.BootSelOpt.GetCurActiveImg;
        }
    }
    else if (Parameter == GETFWUPLOADSELECTOR)
    {
        pAMIDualImageSupReq->Parameter = GETFWUPLOADSELECTOR;
        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIDualImageSupport(hSession, pAMIDualImageSupReq, &pAMIDualImageSupRes, ReqLen, RECIEVE_TIME_OUT);
            if (errVal != 0)
            {
                if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
                {
                    printf("Exiting as IPMI Session timed out due to inactivity\n");
                    exit(YAFU_COMMAND_TIMEOUT_ERR);
                }
                return errVal;
            }
            else
            {
                if (pAMIDualImageSupRes.CompletionCode != 0)
                    return pAMIDualImageSupRes.CompletionCode;
                *DualImageRes = pAMIDualImageSupRes.BootSelOpt.GetUploadSelector;
            }
#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_DUAL_IMG_SUPPORT, ReqBuf, ReqLen);

            for (retry = 0; retry <= 3; retry++)
            {

                if (retry == 3)
                {
                    SET_ERR_CODE(ERR_BMC_COMM);
                    return -1;
                }
                else
                {
                    if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode))
                        SleepMs(8000);
                    else
                        break;
                }
            }
            if (CompCode != 0)
                return CompCode;
            memcpy(&(pAMIDualImageSupRes.BootSelOpt), ResBuf, ResLen);
            *DualImageRes = pAMIDualImageSupRes.BootSelOpt.GetUploadSelector;
        }
    }
    return 0;
}

/*
 *@fn SetDualImageConfig
 *@brief Helps in setting Dual Image Configurations
 *@param hSession - Current Session Pointer
 *@param Paramter - Parameter for Dual Image Setting command
 *@param DualImageReq  Dual Image Setting Command request
 *@return Returns 0 on success
 *            Returns proper error code on failure
 */
int SetDualImageConfig(IPMI20_SESSION_T *hSession, int Parameter, int DualImageReq)
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;

    AMIDualImageSupReq_T *pAMIDualImageSupReq = (AMIDualImageSupReq_T *)ReqBuf;
    AMIDualImageSupRes_T pAMIDualImageSupRes;

    ResLen = sizeof(AMIDualImageSupRes_T);
    ReqLen = sizeof(AMIDualImageSupReq_T);

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        int errVal = 0;
        if (Parameter == SETFWUPLOADSELECTOR)
        {
            memset((char *)pAMIDualImageSupReq, 0, sizeof(AMIDualImageSupReq_T));
            memset((char *)&pAMIDualImageSupRes, 0, sizeof(AMIDualImageSupRes_T));
            pAMIDualImageSupReq->Parameter = SETFWUPLOADSELECTOR;
            pAMIDualImageSupReq->BootSelector = DualImageReq;
            errVal = IPMICMD_AMIDualImageSupport(hSession, pAMIDualImageSupReq, &pAMIDualImageSupRes, ReqLen, RECIEVE_TIME_OUT);
            if (errVal != 0)
            {
                if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
                {
                    printf("Exiting as IPMI Session timed out due to inactivity\n");
                    exit(YAFU_COMMAND_TIMEOUT_ERR);
                }
                return errVal;
            }
        }
#endif
    }
    else
    {

        if (Parameter == SETFWUPLOADSELECTOR)
        {
            pAMIDualImageSupReq->Parameter = SETFWUPLOADSELECTOR;
            pAMIDualImageSupReq->BootSelector = DualImageReq;

            FrameIfcReqHdr(CMD_AMI_DUAL_IMG_SUPPORT, ReqBuf, ReqLen);

            if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode))
            {
                SET_ERR_CODE(ERR_BMC_COMM);
                return -1;
            }
            memcpy(&pAMIDualImageSupRes.BootSelOpt, ResBuf, ResLen);
        }
    }
    return 0;
}

/*
 *@fn DualImageSettings
 *@brief YAFU related Dual Image Settings
 *@param hSession - Current Session Pointer
 *@return Returns 0 on Success
 *            Returns proper error code on failure
 */
int DualImageSettings(IPMI20_SESSION_T *hSession, int preserveconf)
{

    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;

    AMIYAFUDualImgSupReq_T *pAMIYAFUDualImgSupReq = (AMIYAFUDualImgSupReq_T *)ReqBuf;
    AMIYAFUDualImgSupRes_T pAMIYAFUDualImgSupRes;
    ReqLen = sizeof(AMIYAFUDualImgSupReq_T);
    ResLen = sizeof(AMIYAFUDualImgSupRes_T);

    pAMIYAFUDualImgSupReq->DualImgSupReq.Seqnum = 0x00000001;
    pAMIYAFUDualImgSupReq->DualImgSupReq.YafuCmd = CMD_AMI_YAFU_DUAL_IMAGE_SUP;
    pAMIYAFUDualImgSupReq->DualImgSupReq.Datalen = sizeof(AMIYAFUDualImgSupReq_T) - sizeof(YafuHeader);
    pAMIYAFUDualImgSupReq->PreserveConf = preserveconf;
    pAMIYAFUDualImgSupReq->DualImgSupReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUDualImgSupReq->PreserveConf, sizeof(AMIYAFUDualImgSupReq_T) - sizeof(YafuHeader));

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        int errVal;

        errVal = IPMICMD_AMIYAFUDualImageSupport(hSession, pAMIYAFUDualImgSupReq, &pAMIYAFUDualImgSupRes, RECIEVE_TIME_OUT);
        if (errVal != 0)
        {
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
            return errVal;
        }
#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_DUAL_IMAGE_SUP, ReqBuf, ReqLen);

        if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode))
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }

        // pAMIYAFUDualImgSupRes = (AMIYAFUDualImgSupRes_T*)&ResBuf;
        memcpy(&pAMIYAFUDualImgSupRes.DualImgSupRes, ResBuf, ResLen);
        if (pAMIYAFUDualImgSupRes.DualImgSupRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
        {
            AMIYAFUNotAck pAMIYAFUNotAcknowledge;
            memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)&pAMIYAFUDualImgSupRes, sizeof(AMIYAFUNotAck));
            SET_ERR_CODE(ERR_COMP_CODE);
            return -1;
        }
    }
    return 0;
}

/*
 * @fn MemoryAllocation
 * @brief This function Allocate the Required Memory in the RAM
 * @param hSession - Current Session Pointer
 * @param SizeToAlloc - Size to Allocate Memory in RAM
 * @return Returns 0 on success
 */
#if defined(__x86_64__) || defined(WIN64)
int MemoryAllocation(IPMI20_SESSION_T *hSession, unsigned int SizeToAlloc)
#else
int MemoryAllocation(IPMI20_SESSION_T *hSession, unsigned long SizeToAlloc)
#endif
{
    int RetryCount = 0;
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;
    int errVal = 0;

    AMIYAFUAllocateMemoryReq_T *pAMIYAFUAllocateMemoryReq = (AMIYAFUAllocateMemoryReq_T *)ReqBuf;
    AMIYAFUAllocateMemoryRes_T pAMIYAFUAllocateMemoryRes;
    ReqLen = sizeof(AMIYAFUAllocateMemoryReq_T);
    ResLen = sizeof(AMIYAFUAllocateMemoryRes_T);

    while (1)
    {
        pAMIYAFUAllocateMemoryReq->AllocmemReq.Seqnum = 0x00000001;
        pAMIYAFUAllocateMemoryReq->AllocmemReq.YafuCmd = CMD_AMI_YAFU_ALLOCATE_MEMORY;
        pAMIYAFUAllocateMemoryReq->AllocmemReq.Datalen = 0x04;
        pAMIYAFUAllocateMemoryReq->Sizeofmemtoalloc = SizeToAlloc;
        pAMIYAFUAllocateMemoryReq->AllocmemReq.CRC32chksum = CalculateChksum((unsigned char *)&pAMIYAFUAllocateMemoryReq->Sizeofmemtoalloc, sizeof(INT32U));

        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIYAFUAllocateMemory(hSession, pAMIYAFUAllocateMemoryReq, &pAMIYAFUAllocateMemoryRes, RECIEVE_TIME_OUT);
            if (errVal != 0)
                return -1;
#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_YAFU_ALLOCATE_MEMORY, ReqBuf, ReqLen);

            if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode))
            {
                SET_ERR_CODE(ERR_BMC_COMM);
                return -1;
            }
            // pAMIYAFUAllocateMemoryRes = (AMIYAFUAllocateMemoryRes_T*)&ResBuf;
            memcpy(&pAMIYAFUAllocateMemoryRes.AllocmemRes, ResBuf, ResLen);
        }
        if (pAMIYAFUAllocateMemoryRes.AllocmemRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
        {
            AMIYAFUNotAck pAMIYAFUNotAcknowledge;
            memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)&pAMIYAFUAllocateMemoryRes, sizeof(AMIYAFUNotAck));

            if (pAMIYAFUNotAcknowledge.ErrorCode == YAFU_CC_GET_MEM_ERR)
            {
                if (vdbg)
                {
                    printf("\nWarning: YAFUAllocateMemory failed %d \n", errVal);
                    fflush(stdout);
                }
                RetryCount += 1;
                if (RetryCount <= RETRYCOUNT)
                {
                    if (vdbg)
                    {
                        printf("Retry #%d\n", RetryCount);
                        fflush(stdout);
                    }
                    continue;
                }
                else
                {
                    if (vdbg)
                    {
                        printf("Error: Retry Count exceeded. So trying with block by block mode\n");
                        fflush(stdout);
                    }
                    AddofAllocMem = 0xfffffffe;
                    return 0;
                }
            }
            else
            {
                SET_ERR_CODE(ERR_COMP_CODE);
                return -1;
            }
        }
        else
        {
            break; // no Error
        }
    }

    if (CalculateChksum((char *)&pAMIYAFUAllocateMemoryRes.Addofallocmem, sizeof(INT32U)) != pAMIYAFUAllocateMemoryRes.AllocmemRes.CRC32chksum)
    {
        return -1;
    }

    AddofAllocMem = pAMIYAFUAllocateMemoryRes.Addofallocmem;
    IsRunLevelON = 1;
    return 0;
}

/*
 * @fn ReplaceSignedImageKey
 * @brief this function Sets the Public Key in the Existing Firmware
 * @param hSession - Current Session Pointer
 * @param imgName - Public Key File Name
 * @param timeout - timeout value to recieve data from firmware
 * @return Returns 0 on success
 */
int ReplaceSignedImageKey(IPMI20_SESSION_T *hSession, char *FileName, int timeout)
{
    FILE *fp = NULL;
    unsigned char Buf[8 * 1024] = {0};
    int Size = 0;

    BYTE ResBuf[TEMP_MAX_REQ_SIZE];
    int ResLen;
    BYTE CompCode;

    fp = fopen(FileName, "r");
    if (fp == NULL)
    {
        printf("Error Opening Public Key File: %s\n", FileName);
        return -1;
    }

    Size = fread(&Buf[0], 1, sizeof(Buf), fp);
    if (Size <= 0)
    {
        printf("Public Key might be Corrupted or No Data!!!!! Please upload a different Key\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        return (LIBIPMI_HL_AMIYAFUReplaceSignedImageKey(hSession, &Buf[0], Size, timeout));
#endif
    }
    else
    {
        IfcReqHdr.cmdType = CMD_AMI_YAFU_SIGNIMAGEKEY_REPLACE;
        IfcReqHdr.rsSa = BMC_SLAVE_ADDR;
        IfcReqHdr.rsLun = BMC_LUN;
        IfcReqHdr.netFn = OEM_AMI_NETFN;
        IfcReqHdr.busType = PUBLIC_BUS;
        IfcReqHdr.data = Buf;
        IfcReqHdr.dataLength = Size;
        ResLen = 0;

        if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode))
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }

        return CompCode;
    }

    return 0;
}

/*
 * @fn WritetoMemory
 * @brief this function is to write the data in to the Allocated memory in RAM
 * @param hSession - Current Session Pointer
 * @param AddofAlloc - Allocated memory to Add the Data in RAM
 * @param Datalen - Length of data to be written in RAM
 * @param  Buf -Holds the data to be written in RAM
 * @return Returns 0 on success
 */

#if defined(__x86_64__) || defined(WIN64)
int WritetoMemory(IPMI20_SESSION_T *hSession, unsigned int AddofAlloc, INT16U Datalen, char *Buf)
#else
int WritetoMemory(IPMI20_SESSION_T *hSession, unsigned long AddofAlloc, INT16U Datalen, char *Buf)
#endif
{
    int RetryCount_NAK = 0;
    int RetryCount_CSUM = 0;
    int RetryCount = 0;
    int errVal = -1;
    
    int printOnceAlread = 0;
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;
    char *WriteBuffer = NULL;

    AMIYAFUWriteMemoryReq_T *pAMIYAFUWriteMemoryReq = NULL;
    AMIYAFUWriteMemoryRes_T pAMIYAFUWriteMemoryRes;

    WriteBuffer = wbArr;
    ReqLen = sizeof(AMIYAFUWriteMemoryReq_T) + Datalen;
    ResLen = sizeof(AMIYAFUWriteMemoryRes_T);

    if (AddofAlloc == 0x9e3fb008) {
        //printf("\n WritetoMemory  start , AddofAlloc=%#llx", AddofAlloc);
    }
    while (1)
    {
        memset(WriteBuffer, 0, MAX_WRITEBUFLEN);
        pAMIYAFUWriteMemoryReq = (AMIYAFUWriteMemoryReq_T *)WriteBuffer;

        pAMIYAFUWriteMemoryReq->WriteMemReq.Seqnum = 0x00000001;
        pAMIYAFUWriteMemoryReq->WriteMemReq.YafuCmd = CMD_AMI_YAFU_WRITE_MEMORY;
        pAMIYAFUWriteMemoryReq->WriteMemReq.Datalen = 5 + Datalen;
        pAMIYAFUWriteMemoryReq->Memoffset = AddofAlloc;
        pAMIYAFUWriteMemoryReq->WriteWidth = 0x8;
        memcpy((WriteBuffer + sizeof(AMIYAFUWriteMemoryReq_T)), Buf, Datalen);
        pAMIYAFUWriteMemoryReq->WriteMemReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUWriteMemoryReq->Memoffset, pAMIYAFUWriteMemoryReq->WriteMemReq.Datalen);

        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS

            //printf("\n WritetoMemory  Datalen = %#x , AddofAlloc=%#llx\r\n", Datalen, AddofAlloc);// 走这个分支 
            errVal = IPMICMD_AMIYAFUWriteMemory(hSession, (AMIYAFUWriteMemoryReq_T *)WriteBuffer, &pAMIYAFUWriteMemoryRes, RECIEVE_TIME_OUT);
#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_YAFU_WRITE_MEMORY, WriteBuffer, ReqLen);
            errVal = SendTimedImbpRequest(&IfcReqHdr, 10000, ResBuf, &ResLen, &CompCode);
            memcpy(&pAMIYAFUWriteMemoryRes.WriteMemRes, ResBuf, ResLen);
        }

        if (errVal != ACCESN_OK)
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            if (vdbg)
            {
                printf("\nWarning: WritetoMemory failed %d \n", errVal);
                fflush(stdout);
            }
            RetryCount += 1;

            if (RetryCount == 1)
            {
                printf("\nWarning: WritetoMemory failed RetryCount at 1,errVal = %d, will try again \n", errVal);
            } else if (RetryCount <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                printf("Error: Retry Count exceeded. Aborting, line = %d, errVal = %d\n", __LINE__, errVal);
                fflush(stdout);
                return -1;
            }
        }
        else
        {
            RetryCount = 0;
        }
        if (pAMIYAFUWriteMemoryRes.WriteMemRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
        {
            if (vdbg)
            {
                printf("\nWarning: WritetoMemory - AMI_YAFU_COMMON_NAK\n");
                fflush(stdout);
            }
            RetryCount_NAK += 1;
            if (RetryCount_NAK <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount_NAK);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            RetryCount_NAK = 0;
        }

        if (CalculateChksum((char *)&pAMIYAFUWriteMemoryRes.SizeWritten, sizeof(INT16U)) != pAMIYAFUWriteMemoryRes.WriteMemRes.CRC32chksum)
        {
            if (vdbg)
            {
                printf("\nWarning: WritetoMemory - Checksum error\n");
                fflush(stdout);
            }
            RetryCount_CSUM += 1;
            if (RetryCount_CSUM <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount_CSUM);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            // No errors, we can exit the while (1)
            break;
        }
    } // while (1)
    return 0;
}
/*
 * @fn EraseAndFlash
 * @brief this function erases the required sectors and then copies the contents
        from memory to flash.
 * @param hSession - Current Session Pointer
 * @param WriteMemOff - Source Memory offset of the RAM
 * @param FlashOffset - Destination FLASH Offset of the Flash
 * @param  Sizetocpy - Size to Copy from RAM to Flash
 *@param PreserveFlag - Preserving module
 * @return Returns 0 on success
*/
#if defined(__x86_64__) || defined(WIN64)
int EraseAndFlash(IPMI20_SESSION_T *hSession, unsigned int WriteMemOff, unsigned int FlashOffset, unsigned int Sizetocpy)
#else
int EraseAndFlash(IPMI20_SESSION_T *hSession, unsigned long WriteMemOff, unsigned long FlashOffset, unsigned long Sizetocpy)
#endif
{
    int RetryCount_NAK = 0;
    int RetryCount_CSUM = 0;
    INT16U ErrorCode = 0;
    int RetryCount_IPMB_REQ = 0;
    int errVal = -1;
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;
#ifndef MSDOS
    int RetryCount_EC = 0;
#endif

    AMIYAFUEraseCopyFlashReq_T *pAMIYAFUEraseCopyFlashReq = NULL;
    AMIYAFUEraseCopyFlashRes_T pAMIYAFUEraseCopyFlashRes;
    pAMIYAFUEraseCopyFlashReq = (AMIYAFUEraseCopyFlashReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUEraseCopyFlashReq_T);
    ResLen = sizeof(AMIYAFUEraseCopyFlashRes_T);

    while (1)
    {
        pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Seqnum = 0x00000001;
        pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.YafuCmd = CMD_AMI_YAFU_ERASE_COPY_FLASH;
        pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.Datalen = 0x0c;
        pAMIYAFUEraseCopyFlashReq->Memoffset = WriteMemOff;
        pAMIYAFUEraseCopyFlashReq->Flashoffset = FlashOffset;
        pAMIYAFUEraseCopyFlashReq->Sizetocopy = Sizetocpy;
        pAMIYAFUEraseCopyFlashReq->EraseCpyFlashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUEraseCopyFlashReq->Memoffset, (3 * sizeof(INT32U)));

        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIYAFUEraseCopyFlash(hSession, pAMIYAFUEraseCopyFlashReq, &pAMIYAFUEraseCopyFlashRes, RECIEVE_TIME_OUT);
            if (errVal != 0)
            {
                if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
                {
                    printf("Exiting as IPMI Session timed out due to inactivity\n");
                    exit(YAFU_COMMAND_TIMEOUT_ERR);
                }
                if (vdbg)
                {
                    printf("\nWarning: YAFUEraseCopyFlash failed %d \n", errVal);
                    fflush(stdout);
                }
                RetryCount_EC += 1;
                if (RetryCount_EC <= RETRYCOUNT)
                {
                    if (vdbg)
                    {
                        printf("Retry #%d\n", RetryCount_EC);
                        fflush(stdout);
                    }
                    continue;
                }
                else
                {
                    if (vdbg)
                    {
                        printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                        fflush(stdout);
                    }
                    return -1;
                }
            }
            else
            {
                RetryCount_EC = 0;
            }
#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_YAFU_ERASE_COPY_FLASH, ReqBuf, ReqLen);

            errVal = SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode);
            if (errVal != ACCESN_OK)
            {
                SET_ERR_CODE(ERR_BMC_COMM);
                if (vdbg)
                {
                    printf("\nWarning: EraseAndFlash failed %d \n", errVal);
                    fflush(stdout);
                }
                RetryCount_IPMB_REQ += 1;
                if (RetryCount_IPMB_REQ <= RETRYCOUNT)
                {
                    if (vdbg)
                    {
                        printf("Retry #%d\n", RetryCount_IPMB_REQ);
                        fflush(stdout);
                    }
                    continue;
                }
                else
                {
                    if (vdbg)
                    {
                        printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                        fflush(stdout);
                    }
                    return -1;
                }
            }
            else
            {
                RetryCount_IPMB_REQ = 0;
            }
            // pAMIYAFUEraseCopyFlashRes = (AMIYAFUEraseCopyFlashRes_T*)ResBuf;
            memcpy(&pAMIYAFUEraseCopyFlashRes.EraseCpyFlashRes, ResBuf, ResLen);
        }

        if (pAMIYAFUEraseCopyFlashRes.EraseCpyFlashRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
        {
            if (SignedImageSup == 1)
            {
                /* Parse the Error Code to find out the Error */
                ErrorCode = pAMIYAFUEraseCopyFlashRes.Sizecopied & 0x0000FFFF;
                if (ErrorCode == YAFU_CC_INVALID_SIGN_IMAGE)
                {
                    printf("The Uploaded Signed Image or Public key might be wrong !!! Kindly Upload valid Image or Public Key\n");
                    return -2;
                }
                else if (ErrorCode == YAFU_CC_SIGNED_SUPP_NOT_ENABLED)
                {
                    printf("Signed Image Support is not enabled in Existing Flash !!!\n");
                    return -2;
                }
            }
            if (vdbg)
            {
                printf("\nWarning: EraseAndFlash - AMI_YAFU_COMMON_NAK\n");
                fflush(stdout);
            }
            RetryCount_NAK += 1;
            if (RetryCount_NAK <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount_NAK);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            RetryCount_NAK = 0;
        }

        if (CalculateChksum((char *)&pAMIYAFUEraseCopyFlashRes.Sizecopied, sizeof(INT32U)) != pAMIYAFUEraseCopyFlashRes.EraseCpyFlashRes.CRC32chksum)
        {
            if (vdbg)
            {
                printf("\nWarning: EraseAndFlash - Checksum error\n");
                fflush(stdout);
            }
            RetryCount_CSUM += 1;
            if (RetryCount_CSUM <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount_CSUM);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            // No errors, we can exit the while (1)
            break;
        }
    } // while (1)
    if (WriteMemOff > 0)
    {
        EFStart = 1;
    }
    return 0;
}
/*
 * @fn ECFStatus
 * @brief this function to get the status of the Flashing Firmware Process
 * @param hSession - Current Session Pointer
 * @return Returns 0 on success
 */
int ECFStatus(IPMI20_SESSION_T *hSession)
{

    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;
    int cpld_flash_status = 0;
    AMIYAFUGetECFStatusReq_T *pAMIYAFUGetECFStatusReq = NULL;
    AMIYAFUGetECFStatusRes_T pAMIYAFUGetECFStatusRes;
    pAMIYAFUGetECFStatusReq = (AMIYAFUGetECFStatusReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUGetECFStatusReq_T);
    ResLen = sizeof(AMIYAFUGetECFStatusRes_T);

    pAMIYAFUGetECFStatusReq->GetECFStatusReq.Seqnum = 0x00000001;
    pAMIYAFUGetECFStatusReq->GetECFStatusReq.YafuCmd = CMD_AMI_YAFU_GET_ECF_STATUS;
    pAMIYAFUGetECFStatusReq->GetECFStatusReq.CRC32chksum = 0x00;
    while (1)
    {
        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            int errVal;
            errVal = IPMICMD_AMIYAFUGetECFStatus(hSession, pAMIYAFUGetECFStatusReq, &pAMIYAFUGetECFStatusRes, RECIEVE_TIME_OUT);
            CompCode = pAMIYAFUGetECFStatusRes.CompletionCode;
#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_YAFU_GET_ECF_STATUS, ReqBuf, ReqLen);

            if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode))
            {
                SET_ERR_CODE(ERR_BMC_COMM);
                return -1;
            }
            memcpy(&pAMIYAFUGetECFStatusRes.GetECFStatusRes, ResBuf, ResLen);
        }
        if ((!DOMMCUpdate) && (!DONMMEUpdate))
        {
            if (EFStart == 0)
            {
                if (vdbg)
                {
                    printf("CompletionCode - %x\n ", CompCode);
                    printf("YafuCmd - %x\n ", pAMIYAFUGetECFStatusRes.GetECFStatusRes.YafuCmd);
                }
                if (((CompCode != 0) && (CompCode != YAFU_ERR_STATE)) || pAMIYAFUGetECFStatusRes.GetECFStatusRes.YafuCmd != CMD_AMI_YAFU_COMMON_NAK)
                {
                    return -1;
                }
                return 0;
            }

            if (pAMIYAFUGetECFStatusRes.Status == YAFU_CC_FLASH_SAME_IMAGE)
            {
                printf("\rCurrent Image and Existing Image are Same ... Not Proceeding Flashing Operation ...\n");
                return 2;
            }

            if ((pAMIYAFUGetECFStatusRes.Status != 0) && (pAMIYAFUGetECFStatusRes.Status != YAFU_ECF_SUCCESS))
            {
                if ((pAMIYAFUGetECFStatusRes.Status >= 0x02 && pAMIYAFUGetECFStatusRes.Status <= 0x06) || pAMIYAFUGetECFStatusRes.Status == 0x11)
                {
                    if (vdbg)
                        printf("Error in Flashing Firmware Image %x\n", pAMIYAFUGetECFStatusRes.Status);
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
            if (Progressdis == 1 && SilentFlash == 0x00)
            {
                if (ActualConfig == 1)
                {
                    printf("\rFlashing  Firmware Image : %d%%", (int)(ECFPercent + ((((pAMIYAFUGetECFStatusRes.Progress * ActualSizetoCopy) / 100) * 100) / ActualCopySize)));
                    if (pAMIYAFUGetECFStatusRes.Status == YAFU_ECF_SUCCESS)
                    {
                        if (ECFPercent > 0)
                        {
                            printf("\rFlashing  Firmware Image : %d%%... done\n", (int)(ECFPercent + ((((pAMIYAFUGetECFStatusRes.Progress * ActualSizetoCopy) / 100) * 100) / ActualCopySize)));
                        }
                        ECFPercent = (INT16U)((((pAMIYAFUGetECFStatusRes.Progress * ActualSizetoCopy) / 100) * 100) / ActualCopySize) + 1;
                        break;
                    }
                }
                else
                {
                    if (SPIDevice == CPLD_FLASH && ModuleType == 5)
                    {
                        if (cpld_flash_status == 0)
                        {
                            printf("\r                                      ");
                            printf("\rFlashing  Firmware Image : ");
                        }

                        if (cpld_flash_status != 7)
                        {
                            cpld_flash_status++;
                            printf(".");
                            fflush(stdout);
                            SleepMs(500);
                        }
                        else
                            cpld_flash_status = 0;
                    }
                    else
                        printf("\rFlashing  Firmware Image : %d%%", pAMIYAFUGetECFStatusRes.Progress);

                    if (pAMIYAFUGetECFStatusRes.Progress >= 100)
                    {
                        printf("\rFlashing  Firmware Image : %d%%... done", pAMIYAFUGetECFStatusRes.Progress);
                    }
                    if (pAMIYAFUGetECFStatusRes.Status == YAFU_ECF_SUCCESS)
                    {
                        printf("\rFlashing  Firmware Image : 100%%... done\n");
                        break;
                    }
                }
            }
            else
            {
                if (pAMIYAFUGetECFStatusRes.Status == YAFU_ECF_SUCCESS)
                {
                    break;
                }
            }
        }
        else
        {
            if (pAMIYAFUGetECFStatusRes.GetECFStatusRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
            {
                AMIYAFUNotAck pAMIYAFUNotAcknowledge;
                memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)&pAMIYAFUGetECFStatusRes, sizeof(AMIYAFUNotAck));
                if (pAMIYAFUNotAcknowledge.ErrorCode == YAFU_ERR_STATE)
                {
                    printf("Get ECF Status Failed\n");
                    return -1;
                }
            }

            printf("\rFlashing  Firmware Image : %d%%", pAMIYAFUGetECFStatusRes.Progress);
            if (pAMIYAFUGetECFStatusRes.Progress >= 100)
            {
                printf("\rFlashing  Firmware Image : %d%%... done\n", pAMIYAFUGetECFStatusRes.Progress);
                break;
            }
            //   sleep(1);
        }
    }
    return 0;
}
/*
 * @fn VerifyFlash
 * @brief This Fuction the Verify the writtened Flash area in Firmware
 * @param hSession - Current Session Pointer
 * @param MemOffset - Memory Offset of the RAM
 * @param FlashOffset - Flash Offset of the Flash Area in Firmware
 * @param Sizetoverify - Size of Flash Area to verify in Firmware
 * @return Returns 0 on success
 */
#if defined(__x86_64__) || defined(WIN64)
int VerifyFlash(IPMI20_SESSION_T *hSession, unsigned int MemOffset, unsigned int Flashoffset, unsigned int Sizetoverify)
#else
int VerifyFlash(IPMI20_SESSION_T *hSession, unsigned long MemOffset, unsigned long Flashoffset, unsigned long Sizetoverify)
#endif
{
    int RetryCount = 0;
    int RetryCount_NAK = 0;
    int RetryCount_CSUM = 0;
    int errVal = -1;
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;

    AMIYAFUVerifyFlashReq_T *pAMIYAFUVerifyFlashReq = NULL;
    AMIYAFUVerifyFlashRes_T pAMIYAFUVerfyFlashRes;
    pAMIYAFUVerifyFlashReq = (AMIYAFUVerifyFlashReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUVerifyFlashReq_T);
    ResLen = sizeof(AMIYAFUVerifyFlashRes_T);

    while (1)
    {
        pAMIYAFUVerifyFlashReq->VerifyFlashReq.Seqnum = 0x00000001;
        pAMIYAFUVerifyFlashReq->VerifyFlashReq.YafuCmd = CMD_AMI_YAFU_VERIFY_FLASH;
        pAMIYAFUVerifyFlashReq->VerifyFlashReq.Datalen = 0x0c;
        pAMIYAFUVerifyFlashReq->Memoffset = MemOffset;
        pAMIYAFUVerifyFlashReq->Flashoffset = Flashoffset;
        pAMIYAFUVerifyFlashReq->Sizetoverify = Sizetoverify;
        pAMIYAFUVerifyFlashReq->VerifyFlashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUVerifyFlashReq->Memoffset, (3 * sizeof(INT32U)));

        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIYAFUVerifyFlash(hSession, pAMIYAFUVerifyFlashReq, &pAMIYAFUVerfyFlashRes, RECIEVE_TIME_OUT);
#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_YAFU_VERIFY_FLASH, ReqBuf, ReqLen);
            errVal = SendTimedImbpRequest(&IfcReqHdr, 10000, ResBuf, &ResLen, &CompCode);
            memcpy(&pAMIYAFUVerfyFlashRes.VerifyFlashRes, ResBuf, ResLen);
        }
        if (errVal != ACCESN_OK)
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            if (vdbg)
            {
                printf("\nWarning: VerifyFlash failed %d \n", errVal);
                fflush(stdout);
            }
            RetryCount += 1;
            if (RetryCount <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            RetryCount = 0;
        }
        if (pAMIYAFUVerfyFlashRes.VerifyFlashRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
        {
            if (vdbg)
            {
                printf("\nWarning: VerifyFlash - AMI_YAFU_COMMON_NAK\n");
                fflush(stdout);
            }
            RetryCount_NAK += 1;
            if (RetryCount_NAK <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount_NAK);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            RetryCount_NAK = 0;
        }

        if (CalculateChksum((char *)&pAMIYAFUVerfyFlashRes.Offset, sizeof(INT32U)) != pAMIYAFUVerfyFlashRes.VerifyFlashRes.CRC32chksum)
        {
            if (vdbg)
            {
                printf("\nWarning: VerifyFlash - Checksum error\n");
                fflush(stdout);
            }
            RetryCount_CSUM += 1;
            if (RetryCount_CSUM <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount_CSUM);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            // No errors, we can exit the while (1)
            break;
        }
    }
    return 0;
}
/*
 * @fn VerifyStatus
 * @brief This Function get the status of the Verify Flash Progress
 * @param hsession_t - Curresnt Session Pointer
 * @return Returns 0 on success
 */
int VerifyStatus(IPMI20_SESSION_T *hsession_t)
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;

    AMIYAFUGetVerifyStatusReq_T *pAMIYAFUGetVerifyStatusReq = NULL;
    AMIYAFUGetVerifyStatusRes_T pAMIYAFUGetVerifyStatusRes;
    pAMIYAFUGetVerifyStatusReq = (AMIYAFUGetVerifyStatusReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUGetVerifyStatusReq_T);
    ResLen = sizeof(AMIYAFUGetVerifyStatusRes_T);
    pAMIYAFUGetVerifyStatusReq->GetVerifyStatusReq.Seqnum = 0x00000001;
    pAMIYAFUGetVerifyStatusReq->GetVerifyStatusReq.YafuCmd = CMD_AMI_YAFU_GET_VERIFY_STATUS;
    pAMIYAFUGetVerifyStatusReq->GetVerifyStatusReq.CRC32chksum = 0x00;

    while (1)
    {

        if (DOMMCUpdate)
        {
            int errVal;
            if (byMedium == KCS_MEDIUM)
            {
                FrameIfcReqHdr(CMD_AMI_YAFU_GET_VERIFY_STATUS, ReqBuf, ReqLen);
                if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode))
                {
                    SET_ERR_CODE(ERR_BMC_COMM);
                    return -1;
                }
                // pAMIYAFUGetVerifyStatusRes = (AMIYAFUGetVerifyStatusRes_T*)ResBuf;
                memcpy(&pAMIYAFUGetVerifyStatusRes.GetVerifyStatusRes, ResBuf, ResLen);

                printf("\rVerifying Firmware Image : %d%%", pAMIYAFUGetVerifyStatusRes.Progress);
                if (pAMIYAFUGetVerifyStatusRes.Progress >= 100)
                {
                    printf("\rVerifying Firmware Image : %d%%... done", pAMIYAFUGetVerifyStatusRes.Progress);
                    break;
                }
            }
            else
            {
#ifndef MSDOS
                errVal = IPMICMD_AMIYAFUGetVerifyStatus(hsession_t, pAMIYAFUGetVerifyStatusReq, &pAMIYAFUGetVerifyStatusRes, RECIEVE_TIME_OUT);
                printf("\rVerifying Firmware Image : %d%%", pAMIYAFUGetVerifyStatusRes.Progress);
                if (pAMIYAFUGetVerifyStatusRes.Progress >= 100)
                {
                    printf("\rVerifying Firmware Image : %d%%... done\n", pAMIYAFUGetVerifyStatusRes.Progress);
                    break;
                }
#endif
            }
            //             sleep(1);
        }
        else
        {
            if (byMedium != KCS_MEDIUM)
            {
#ifndef MSDOS
                int errVal;
                errVal = IPMICMD_AMIYAFUGetVerifyStatus(hsession_t, pAMIYAFUGetVerifyStatusReq, &pAMIYAFUGetVerifyStatusRes, RECIEVE_TIME_OUT);
#endif
            }
            else
            {
                FrameIfcReqHdr(CMD_AMI_YAFU_GET_VERIFY_STATUS, ReqBuf, ReqLen);
                if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode))
                {
                    SET_ERR_CODE(ERR_BMC_COMM);
                    return -1;
                }
                // pAMIYAFUGetVerifyStatusRes = (AMIYAFUGetVerifyStatusRes_T*)ResBuf;
                memcpy(&pAMIYAFUGetVerifyStatusRes.GetVerifyStatusRes, ResBuf, ResLen);
            }

            if ((pAMIYAFUGetVerifyStatusRes.Status != 0) && (pAMIYAFUGetVerifyStatusRes.Status != YAFU_VERIFY_SUCCESS))
            {
                if (pAMIYAFUGetVerifyStatusRes.Status >= 0x02 && pAMIYAFUGetVerifyStatusRes.Status <= 0x06)
                {
                    if (vdbg)
                        printf("Verify failed %x\n", pAMIYAFUGetVerifyStatusRes.Status);
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
            if (Progressdis == 1 && SilentFlash == 0x00)
            {
                if (ActualConfig == 1)
                {
                    printf("\rVerifying Firmware Image : %d%%", (int)(VerifyPercent + ((((pAMIYAFUGetVerifyStatusRes.Progress * ActualSizetoCopy) / 100) * 100) / ActualCopySize)));
                    if (pAMIYAFUGetVerifyStatusRes.Status == YAFU_VERIFY_SUCCESS)
                    {
                        if (VerifyPercent > 0)
                        {
                            printf("\rVerifying Firmware Image : %d%%... done\n", (int)(VerifyPercent + ((((pAMIYAFUGetVerifyStatusRes.Progress * ActualSizetoCopy) / 100) * 100) / ActualCopySize)));
                        }
                        VerifyPercent = (INT16U)((((pAMIYAFUGetVerifyStatusRes.Progress * ActualSizetoCopy) / 100) * 100) / ActualCopySize) + 1;
                        break;
                    }
                }
                else
                {
                    printf("\rVerifying Firmware Image : %d%%", pAMIYAFUGetVerifyStatusRes.Progress);
                    if (pAMIYAFUGetVerifyStatusRes.Progress >= 100)
                    {
                        printf("\rVerifying Firmware Image : %d%%... done", pAMIYAFUGetVerifyStatusRes.Progress);
                    }
                    if (pAMIYAFUGetVerifyStatusRes.Status == YAFU_VERIFY_SUCCESS)
                    {
                        printf("\rVerifying Firmware Image : 100%%... done\n");
                        break;
                    }
                }
            }
            else
            {
                if (pAMIYAFUGetVerifyStatusRes.Status == YAFU_VERIFY_SUCCESS)
                {
                    break;
                }
            }
        }
    }
    if (CalculateChksum((char *)&pAMIYAFUGetVerifyStatusRes.Offset, sizeof(INT32U)) != pAMIYAFUGetVerifyStatusRes.GetVerifyStatusRes.CRC32chksum)
    {
        return -1;
    }
    return 0;
}
/*
 * @fn DeactivateFlshMode
 * @brief this fuction Deactivate Flash mode in Firmware
 * @param hSession - Current Session Pointer
 * @return Returns 0 on success
 */
int DeactivateFlshMode(IPMI20_SESSION_T *hSession)
{

    static BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;
    AMIYAFUNotAck pAMIYAFUNotAcknowledge;
#ifndef MSDOS
    int errVal;
#endif

    AMIYAFUDeactivateFlashReq_T *pAMIYAFUDeactivateFlashReq = NULL;
    AMIYAFUDeactivateFlashRes_T pAMIYAFUDeactivateFlashRes;
    pAMIYAFUDeactivateFlashReq = (AMIYAFUDeactivateFlashReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUDeactivateFlashReq_T);
    ResLen = sizeof(AMIYAFUDeactivateFlashRes_T);

    pAMIYAFUDeactivateFlashReq->DeactivateFlashReq.Seqnum = 0x00000001;
    pAMIYAFUDeactivateFlashReq->DeactivateFlashReq.YafuCmd = CMD_AMI_YAFU_DEACTIVATE_FLASH_MODE;
    pAMIYAFUDeactivateFlashReq->DeactivateFlashReq.Datalen = 0x00;
    pAMIYAFUDeactivateFlashReq->DeactivateFlashReq.CRC32chksum = 0x00;

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIYAFUDeactivateFlash(hSession, pAMIYAFUDeactivateFlashReq, &pAMIYAFUDeactivateFlashRes, RECIEVE_TIME_OUT);
        if (errVal != 0)
        {
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
            return -1;
        }
#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_DEACTIVATE_FLASH_MODE, ReqBuf, ReqLen);

        if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 2000, ResBuf, &ResLen, &CompCode))
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }
        if (ResLen != sizeof(AMIYAFUDeactivateFlashRes_T) - 1)
        {
            SET_ERR_CODE(ERR_RES_LEN);
            return -1;
        }
        // pAMIYAFUDeactivateFlashRes = (AMIYAFUDeactivateFlashRes_T*)ResBuf;
        memcpy(&pAMIYAFUDeactivateFlashRes.DeactivateFlashRes, ResBuf, ResLen);
    }

    if (pAMIYAFUDeactivateFlashRes.DeactivateFlashRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
    {

        memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)&pAMIYAFUDeactivateFlashRes, sizeof(AMIYAFUNotAck));
        return -1;
    }

    if (CalculateChksum((char *)&pAMIYAFUDeactivateFlashRes.Status, sizeof(INT8U)) != pAMIYAFUDeactivateFlashRes.DeactivateFlashRes.CRC32chksum)
    {
        return -1;
    }
    return 0;
}
/*
 * @fn ResetDevice
 * @brief this function Reset the Firmware
 * @param hSession - Current Session Pointer
 * @param WaitTime - Time to wait before restart in Firmware
 * @return Returns 0 on success
 */
int ResetDevice(IPMI20_SESSION_T *hSession, INT16U WaitTime)
{

    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[TEMP_MAX_REQ_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;

    AMIYAFUResetDeviceReq_T *pAMIYAFUResetDeviceReq = NULL;
    AMIYAFUResetDeviceRes_T pAMIYAFUResetDeviceRes;
    pAMIYAFUResetDeviceReq = (AMIYAFUResetDeviceReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUResetDeviceReq_T);
    ResLen = sizeof(AMIYAFUResetDeviceRes_T);

    pAMIYAFUResetDeviceReq->ResetReq.Seqnum = 0x00000001;
    pAMIYAFUResetDeviceReq->ResetReq.YafuCmd = CMD_AMI_YAFU_RESET_DEVICE;
    pAMIYAFUResetDeviceReq->ResetReq.Datalen = 0x02;
    pAMIYAFUResetDeviceReq->WaitSec = WaitTime;
    pAMIYAFUResetDeviceReq->ResetReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUResetDeviceReq->WaitSec, sizeof(INT16U));
    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        int errVal;
        errVal = IPMICMD_AMIYAFUResetDevice(hSession, pAMIYAFUResetDeviceReq, &pAMIYAFUResetDeviceRes, 20);
        if (errVal != 0)
            return -1;
#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_RESET_DEVICE, ReqBuf, ReqLen);

        if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 2000, ResBuf, &ResLen, &CompCode))
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }
        if (ResLen != sizeof(AMIYAFUResetDeviceRes_T))
        {
            SET_ERR_CODE(ERR_RES_LEN);
            return -1;
        }
        // pAMIYAFUResetDeviceRes = (AMIYAFUResetDeviceRes_T*)ResBuf;
        memcpy(&pAMIYAFUResetDeviceRes.ResetRes, ResBuf, ResLen);
    }

    if (pAMIYAFUResetDeviceRes.ResetRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
        return -1;

    if (CalculateChksum((char *)&pAMIYAFUResetDeviceRes.Status, sizeof(INT8U)) != pAMIYAFUResetDeviceRes.ResetRes.CRC32chksum)
        return -1;
    return 0;
}
/*
 * @fn protectFlash
 * @brief This Function Protect Off the Block in Firmware
 * @param hSession - Current Session Pointer
 * @param Blknum - Block Number of the Firmware
 * @param Protect - Lock or Unlock the Block in the Firmware
 * @return Returns 0 on success
 */
int protectFlash(IPMI20_SESSION_T *hSession, INT32U Blknum, INT8U Protect)
{
    static BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    static BYTE ResBuf[TEMP_MAX_REQ_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;
#ifndef MSDOS
    int errVal;
#endif

    AMIYAFUProtectFlashReq_T *pAMIYAFUProtectFlashReq = NULL;
    AMIYAFUProtectFlashRes_T pAMIYAFUProtectFlashRes;
    AMIYAFUNotAck pAMIYAFUNotAcknowledge;
    pAMIYAFUProtectFlashReq = (AMIYAFUProtectFlashReq_T *)&ReqBuf;
    // pAMIYAFUProtectFlashRes = ( AMIYAFUProtectFlashRes_T * )&ResBuf;

    ReqLen = sizeof(AMIYAFUProtectFlashReq_T);
    ResLen = sizeof(AMIYAFUProtectFlashRes_T);

    pAMIYAFUProtectFlashReq->ProtectFlashReq.Seqnum = 0x00000001;
    pAMIYAFUProtectFlashReq->ProtectFlashReq.YafuCmd = CMD_AMI_YAFU_PROTECT_FLASH;
    pAMIYAFUProtectFlashReq->ProtectFlashReq.Datalen = 0x05;
    pAMIYAFUProtectFlashReq->Blknum = Blknum;
    pAMIYAFUProtectFlashReq->Protect = Protect;
    pAMIYAFUProtectFlashReq->ProtectFlashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUProtectFlashReq->Blknum, (sizeof(INT32U) + sizeof(INT8U)));

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIYAFUProtectFlash(hSession, pAMIYAFUProtectFlashReq, &pAMIYAFUProtectFlashRes, RECIEVE_TIME_OUT);
        if (errVal != 0)
        {
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
            return -1;
        }
#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_PROTECT_FLASH, ReqBuf, ReqLen);

        if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 10000, ResBuf, &ResLen, &CompCode))
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }
        memcpy(&pAMIYAFUProtectFlashRes.ProtectFlashRes, ResBuf, ResLen);//ResLen = 0x0e������
    }
    if (pAMIYAFUProtectFlashRes.ProtectFlashRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
    {
        printf("Protect Flash Failed\n");
        memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)&pAMIYAFUProtectFlashRes, sizeof(AMIYAFUNotAck));
        return -1;
    }

    if (CalculateChksum((char *)&pAMIYAFUProtectFlashRes.Status, sizeof(INT8U)) != pAMIYAFUProtectFlashRes.ProtectFlashRes.CRC32chksum)
        return -1;

    return 0;
}
/*
 * @fn FreeMemory
 * @brief This function is to Free the Allocated memory
 * @param hSession - Current Session Pointer
 * @param WriteMemOff - Allocated Memory to Free
 * @return Returns 0 on success
 */
int FreeMemory(IPMI20_SESSION_T *hSession, INT32U WriteMemOff)
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[TEMP_MAX_REQ_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;

    AMIYAFUFreeMemoryReq_T *pAMIYAFUFreeMemoryReq = NULL;
    AMIYAFUFreeMemoryRes_T pAMIYAFUFreeMemoryRes = {0};
    pAMIYAFUFreeMemoryReq = (AMIYAFUFreeMemoryReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUFreeMemoryReq_T);
    ResLen = sizeof(AMIYAFUFreeMemoryRes_T);

    pAMIYAFUFreeMemoryReq->FreememReq.Seqnum = 0x00000001;
    pAMIYAFUFreeMemoryReq->FreememReq.YafuCmd = CMD_AMI_YAFU_FREE_MEMORY;
    pAMIYAFUFreeMemoryReq->FreememReq.Datalen = 0x4;
    pAMIYAFUFreeMemoryReq->AddrtobeFreed = WriteMemOff;
    pAMIYAFUFreeMemoryReq->FreememReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUFreeMemoryReq->AddrtobeFreed, sizeof(INT32U));

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        int errVal;
        errVal = IPMICMD_AMIYAFUFreeMemory(hSession, pAMIYAFUFreeMemoryReq, &pAMIYAFUFreeMemoryRes, RECIEVE_TIME_OUT);
        if (errVal != 0)
            return -1;
#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_FREE_MEMORY, ReqBuf, ReqLen);

        if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 10000, ResBuf, &ResLen, &CompCode))
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }
        memcpy(&pAMIYAFUFreeMemoryRes.FreememRes, ResBuf, ResLen);
    }
    if (pAMIYAFUFreeMemoryRes.FreememRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
    {
        AMIYAFUNotAck pAMIYAFUNotAcknowledge;
        memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)&pAMIYAFUFreeMemoryRes, sizeof(AMIYAFUNotAck));
        return -1;
    }

    if (CalculateChksum((char *)&pAMIYAFUFreeMemoryRes.Status, sizeof(INT8U)) != pAMIYAFUFreeMemoryRes.FreememRes.CRC32chksum)
        return -1;

    return 0;
}
/*
 * @fn FlashModHeadInfo
 * @brief This function is invoked to get Flash Mode Header Info
 * @param hSession - Current Session Pointer
 * @param FMHRes - Response Pointer of FHMInfo(AMIYAFUGetFMHInfoRes_T)
 * @return Returns 0 on success
 */
int FlashModHeadInfo(IPMI20_SESSION_T *hSession, AMIYAFUGetFMHInfoRes_T *FMHRes)
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;
    AMIYAFUGetFMHInfoRes_T FlashMHRes;
    AMIYAFUGetFMHInfoReq_T *pAMIYAFUGetFMHInfoReq = NULL;
    pAMIYAFUGetFMHInfoReq = (AMIYAFUGetFMHInfoReq_T *)ReqBuf;

    ReqLen = sizeof(AMIYAFUGetFMHInfoReq_T);
    ResLen = MAX_FMHLENGTH;

    pAMIYAFUGetFMHInfoReq->FMHReq.Seqnum = 0x00000001;
    pAMIYAFUGetFMHInfoReq->FMHReq.YafuCmd = CMD_AMI_YAFU_GET_FMH_INFO;
    pAMIYAFUGetFMHInfoReq->FMHReq.Datalen = 0x00;
    pAMIYAFUGetFMHInfoReq->FMHReq.CRC32chksum = 0x00;

    if (byMedium == KCS_MEDIUM)
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_GET_FMH_INFO, ReqBuf, ReqLen);

        if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 13000, ResBuf, &ResLen, &CompCode))
        {
            printf("Error in SendTimedImbpRequest \n");
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }
        memcpy((char *)&FlashMHRes.FMHRes, (char *)&ResBuf[0], YAFU_HEADER_SIZE);
        ResLen = FlashMHRes.FMHRes.Datalen;
        memcpy((char *)&FMHRes->FMHRes, (char *)&ResBuf[0], (ResLen + YAFU_HEADER_SIZE));
    }

    else
    {
#ifndef MSDOS
        int errVal;
        errVal = IPMICMD_AMIYAFUGetFMHInfo(hSession, pAMIYAFUGetFMHInfoReq, (AMIYAFUGetFMHInfoRes_T *)FMHRes, RECIEVE_TIME_OUT);
        if (errVal != 0)
            return -1;
#endif
    }

    if (FMHRes->FMHRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
    {
        AMIYAFUNotAck pAMIYAFUNotAcknowledge;
        memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)FMHRes, sizeof(AMIYAFUNotAck));
        return -1;
    }

    if (CalculateChksum((char *)&FMHRes->Reserved, FMHRes->FMHRes.Datalen) != FMHRes->FMHRes.CRC32chksum)
        return -1;

    return 0;
}
/*
 * @fn GetStatus
 * @brief This function is invoked to get Status of the SP
 * @param hSession - Current Session Pointer
 * @return Returns 0 on success
 */
int GetStatus(IPMI20_SESSION_T *hSession)
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;
    AMIYAFUGetStatusReq_T *pAMIYAFUGetStatusReq = NULL;
    AMIYAFUGetStatusRes_T pAMIYAFUGetStatusRes;
#ifndef MSDOS
    int errVal;
#endif
    pAMIYAFUGetStatusReq = (AMIYAFUGetStatusReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUGetStatusReq_T);
    ResLen = sizeof(AMIYAFUGetStatusRes_T);

    pAMIYAFUGetStatusReq->GetStatusReq.Seqnum = 0x00000001;
    pAMIYAFUGetStatusReq->GetStatusReq.YafuCmd = CMD_AMI_YAFU_GET_STATUS;
    pAMIYAFUGetStatusReq->GetStatusReq.Datalen = 0x00;
    pAMIYAFUGetStatusReq->GetStatusReq.CRC32chksum = 0x00;

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIYAFUGetStatus(hSession, pAMIYAFUGetStatusReq, &pAMIYAFUGetStatusRes, RECIEVE_TIME_OUT);
        if (errVal != 0)
        {
            if (pAMIYAFUGetStatusRes.CompletionCode == YAFU_CC_FLASHER_NOT_READY)
            {
                printf("\n Flasher Not Ready ! Please Try After Sometime  \n");
                Close_Session(hSession);
            }

            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
            return -1;
        }
#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_GET_STATUS, ReqBuf, ReqLen);

        if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 2000, ResBuf, &ResLen, &CompCode))
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }
        memcpy(&pAMIYAFUGetStatusRes.GetStatusRes, ResBuf, ResLen);
        if (CompCode == YAFU_CC_FLASHER_NOT_READY)
        {
            printf("\n Flasher Not Ready ! Please Try After Sometime  \n");
            return -1;
        }
    }

    if (pAMIYAFUGetStatusRes.GetStatusRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
    {
        AMIYAFUNotAck pAMIYAFUNotAcknowledge;
        memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)&pAMIYAFUGetStatusRes, sizeof(AMIYAFUNotAck));
        return -1;
    }

    if (CalculateChksum((char *)&pAMIYAFUGetStatusRes.LastStatusCode, (INT32U)pAMIYAFUGetStatusRes.GetStatusRes.Datalen) != pAMIYAFUGetStatusRes.GetStatusRes.CRC32chksum)
    {
        printf("check sum failed\n");
        return -1;
    }

    RecoveryFlashMode = pAMIYAFUGetStatusRes.Mode;
    return 0;
}
/*
 * @fn GetBootConfig
 * @brief This function is invoked to get Boot Variable Values
 * @param hSession - Current Session Pointer
 * @param BootVariables - Char Pointer of Boot Variable Names
 * @param BootVar - Char Pointer of Boot Variable Values
 * @return Returns 0 on success
 */
int GetBootConfig(IPMI20_SESSION_T *hSession, char *BootVariables, char *BootVar)
{

    int RetryCount_NAK = 0;
    int RetryCount_CSUM = 0;
    int RetryCount = 0;
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE] = {0};
    int ResLen, ReqLen;
    BYTE CompCode;
    int errVal = -1;

    AMIYAFUGetBootConfigReq_T *pAMIYAFUGetBootConfigReq = NULL;
    AMIYAFUGetBootConfigRes_T *pAMIYAFUGetBootConfigRes = NULL;
    pAMIYAFUGetBootConfigReq = (AMIYAFUGetBootConfigReq_T *)&ReqBuf;
    pAMIYAFUGetBootConfigRes = (AMIYAFUGetBootConfigRes_T *)&ResBuf;
    ReqLen = sizeof(AMIYAFUGetBootConfigReq_T);
    ResLen = MAX_GETBOOTVAL;

    while (1)
    {
        memset((char *)pAMIYAFUGetBootConfigReq, 0, sizeof(AMIYAFUGetBootConfigReq_T));

        pAMIYAFUGetBootConfigReq->GetBootReq.Seqnum = 0x00000001;
        pAMIYAFUGetBootConfigReq->GetBootReq.YafuCmd = CMD_AMI_YAFU_GET_BOOT_CONFIG;
        pAMIYAFUGetBootConfigReq->GetBootReq.Datalen = sizeof(pAMIYAFUGetBootConfigReq->VarName);
        strcpy(pAMIYAFUGetBootConfigReq->VarName, BootVar);
        pAMIYAFUGetBootConfigReq->GetBootReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUGetBootConfigReq->VarName[0], pAMIYAFUGetBootConfigReq->GetBootReq.Datalen);
        pAMIYAFUGetBootConfigRes = (AMIYAFUGetBootConfigRes_T *)BootVariables;

        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIYAFUGetBootConfig(hSession, pAMIYAFUGetBootConfigReq, (AMIYAFUGetBootConfigRes_T *)BootVariables, RECIEVE_TIME_OUT); //&pAMIYAFUGetBootConfigRes

#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_YAFU_GET_BOOT_CONFIG, ReqBuf, ReqLen);

            errVal = SendTimedImbpRequest(&IfcReqHdr, 4000, ResBuf, &ResLen, &CompCode);
            BootVariables[0] = CompCode;
            memcpy(&BootVariables[1], (char *)&ResBuf[0], (pAMIYAFUGetBootConfigRes->GetBootRes.Datalen + sizeof(AMIYAFUGetBootConfigRes_T)));
        }

        if (errVal != ACCESN_OK)
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            if (vdbg)
            {
                printf("\nWarning: GetBootConfig failed %d \n", errVal);
                fflush(stdout);
            }
            RetryCount += 1;
            if (RetryCount <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            RetryCount = 0;
        }

        if (pAMIYAFUGetBootConfigRes->GetBootRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
        {
            if (vdbg)
            {
                printf("\nWarning: GetBootConfig - AMI_YAFU_COMMON_NAK\n");
                fflush(stdout);
            }
            RetryCount_NAK += 1;
            if (RetryCount_NAK <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount_NAK);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            RetryCount_NAK = 0;
        }

        if (CalculateChksum((char *)&pAMIYAFUGetBootConfigRes->Status, (INT32U)pAMIYAFUGetBootConfigRes->GetBootRes.Datalen) != pAMIYAFUGetBootConfigRes->GetBootRes.CRC32chksum)
        {
            RetryCount_CSUM += 1;
            if (RetryCount_CSUM <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Warning: Checksum failure GetBootConfig.  Attempting Retry #%d\n", RetryCount_CSUM);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Checksum failure GetBootConfig.  Aborting operation\n");
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            // No error, break out of the while (1) loop
            break;
        }
    } // while 1
    return 0;
}
/*
 * @fn SetBootConfig
 * @brief This function is invoked to set the values of Boot variables
 * @param hsession - Current Session Pointer
 * @param BootVar - Holds Boot Variable Names to be set
 * @param BootVal - Holds Boot Variable Values to be set
 * @return Returns 0 on success
 */
int SetBootConfig(IPMI20_SESSION_T *hSession, char *BootVar, char *BootVal)
{

    int RetryCount_NAK = 0;
    int RetryCount_CSUM = 0;
    int RetryCount = 0;
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, len = 0;
    BYTE CompCode;
    int errVal = -1;

    AMIYAFUSetBootConfigReq_T *pAMIYAFUSetBootConfigReq = NULL;
    AMIYAFUSetBootConfigRes_T pAMIYAFUSetBootConfigRes;

#if defined(__x86_64__) || defined(WIN64)
    unsigned int ReqLen = 0;
#else
    unsigned long ReqLen = 0;
#endif
    char *TempBuff = NULL;
    char tbArr[MAX_BOOTVAL_LENGTH];
    len = strlen(BootVal);
    pAMIYAFUSetBootConfigReq = (AMIYAFUSetBootConfigReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUSetBootConfigReq_T) + len + 1;
    ResLen = sizeof(AMIYAFUSetBootConfigRes_T);

    while (1)
    {
        TempBuff = tbArr;
        memset((char *)&pAMIYAFUSetBootConfigRes, 0, sizeof(AMIYAFUSetBootConfigRes_T));
        memset(TempBuff, 0, MAX_BOOTVAL_LENGTH);
        pAMIYAFUSetBootConfigReq = (AMIYAFUSetBootConfigReq_T *)TempBuff;

        pAMIYAFUSetBootConfigReq->SetBootReq.Seqnum = 0x00000002;
        pAMIYAFUSetBootConfigReq->SetBootReq.YafuCmd = CMD_AMI_YAFU_SET_BOOT_CONFIG;
        pAMIYAFUSetBootConfigReq->SetBootReq.Datalen = sizeof(pAMIYAFUSetBootConfigReq->VarName) + len + 1;
        strcpy(pAMIYAFUSetBootConfigReq->VarName, BootVar);

        TempBuff += sizeof(AMIYAFUSetBootConfigReq_T);
        memcpy(TempBuff, BootVal, (len + 1));
        TempBuff -= sizeof(AMIYAFUSetBootConfigReq_T);
        pAMIYAFUSetBootConfigReq->SetBootReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUSetBootConfigReq->VarName, pAMIYAFUSetBootConfigReq->SetBootReq.Datalen);

        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIYAFUSetBootConfig(hSession, (AMIYAFUSetBootConfigReq_T *)TempBuff, &pAMIYAFUSetBootConfigRes, RECIEVE_TIME_OUT, ReqLen);
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_YAFU_SET_BOOT_CONFIG, TempBuff, ReqLen);
            errVal = SendTimedImbpRequest(&IfcReqHdr, 4000, ResBuf, &ResLen, &CompCode);
            memcpy(&pAMIYAFUSetBootConfigRes.SetBootRes, ResBuf, ResLen);
        }
        if (errVal != ACCESN_OK)
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            if (vdbg)
            {
                printf("\nWarning: SetBootConfig failed %d \n", errVal);
                fflush(stdout);
            }
            RetryCount += 1;
            if (RetryCount <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            RetryCount = 0;
        }

        if (pAMIYAFUSetBootConfigRes.SetBootRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
        {
            if (vdbg)
            {
                printf("\nWarning: SetBootConfig - AMI_YAFU_COMMON_NAK\n");
                fflush(stdout);
            }
            RetryCount_NAK += 1;
            if (RetryCount_NAK <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount_NAK);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            RetryCount_NAK = 0;
        }

        if (CalculateChksum((char *)&pAMIYAFUSetBootConfigRes.Status, (INT32U)pAMIYAFUSetBootConfigRes.SetBootRes.Datalen) != pAMIYAFUSetBootConfigRes.SetBootRes.CRC32chksum)
        {
            RetryCount_CSUM += 1;
            if (RetryCount_CSUM <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Warning: Checksum failure SetBootConfig.  Attempting Retry #%d\n", RetryCount_CSUM);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Checksum failure SetBootConfig.  Aborting operation\n");
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            // No error, break out of the while (1) loop
            break;
        }
    } // while 1

    return 0;
}
/*
 * @fn GetAllBootVars
 * @brief This function is invoked to get All Boot Variables
 * @param hSession - Current Session Pointer
 * @param BootVars - Boot Variable Names char pointer
 * @param BootVarsCount  - Boot Variables Count Pointer
 * @return Returns 0 on success
 */
int GetAllBootVars(IPMI20_SESSION_T *hSession, unsigned char *BootVars, INT16U *BootVarsCount)
{

    int RetryCount_NAK = 0;
    int RetryCount_CSUM = 0;
    int RetryCount = 0;
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE] = {0};
    BYTE TempBuf[MAX_RES_SIZE] = {0};
    int ResLen, ReqLen;
    BYTE CompCode;
    AMIYAFUGetBootVarsReq_T *pAMIYAFUGetBootVarsReq = NULL;
    AMIYAFUGetBootVarsRes_T *pAMIYAFUGetBootVarsRes;
    int errVal = -1;

    pAMIYAFUGetBootVarsReq = (AMIYAFUGetBootVarsReq_T *)ReqBuf;
    pAMIYAFUGetBootVarsRes = (AMIYAFUGetBootVarsRes_T *)ResBuf;

    ReqLen = sizeof(AMIYAFUGetBootVarsReq_T);
    ResLen = MAX_GETBOOTVAR;

    while (1)
    {
        pAMIYAFUGetBootVarsReq->GetBootReq.Seqnum = 0x00000001;
        pAMIYAFUGetBootVarsReq->GetBootReq.YafuCmd = CMD_AMI_YAFU_GET_BOOT_VARS;
        pAMIYAFUGetBootVarsReq->GetBootReq.Datalen = 0x00;
        pAMIYAFUGetBootVarsReq->GetBootReq.CRC32chksum = 0x00;

        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIYAFUGetAllBootVars(hSession, pAMIYAFUGetBootVarsReq, (AMIYAFUGetBootVarsRes_T *)ResBuf, RECIEVE_TIME_OUT);
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }

#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_YAFU_GET_BOOT_VARS, ReqBuf, ReqLen);
            errVal = SendTimedImbpRequest(&IfcReqHdr, 4000, TempBuf, &ResLen, &CompCode);
            ResBuf[0] = CompCode;
            memcpy(&ResBuf[1], &TempBuf[0], ResLen);
        }
        if (errVal != ACCESN_OK)
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            if (vdbg)
            {
                printf("\nWarning: GetAllBootVars failed %d \n", errVal);
                fflush(stdout);
            }
            RetryCount += 1;
            if (RetryCount <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            RetryCount = 0;
        }
        if (pAMIYAFUGetBootVarsRes->GetBootRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
        {

            if (vdbg)
            {
                printf("\nWarning: GetAllBootVars - AMI_YAFU_COMMON_NAK\n");
                fflush(stdout);
            }
            RetryCount_NAK += 1;
            if (RetryCount_NAK <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Retry #%d\n", RetryCount_NAK);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Retry Count exceeded. Aborting, line = %d\n", __LINE__);
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            RetryCount_NAK = 0;
        }

        if (CalculateChksum((char *)&pAMIYAFUGetBootVarsRes->VarCount, (INT32U)pAMIYAFUGetBootVarsRes->GetBootRes.Datalen) != pAMIYAFUGetBootVarsRes->GetBootRes.CRC32chksum)
        {
            RetryCount_CSUM += 1;
            if (RetryCount_CSUM <= RETRYCOUNT)
            {
                if (vdbg)
                {
                    printf("Warning: Checksum failure GetAllBootVars.  Attempting Retry #%d\n", RetryCount_CSUM);
                    fflush(stdout);
                }
                continue;
            }
            else
            {
                if (vdbg)
                {
                    printf("Error: Checksum failure GetAllBootVars.  Aborting operation\n");
                    fflush(stdout);
                }
                return -1;
            }
        }
        else
        {
            memcpy(BootVars, (char *)&ResBuf[0], (pAMIYAFUGetBootVarsRes->GetBootRes.Datalen + sizeof(AMIYAFUGetBootVarsRes_T)));
            *BootVarsCount = (INT16U)pAMIYAFUGetBootVarsRes->VarCount;
            break;
        }
    } // while (1)
    return 0;
}

/*
 *@fn GetAllPreserveConfStatus
 *@brief Helps in setting Dual Image Configurations
 *@param hSession - Current Session Pointer
 *@param status - To get the status of all PreserveConf
 *@param enabledstatus - To get the configurable status of all PreserveConf
 *@return Returns 0 on success
 *        Returns proper error code on failure
 */

int GetPreserveConfStatus(IPMI20_SESSION_T *hSession, unsigned short selector)
{
#ifndef MSDOS
    int errVal = 0;
#endif
    BYTE ResBuf[TEMP_MAX_REQ_SIZE] = {0};
    int ResLen, retry;
    BYTE CompCode;
    GetPreserveConfigReq_T pGetPreserveConfigReq;
    GetPreserveConfigRes_T pGetPreserveConfigRes = {0, 0};

    pGetPreserveConfigReq.Selector = selector;
    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIGetpreserveConfStatus(hSession, &pGetPreserveConfigReq, &pGetPreserveConfigRes, RECIEVE_TIME_OUT);
        CompCode = pGetPreserveConfigRes.CompletionCode;
        if (errVal != 0)
        {
            printf("The return value is %x \n", errVal);
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
            return errVal;
        }
#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_GET_PRESERVE_CONF, NULL, 0);

        for (retry = 0; retry <= 3; retry++)
        {

            if (retry == 3)
            {
                SET_ERR_CODE(ERR_BMC_COMM);
                return -1;
            }
            else
            {
                if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode))
                    SleepMs(8000);
                else
                    break;
            }
        }
    }
    if (0 != CompCode)
    {
        if (0XC1 == CompCode)
        {
            return -1;
        }
        return CompCode;
    }
    return 0;
}

int GetAllPreserveConfStatus(IPMI20_SESSION_T *hSession, unsigned short *status, unsigned short *enabledstatus)
{
#ifndef MSDOS
    int errVal = 0;
#endif
    BYTE ResBuf[TEMP_MAX_REQ_SIZE] = {0};
    int ResLen, retry;
    BYTE CompCode;

    GetAllPreserveConfigRes_T pGetAllPreserveConfigRes;

    ResLen = sizeof(GetAllPreserveConfigRes_T);

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIGetAllPreserveConfStatus(hSession, &pGetAllPreserveConfigRes, 0, RECIEVE_TIME_OUT);

        CompCode = pGetAllPreserveConfigRes.CompletionCode;

        if (errVal != 0)
        {
            printf("The return value is %x \n", errVal);
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
            return errVal;
        }

#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_GET_ALL_PRESERVE_CONF, NULL, 0);

        for (retry = 0; retry <= 3; retry++)
        {

            if (retry == 3)
            {
                SET_ERR_CODE(ERR_BMC_COMM);
                return -1;
            }
            else
            {
                if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode))
                    SleepMs(8000);
                else
                    break;
            }
        }
        memcpy(&pGetAllPreserveConfigRes.Reserved, ResBuf, ResLen);
    }
    if (0 != CompCode)
    {
        if (0XC1 == CompCode)
        {
            return -1;
        }
        return CompCode;
    }
    *status = pGetAllPreserveConfigRes.Status;
    *enabledstatus = pGetAllPreserveConfigRes.EnabledStatus;
    return 0;
}

/*
 *@fn SetAllPreserveConfStatus
 *@brief Helps in setting Preserve Configuration Setup
 *@param hSession - Current Session Pointer
 *@param Status -   Status Value for AMISetAllPreserveConfStatus command
 *@return Returns 0 on success
 *        Returns proper error code on failure
 */
int SetAllPreserveConfStatus(IPMI20_SESSION_T *hSession, unsigned short Status)
{
#ifndef MSDOS
    int errVal = 0;
    SetAllPreserveConfigRes_T pSetAllPreserveConfigRes;
#endif
    INT8U ReqLen = 0;

    BYTE ReqBuf[TEMP_MAX_REQ_SIZE] = {0};
    BYTE ResBuf[MAX_RES_SIZE] = {0};
    int ResLen = 0, retry;
    BYTE CompCode = 0;

    SetAllPreserveConfigReq_T *pSetAllPreserveConfigReq;
    pSetAllPreserveConfigReq = (SetAllPreserveConfigReq_T *)ReqBuf;
    ReqLen = sizeof(SetAllPreserveConfigReq_T);

    pSetAllPreserveConfigReq->Status = Status;

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        errVal = IPMICMD_AMISetAllPreserveConfStatus(hSession, pSetAllPreserveConfigReq, &pSetAllPreserveConfigRes, ReqLen, RECIEVE_TIME_OUT);

        if (errVal != 0)
        {
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
            return errVal;
        }

#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_SET_ALL_PRESERVE_CONF, ReqBuf, ReqLen);

        for (retry = 0; retry <= 3; retry++)
        {

            if (retry == 3)
            {
                SET_ERR_CODE(ERR_BMC_COMM);
                return -1;
            }
            else
            {
                if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode))
                    SleepMs(8000);
                else
                    break;
            }
        }
    }
    if (0 != CompCode)
    {
        if (0XC1 == CompCode)
        {
            return -1;
        }
        return CompCode;
    }
    return 0;
}
int OnEnableUSBDevice()
{

    BYTE ReqBuf[TEMP_MAX_REQ_SIZE] = {0};
    int ResLen, ReqLen;
#ifndef MSDOS
    int retry;
    BYTE CompCode = -1;
    BYTE ResBuf[TEMP_MAX_REQ_SIZE] = {0};
#endif
    ResLen = sizeof(AMIVirtualDeviceSetStatusRes_T);
    ReqLen = sizeof(AMIVirtualDeviceSetStatusReq_T);
    ReqBuf[0] = 0x0; // Disable the power save mode command value;

#ifdef WINDOWS

    if (FLASH_LoadDriver() != 1)
    {
        printf("Failed\n");
        exit(1);
    }

#endif
#ifdef ICC_OS_LINUX
    CheckAndUnloadIPMIDriver();
#endif

    SetBMCPorts((WORD)0xCA2, (WORD)0xCA3);
    OpenIfc();

#ifndef MSDOS
    FrameIfcReqHdr(CMD_AMI_VIRTUAL_DEVICE_SET_STATUS, ReqBuf, ReqLen);
    if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 1000, ResBuf, &ResLen, &CompCode))
    {
        SET_ERR_CODE(ERR_BMC_COMM);
#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif
        return -1;
    }

#ifdef ICC_OS_LINUX
    LoadOpenIPMIDrivers();
#endif

    if (CompCode != 0)
        return CompCode;
#endif

    return 0;
}

int GetExtendedLogConf(IPMI20_SESSION_T *hSession, INT8U LogConfig)
{

#ifndef MSDOS
    int errVal = 0;
#endif
    BYTE ResBuf[TEMP_MAX_REQ_SIZE] = {0};
    int ResLen;
    BYTE CompCode;
    int retVal = 0;

    AMIGetEXTtLogConfRes_T pGetExtLogConfRes;

    ResLen = sizeof(AMIGetEXTtLogConfRes_T);

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIGetExtendedLogConf(hSession, &pGetExtLogConfRes, 0, RECIEVE_TIME_OUT);

        CompCode = pGetExtLogConfRes.CompletionCode;

        if (pGetExtLogConfRes.CompletionCode != 0)
        {
            return pGetExtLogConfRes.CompletionCode;
        }
        if (errVal != 0)
        {
            printf("The return value is %x \n", errVal);
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
            return errVal;
        }

#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_GET_EXTLOG_CONF, NULL, 0);

        retVal = SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode);
        if (CompCode == 0xC1)
        {
            return CompCode;
        }
        if (ACCESN_OK != retVal)
        {

            SET_ERR_CODE(ERR_BMC_COMM);
            printf("Error: Get feature status..\n");
            exit(0);
        }
        memcpy(&pGetExtLogConfRes.LogConfig, ResBuf, ResLen);
    }
    return 0;
}

UINT16 IsfeatureEnabled(IPMI20_SESSION_T *hSession, char *featureName)
{

    int DualImageRes = 0;
    BYTE CompCode = 0;
    unsigned short status = 0, enabledstatus = 0;
    INT8U LogConfig = 0;
    int DualImageSupport = 0;

    if ((strcmp(featureName, "CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT")) == 0)
    {
        CompCode = GetDualImageSupport(hSession, GETCURACTIVEIMG, &DualImageRes);
        if (CompCode == 0x0)
        {
            DualImageSupport = 1;
        }
        return CompCode;
    }
    else if ((strcmp(featureName, "CONFIG_SPX_FEATURE_PRESERVE_CONF_SUPPORT")) == 0)
    {
        CompCode = GetAllPreserveConfStatus(hSession, &status, &enabledstatus);
        return CompCode;
    }
    else if ((strcmp(featureName, "CONFIG_SPX_FEATURE_EXTENDEDLOG_SUPPORT")) == 0)
    {
        CompCode = GetExtendedLogConf(hSession, LogConfig);

        return CompCode;
    }
    else if ((strcmp(featureName, "CONFIG_SPX_FEATURE_GLOBAL_ON_LINE_FLASHING_SUPPORT")) == 0)
    {
        if (DualImageSupport == 1)
        {
            return 0;
        }
        else
        {
            return 0xc1;
        }
    }
    else
    {
        return 0xc1;
    }
}

/*
 * @fn GetFeatureStatus
 * @brief this function is used to Get Feature Status.
 * @param hSession - Current Session Pointer
 * @param featureName - Name of rhe feature
 * @return Returns -1 on error
 *                  1 - feature available.
 *                  0 - feature not enable/available.
 */
int GetFeatureStatus(IPMI20_SESSION_T *hSession, char *featureName)
{
#ifndef MSDOS
    BYTE featureStatus = 0;
#endif
    BYTE ResBuf[MAX_RES_SIZE] = {0};
    int ResLen, retVal;
    uint16 CompCode;

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        CompCode = LIBIPMI_HL_AMIGetFeatureStatus(hSession, featureName, &featureStatus, RECIEVE_TIME_OUT);
        if (CompCode != 0)
        {

            if (CompCode == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {

                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }

            if (CompCode == CC_INV_CMD)
            {
                /*printf("Warning: Current Yafutool not Support with given firmware!!!! \n");
                printf("Use Lower Version (< 3.0.0) of yafutools \n");
                exit(0);*/

                CompCode = IsfeatureEnabled(hSession, featureName);
                if (CompCode == CC_INV_CMD)
                {
                    return 0;
                }
                else if (CompCode == CC_SUCCESS)
                {
                    return 1;
                }
                else
                {
                    printf("Error Getting Feature Info \n");
                    exit(0);
                }
            }
            if (CompCode == CC_DEV_IN_FIRMWARE_UPDATE_MODE)
            {
                printf("Warning: Device is already in Firmware Update Mode...\n");
                exit(YAFU_CC_DEV_IN_FIRMWARE_UPDATE_MODE);
            }
            printf("Error: Get feature status..\n");
            exit(0);
        }
        else
            return featureStatus;
#endif
    }
    else
    {

        ResLen = sizeof(AMIGetFeatureStatusRes_T);
        FrameIfcReqHdr(CMD_AMI_GET_FEATURE_STATUS, featureName, strlen(featureName));

        retVal = SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode);
        if (CompCode == 0xC1)
        {
            /* printf("Warning: Current Yafutool not Support with given firmware!!!! \n");
             printf("Use Lower Version (< 3.0.0) of yafutools \n");
             exit(0);*/

            CompCode = IsfeatureEnabled(hSession, featureName);
            if (CompCode == 0xc1)
            {
                return 0;
            }
            else if (CompCode == 0)
            {
                return 1;
            }
            else
            {
                printf("Error Getting Feature Info \n");
                exit(0);
            }
        }
        if (ACCESN_OK != retVal)
        {

            SET_ERR_CODE(ERR_BMC_COMM);
            printf("Error: Get feature status..\n");
            exit(0);
        }
        return ResBuf[0];
    }

    return 0;
}

/*@fn SendMiscellaneousInfo
 * @brief This function is used o send miscellaneous info to BMC
 * @param hSession - Current Session Pointer
 * @param PreserveFlag - info about preserve sections
 * @return Returns 0 on success
 */
int SendMiscellaneousInfo(IPMI20_SESSION_T *hSession, INT8U PreserveFlag, AMIYAFUMiscellaneousRes_T *pRes)
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    BYTE CompCode;
    AMIYAFUMiscellaneousReq_T *pAMIYAFUMiscellaneousReq = NULL;
#ifndef MSDOS
    int errVal;
#endif

    pAMIYAFUMiscellaneousReq = (AMIYAFUMiscellaneousReq_T *)&ReqBuf;

    ReqLen = sizeof(AMIYAFUMiscellaneousReq_T);
    ResLen = sizeof(AMIYAFUMiscellaneousRes_T);

    pAMIYAFUMiscellaneousReq->PreserveFlag = PreserveFlag;

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIMiscellaneousInfo(hSession, pAMIYAFUMiscellaneousReq, (AMIYAFUMiscellaneousRes_T *)pRes, RECIEVE_TIME_OUT);
        if (errVal != 0)
        {
            if (errVal == LIBIPMI_MEDIUM_E_TIMED_OUT)
            {
                printf("Exiting as IPMI Session timed out due to inactivity\n");
                exit(YAFU_COMMAND_TIMEOUT_ERR);
            }
            return -1;
        }
#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_MISCELLANEOUS_INFO, ReqBuf, ReqLen);
        if (ACCESN_OK != SendTimedImbpRequest(&IfcReqHdr, 6000, ResBuf, &ResLen, &CompCode))
        {
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }
        memcpy((BYTE *)&pRes->MiscRes, (BYTE *)&ResBuf[0], ResLen);
    }
    return 0;
}

int GetConfBkupConfLocation(int *ConfLocation, int *BkupConfLocation, INT8U FwType)
{
    INT8U Count = 0;

    for (Count = 0; Count < ModuleCount; Count++)
    {
        if (FwType == 1) // Running Firmware
        {
            if (spstrcasecmp(CurrFwModHdr[Count]->ModuleName, "conf") == 0)
            {
                if (*ConfLocation != -1)
                {
                    *BkupConfLocation = Count;
                    break;
                }
                *ConfLocation = Count;
            }
        }
        else if (FwType == 2) // Image File
        {
            if (spstrcasecmp(FwModuleHdr[Count]->ModuleName, "conf") == 0)
            {
                if (*ConfLocation != -1)
                {
                    *BkupConfLocation = Count;
                    break;
                }
                *ConfLocation = Count;
            }
        }
    }

    if ((*ConfLocation == -1) && (*BkupConfLocation == -1))
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int ReadRngFirmwareRelBase(IPMI20_SESSION_T *hSession, char *CurReleaseID, char *CurCodeBaseID)
{
    int ResLen = 0, ReqLen = 0, errVal = 0, FMHFound = 0;
    BYTE CompCode = 0, Count = 0;
    unsigned int BlkSize = 0, TotalSize = 0;
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE] = {0};
    BYTE ResBuf[MAX_RES_SIZE * 2] = {0};
    BYTE *ReleaseBuf = NULL;
    INT32U ModuleLocation = 0;
    AMIYAFUReadFlashReq_T *pAMIYAFUReadFlashReq = NULL;
    AMIYAFUReadFlashRes_T *pAMIYAFUReadFlashRes = NULL;

    pAMIYAFUReadFlashReq = (AMIYAFUReadFlashReq_T *)ReqBuf;
    pAMIYAFUReadFlashRes = (AMIYAFUReadFlashRes_T *)ResBuf;

    ReqLen = sizeof(AMIYAFUReadFlashReq_T);

    for (Count = 0; CurrFwModHdr[Count] != NULL; Count++)
    {
        if (CurrFwModHdr[Count]->ModuleType == MODULE_FMH_FIRMWARE)
        {
            TotalSize = CurrFwModHdr[Count]->ModuleSize;
            FMHFound = 1;
            if (((CurrFwModHdr[Count]->Fmh_Version.FwVersion & 0x00FF) == FMH_MAJOR) &&
                (((CurrFwModHdr[Count]->Fmh_Version.FwVersion & 0xFF00) >> 8) <= FMH_MINOR))
            {
                ModuleLocation = CurrFwModHdr[Count]->ModuleLocation + CurrFwModHdr[Count]->FmhLocation;
            }
            else
            {
                ModuleLocation = CurrFwModHdr[Count]->ModuleLocation;
            }
            break;
        }
    }

    if (!FMHFound)
    {
        printf("FMH information not found\n");
        return -1;
    }

    ReleaseBuf = malloc(TotalSize);
    if (ReleaseBuf == NULL)
    {
        printf("Cannot allocate memory\n");
        fflush(stdout);
        return -1;
    }

    while (TotalSize != 0)
    {
        pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum = 0x00000001;
        pAMIYAFUReadFlashReq->ReadFlashReq.YafuCmd = CMD_AMI_YAFU_READ_FLASH;
        pAMIYAFUReadFlashReq->ReadFlashReq.Datalen = 0x07;
        pAMIYAFUReadFlashReq->offsettoread = ModuleLocation + BlkSize;
        pAMIYAFUReadFlashReq->Readwidth = 0;
        if (TotalSize < MAX_SIZE_TO_READ)
        {
            pAMIYAFUReadFlashReq->Sizetoread = TotalSize;
        }
        else
        {
            pAMIYAFUReadFlashReq->Sizetoread = MAX_SIZE_TO_READ;
        }

        ResLen = sizeof(AMIYAFUReadFlashRes_T) + pAMIYAFUReadFlashReq->Sizetoread;

        pAMIYAFUReadFlashReq->ReadFlashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUReadFlashReq->offsettoread, pAMIYAFUReadFlashReq->ReadFlashReq.Datalen);

        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIYAFUReadFlash(hSession, pAMIYAFUReadFlashReq, pAMIYAFUReadFlashRes, RECIEVE_TIME_OUT);
#endif
        }
        else
        {
            FrameIfcReqHdr(CMD_AMI_YAFU_READ_FLASH, ReqBuf, ReqLen);
            errVal = SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode);
        }

#ifndef MSDOS
        if ((ACCESN_OK != errVal) || (pAMIYAFUReadFlashRes->CompletionCode != 0))
#else
        if ((ACCESN_OK != errVal) || (CompCode != 0))
#endif
        {
            AMIYAFUNotAck *pAMIYAFUNotAck = (AMIYAFUNotAck *)ResBuf;
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }

        // Copy Firmware Module Data
        memcpy(ReleaseBuf + BlkSize, (char *)&ResBuf[sizeof(AMIYAFUReadFlashRes_T)], pAMIYAFUReadFlashReq->Sizetoread);

        BlkSize += pAMIYAFUReadFlashReq->Sizetoread;
        TotalSize -= pAMIYAFUReadFlashReq->Sizetoread;
    }

    GetReleaseaandCodeBaseVersion(ReleaseBuf, CurrFwModHdr[Count]->ModuleSize, "FW_RELEASEID=", CurReleaseID);
    GetReleaseaandCodeBaseVersion(ReleaseBuf, CurrFwModHdr[Count]->ModuleSize, "FW_CODEBASEVERSION=", CurCodeBaseID);

    free(ReleaseBuf);
    return 0;
}

int ReadBackupConfnBkconf(IPMI20_SESSION_T *hSession, int ReadFlag)
{
    unsigned int BlkSize = 0, TotalSize = 0;
    int ResLen, ReqLen, DiscardByte = 0;
    BYTE CompCode = 0;
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE] = {0};
    BYTE ResBuf[MAX_RES_SIZE * 2] = {0};
    int errVal = 0;
    AMIYAFUReadFlashReq_T *pAMIYAFUReadFlashReq = NULL;
    AMIYAFUReadFlashRes_T *pAMIYAFUReadFlashRes = NULL;
    int ConfLocation = -1, bkupConfLocation = -1;

    pAMIYAFUReadFlashReq = (AMIYAFUReadFlashReq_T *)ReqBuf;
    pAMIYAFUReadFlashRes = (AMIYAFUReadFlashRes_T *)ResBuf;

    if (GetConfBkupConfLocation(&ConfLocation, &bkupConfLocation, 1) != 0)
    {
        printf("Get Configuration Index Failed\n");
        return -1;
    }

    if (ReadFlag == 1)
    {
        printf("Reading Configurations       \r");

        TotalSize = CurrFwModHdr[ConfLocation]->ModuleSize;

        ConfData = malloc(TotalSize);
        if (ConfData == NULL)
        {
            printf("Allocating Memory Failed\n");
            return -1;
        }

        memset(ConfData, 0, TotalSize);

        while (TotalSize != 0)
        {
            memset(&ReqBuf[0], 0, sizeof(ReqBuf));
            memset(&ResBuf[0], 0, sizeof(ResBuf));
            pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum = 0x00000001;
            pAMIYAFUReadFlashReq->ReadFlashReq.YafuCmd = CMD_AMI_YAFU_READ_FLASH;
            pAMIYAFUReadFlashReq->ReadFlashReq.Datalen = 0x07;

            if (((CurrFwModHdr[ConfLocation]->Fmh_Version.FwVersion & 0x00FF) == FMH_MAJOR) &&
                (((CurrFwModHdr[ConfLocation]->Fmh_Version.FwVersion & 0xFF00) >> 8) <= FMH_MINOR))
            {
                pAMIYAFUReadFlashReq->offsettoread = CurrFwModHdr[ConfLocation]->ModuleLocation + CurrFwModHdr[ConfLocation]->FmhLocation + BlkSize;
            }
            else
            {
                pAMIYAFUReadFlashReq->offsettoread = CurrFwModHdr[ConfLocation]->ModuleLocation + BlkSize;
            }

            pAMIYAFUReadFlashReq->Readwidth = 0;

            if (TotalSize < MAX_SIZE_TO_READ)
            {
                pAMIYAFUReadFlashReq->Sizetoread = TotalSize;
            }
            else
            {
                pAMIYAFUReadFlashReq->Sizetoread = MAX_SIZE_TO_READ;
            }

            ReqLen = sizeof(AMIYAFUReadFlashReq_T);

            ResLen = sizeof(AMIYAFUReadFlashRes_T) + pAMIYAFUReadFlashReq->Sizetoread;

            pAMIYAFUReadFlashReq->ReadFlashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUReadFlashReq->offsettoread, pAMIYAFUReadFlashReq->ReadFlashReq.Datalen);

            if (byMedium != KCS_MEDIUM)
            {
#ifndef MSDOS
                DiscardByte = 0;
                errVal = IPMICMD_AMIYAFUReadFlash(hSession, pAMIYAFUReadFlashReq, pAMIYAFUReadFlashRes, RECIEVE_TIME_OUT);
#endif
            }
            else
            {
                DiscardByte = 1;
                FrameIfcReqHdr(CMD_AMI_YAFU_READ_FLASH, ReqBuf, ReqLen);
                errVal = SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode);
            }

#ifndef MSDOS
            if ((ACCESN_OK != errVal) || (pAMIYAFUReadFlashRes->CompletionCode != 0))
#else
            if ((ACCESN_OK != errVal) || (CompCode != 0))
#endif
            {
                AMIYAFUNotAck *pAMIYAFUNotAck = (AMIYAFUNotAck *)ResBuf;
                printf("Failure in Reading Configurations from Running Image\n");
                SET_ERR_CODE(ERR_BMC_COMM);
                free(ConfData);
                return -1;
            }

            memcpy(ConfData + BlkSize, &ResBuf[sizeof(AMIYAFUReadFlashRes_T) - DiscardByte], pAMIYAFUReadFlashReq->Sizetoread);

            BlkSize += pAMIYAFUReadFlashReq->Sizetoread;
            TotalSize -= pAMIYAFUReadFlashReq->Sizetoread;
        }
        printf("Reading  Configurations ...        done\n");
    }
    else
    {

        printf("Reading Backup Configurations       \r");

        TotalSize = CurrFwModHdr[bkupConfLocation]->ModuleSize;
        BlkSize = 0;

        BkupConfData = malloc(TotalSize);
        if (BkupConfData == NULL)
        {
            printf("Allocating Memory Failed\n");
            return -1;
        }

        memset(BkupConfData, 0, TotalSize);

        while (TotalSize != 0)
        {
            memset(&ReqBuf[0], 0, sizeof(ReqBuf));
            memset(&ResBuf[0], 0, sizeof(ResBuf));
            pAMIYAFUReadFlashReq->ReadFlashReq.Seqnum = 0x00000001;
            pAMIYAFUReadFlashReq->ReadFlashReq.YafuCmd = CMD_AMI_YAFU_READ_FLASH;
            pAMIYAFUReadFlashReq->ReadFlashReq.Datalen = 0x07;

            if (((CurrFwModHdr[bkupConfLocation]->Fmh_Version.FwVersion & 0x00FF) == FMH_MAJOR) &&
                (((CurrFwModHdr[bkupConfLocation]->Fmh_Version.FwVersion & 0xFF00) >> 8) <= FMH_MINOR))
            {
                pAMIYAFUReadFlashReq->offsettoread = CurrFwModHdr[bkupConfLocation]->ModuleLocation + CurrFwModHdr[bkupConfLocation]->FmhLocation + BlkSize;
            }
            else
            {
                pAMIYAFUReadFlashReq->offsettoread = CurrFwModHdr[bkupConfLocation]->ModuleLocation + BlkSize;
            }

            pAMIYAFUReadFlashReq->Readwidth = 0;

            if (TotalSize < MAX_SIZE_TO_READ)
            {
                pAMIYAFUReadFlashReq->Sizetoread = TotalSize;
            }
            else
            {
                pAMIYAFUReadFlashReq->Sizetoread = MAX_SIZE_TO_READ;
            }

            ResLen = sizeof(AMIYAFUReadFlashRes_T) + pAMIYAFUReadFlashReq->Sizetoread;

            pAMIYAFUReadFlashReq->ReadFlashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUReadFlashReq->offsettoread, pAMIYAFUReadFlashReq->ReadFlashReq.Datalen);

            if (byMedium != KCS_MEDIUM)
            {
#ifndef MSDOS
                DiscardByte = 0;
                errVal = IPMICMD_AMIYAFUReadFlash(hSession, pAMIYAFUReadFlashReq, pAMIYAFUReadFlashRes, RECIEVE_TIME_OUT);
#endif
            }
            else
            {
                DiscardByte = 1;
                FrameIfcReqHdr(CMD_AMI_YAFU_READ_FLASH, ReqBuf, ReqLen);
                errVal = SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode);
            }
#ifndef MSDOS
            if ((ACCESN_OK != errVal) || (pAMIYAFUReadFlashRes->CompletionCode != 0))
#else
            if ((ACCESN_OK != errVal) || (CompCode != 0))
#endif
            {
                AMIYAFUNotAck *pAMIYAFUNotAck = (AMIYAFUNotAck *)ResBuf;
                printf("Failure in Reading  Backup Configurations from Running Image\n");
                SET_ERR_CODE(ERR_BMC_COMM);
                free(BkupConfData);
                return -1;
            }

            memcpy(BkupConfData + BlkSize, &ResBuf[sizeof(AMIYAFUReadFlashRes_T) - DiscardByte], pAMIYAFUReadFlashReq->Sizetoread);

            BlkSize += pAMIYAFUReadFlashReq->Sizetoread;
            TotalSize -= pAMIYAFUReadFlashReq->Sizetoread;
        }
        printf("Reading Backup Configurations ...  done\n");
    }

    fflush(stdout);
    return 0;
}

int EraseFlash(IPMI20_SESSION_T *hSession, unsigned int ModLocation, unsigned int ModSize)
{
    AMIYAFUErashFlashReq_T *pAMIYAFUErashFlashReq = NULL;
    AMIYAFUErashFlashRes_T *pAMIYAFUErashFlashRes = NULL;
    unsigned int TotalBlkCount = 0, BlkNum = 0;
    BYTE CompCode = 0;
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE] = {0};
    BYTE ResBuf[TEMP_MAX_REQ_SIZE] = {0};
    int errVal = 0, ReqLen = 0, ResLen = 0;

    pAMIYAFUErashFlashReq = (AMIYAFUErashFlashReq_T *)&ReqBuf[0];
    pAMIYAFUErashFlashRes = (AMIYAFUErashFlashRes_T *)&ResBuf[0];

    BlkNum = ModLocation / EraseBlkSize;
    TotalBlkCount = ModSize / EraseBlkSize;

    while (TotalBlkCount > 0)
    {
        pAMIYAFUErashFlashReq->EraseFlashReq.Seqnum = 0x00000001;
        pAMIYAFUErashFlashReq->EraseFlashReq.YafuCmd = CMD_AMI_YAFU_ERASE_FLASH;
        pAMIYAFUErashFlashReq->EraseFlashReq.Datalen = 0x04;
        pAMIYAFUErashFlashReq->Blknumtoerase = BlkNum + 1;

        pAMIYAFUErashFlashReq->EraseFlashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUErashFlashReq->Blknumtoerase, pAMIYAFUErashFlashReq->EraseFlashReq.Datalen);

        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIYAFUEraseFlash(hSession, pAMIYAFUErashFlashReq, pAMIYAFUErashFlashRes, RECIEVE_TIME_OUT);
#endif
        }
        else
        {
            ReqLen = sizeof(AMIYAFUErashFlashReq_T);
            ResLen = sizeof(AMIYAFUErashFlashRes_T);
            FrameIfcReqHdr(CMD_AMI_YAFU_ERASE_FLASH, ReqBuf, ReqLen);
            errVal = SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode);
        }

#ifndef MSDOS
        if ((ACCESN_OK != errVal) || (pAMIYAFUErashFlashRes->CompletionCode != 0))
#else
        if ((ACCESN_OK != errVal) || (CompCode != 0))
#endif
        {
            AMIYAFUNotAck *pAMIYAFUNotAck = (AMIYAFUNotAck *)&pAMIYAFUErashFlashRes;
            printf("Failure in Erasing Configuration Sectors\n");
            SET_ERR_CODE(ERR_BMC_COMM);
            return -1;
        }

        BlkNum++;
        TotalBlkCount--;
    }
    return 0;
}

int WriteBackupConfnBkconf(IPMI20_SESSION_T *hSession, int WriteFlag)
{
    unsigned int BlkSize = 0, TotalSize = 0;
    unsigned int ModuleLocation = 0;
    int ResLen, ReqLen;
    BYTE CompCode = 0;
    BYTE ReqBuf[MAX_RES_SIZE * 2] = {0};
    BYTE ResBuf[TEMP_MAX_REQ_SIZE] = {0};
    int errVal = 0;
    AMIYAFUWriteFlashReq_T *pAMIYAFUWriteFlashReq = NULL;
    AMIYAFUWriteFlashRes_T *pAMIYAFUWriteFlashRes = NULL;
    int ConfLocation = -1, bkupConfLocation = -1;

    pAMIYAFUWriteFlashReq = (AMIYAFUWriteFlashReq_T *)&ReqBuf[0];
    pAMIYAFUWriteFlashRes = (AMIYAFUWriteFlashRes_T *)&ResBuf[0];

    if (GetConfBkupConfLocation(&ConfLocation, &bkupConfLocation, 2) != 0)
    {
        printf("Get Configuration Index Failed\n");
        return -1;
    }

    if (WriteFlag == 1)
    {
        TotalSize = FwModuleHdr[ConfLocation]->ModuleSize;

        if (((FwModuleHdr[ConfLocation]->Fmh_Version.FwVersion & 0x00FF) == FMH_MAJOR) &&
            (((FwModuleHdr[ConfLocation]->Fmh_Version.FwVersion & 0xFF00) >> 8) <= FMH_MINOR))
        {
            ModuleLocation = FwModuleHdr[ConfLocation]->ModuleLocation + FwModuleHdr[ConfLocation]->FmhLocation;
        }
        else
        {
            ModuleLocation = FwModuleHdr[ConfLocation]->ModuleLocation;
        }

        EraseFlash(hSession, ModuleLocation, TotalSize);

        while (TotalSize != 0)
        {
            memset(&ReqBuf[0], 0, sizeof(ReqBuf));
            pAMIYAFUWriteFlashReq->WriteFlashReq.Seqnum = 0x00000001;
            pAMIYAFUWriteFlashReq->WriteFlashReq.YafuCmd = CMD_AMI_YAFU_WRITE_FLASH;
            pAMIYAFUWriteFlashReq->offsettowrite = ModuleLocation + BlkSize;
            pAMIYAFUWriteFlashReq->Writewidth = 0;

            if (TotalSize < MAX_SIZE_TO_READ)
            {
                pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen = TotalSize;
            }
            else
            {
                pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen = MAX_SIZE_TO_READ;
            }

            memcpy(&ReqBuf[sizeof(AMIYAFUWriteFlashReq_T)], ConfData + BlkSize, pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen);

            pAMIYAFUWriteFlashReq->WriteFlashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUWriteFlashReq->offsettowrite, pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen + 5);

            if (byMedium != KCS_MEDIUM)
            {
#ifndef MSDOS
                errVal = IPMICMD_AMIYAFUWriteFlash(hSession, pAMIYAFUWriteFlashReq, pAMIYAFUWriteFlashRes, RECIEVE_TIME_OUT);
#endif
            }
            else
            {
                ReqLen = sizeof(AMIYAFUWriteFlashReq_T) + pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen;
                pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen += 5;
                ResLen = sizeof(AMIYAFUWriteFlashRes_T);
                FrameIfcReqHdr(CMD_AMI_YAFU_WRITE_FLASH, ReqBuf, ReqLen);
                errVal = SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode);
            }
#ifndef MSDOS
            if ((ACCESN_OK != errVal) || (pAMIYAFUWriteFlashRes->CompletionCode != 0))
#else
            if ((ACCESN_OK != errVal) || (CompCode != 0))
#endif
            {
                AMIYAFUNotAck *pAMIYAFUNotAck = (AMIYAFUNotAck *)ResBuf;
                printf("Failure in Writing Configurations to New Image\n");
                SET_ERR_CODE(ERR_BMC_COMM);
                free(ConfData);
                return -1;
            }

            BlkSize += (pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen - 5);
            TotalSize -= (pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen - 5);
        }

        printf("Setting Configurations ...         done\n");
        free(ConfData);
    }
    else
    {
        printf("Setting Backup Configurations         \r");

        TotalSize = FwModuleHdr[bkupConfLocation]->ModuleSize;
        BlkSize = 0;

        if (((FwModuleHdr[bkupConfLocation]->Fmh_Version.FwVersion & 0x00FF) == FMH_MAJOR) &&
            (((FwModuleHdr[bkupConfLocation]->Fmh_Version.FwVersion & 0xFF00) >> 8) <= FMH_MINOR))
        {
            ModuleLocation = FwModuleHdr[bkupConfLocation]->ModuleLocation + FwModuleHdr[bkupConfLocation]->FmhLocation;
        }
        else
        {
            ModuleLocation = FwModuleHdr[bkupConfLocation]->ModuleLocation;
        }

        EraseFlash(hSession, ModuleLocation, TotalSize);

        while (TotalSize != 0)
        {
            memset(&ReqBuf[0], 0, sizeof(ReqBuf));
            pAMIYAFUWriteFlashReq->WriteFlashReq.Seqnum = 0x00000001;
            pAMIYAFUWriteFlashReq->WriteFlashReq.YafuCmd = CMD_AMI_YAFU_WRITE_FLASH;
            pAMIYAFUWriteFlashReq->offsettowrite = ModuleLocation + BlkSize;
            pAMIYAFUWriteFlashReq->Writewidth = 0;
            if (TotalSize < MAX_SIZE_TO_READ)
            {
                pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen = TotalSize;
            }
            else
            {
                pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen = MAX_SIZE_TO_READ;
            }

            memcpy(&ReqBuf[sizeof(AMIYAFUWriteFlashReq_T)], BkupConfData + BlkSize, pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen);

            pAMIYAFUWriteFlashReq->WriteFlashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUWriteFlashReq->offsettowrite, pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen + 5);

            if (byMedium != KCS_MEDIUM)
            {
#ifndef MSDOS
                errVal = IPMICMD_AMIYAFUWriteFlash(hSession, pAMIYAFUWriteFlashReq, pAMIYAFUWriteFlashRes, RECIEVE_TIME_OUT);
#endif
            }
            else
            {
                ReqLen = sizeof(AMIYAFUWriteFlashReq_T) + pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen;
                pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen += 5;
                ResLen = sizeof(AMIYAFUWriteFlashRes_T);
                FrameIfcReqHdr(CMD_AMI_YAFU_WRITE_FLASH, ReqBuf, ReqLen);
                errVal = SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode);
            }

#ifndef MSDOS
            if ((ACCESN_OK != errVal) || (pAMIYAFUWriteFlashRes->CompletionCode != 0))
#else
            if ((ACCESN_OK != errVal) || (CompCode != 0))
#endif
            {
                AMIYAFUNotAck *pAMIYAFUNotAck = (AMIYAFUNotAck *)ResBuf;
                printf("Failure in Writing Backup Configurations to New Image\n");
                SET_ERR_CODE(ERR_BMC_COMM);
                free(BkupConfData);
                return -1;
            }

            BlkSize += (pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen - 5);
            TotalSize -= (pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen - 5);
        }
        printf("Setting Backup Configurations ...  done\n");
        free(BkupConfData);
    }

    fflush(stdout);
    return 0;
}

int SetBlkUBootVars(IPMI20_SESSION_T *hSession, env_t *env, unsigned int EnvSize)
{
    BYTE ReqBuf[MAX_RES_SIZE * 2] = {0};
    BYTE ResBuf[TEMP_MAX_REQ_SIZE] = {0};
    int errVal = 0, ReqLen = 0, ResLen = 0;
    unsigned int BlkSize = 0, TotalSize = 0, AllocatedSize = 0;
    BYTE CompCode = 0;
    BYTE *BootMem = NULL;
    AMIYAFUWriteFlashReq_T *pAMIYAFUWriteFlashReq = NULL;
    AMIYAFUWriteFlashRes_T *pAMIYAFUWriteFlashRes = NULL;

    printf("Setting Env variables ...               \r");
    fflush(stdout);

    pAMIYAFUWriteFlashReq = (AMIYAFUWriteFlashReq_T *)ReqBuf;
    pAMIYAFUWriteFlashRes = (AMIYAFUWriteFlashRes_T *)ResBuf;

    BootMem = malloc(EnvSize);
    if (BootMem == NULL)
    {
        return -1;
    }

    memset(BootMem, 0, EnvSize);

    memcpy(BootMem, &env->crc, sizeof(env->crc));
    memcpy(BootMem + sizeof(env->crc), env->data, EnvSize - 8);

    TotalSize = EnvSize - 4;

    if ((ImgOpt == -1) && ((FwModuleHdr[0]->Fmh_Version.FwVersion & 0x00FF) == FMH_MAJOR) &&
        (((FwModuleHdr[0]->Fmh_Version.FwVersion & 0xFF00) >> 8) <= FMH_MINOR) &&
        (FwModuleHdr[0]->AllocatedSize % (EraseBlkSize * 2) != 0))
    {
        AllocatedSize = (FwModuleHdr[0]->AllocatedSize - (2 * EnvSize));
    }
    else
    {
        AllocatedSize = (FwModuleHdr[0]->AllocatedSize - EnvSize);
    }

    while (TotalSize != 0)
    {
        memset(&ReqBuf[0], 0, sizeof(ReqBuf));
        pAMIYAFUWriteFlashReq->WriteFlashReq.Seqnum = 0x00000001;
        pAMIYAFUWriteFlashReq->WriteFlashReq.YafuCmd = CMD_AMI_YAFU_WRITE_FLASH;
        pAMIYAFUWriteFlashReq->offsettowrite = AllocatedSize + BlkSize;
        pAMIYAFUWriteFlashReq->Writewidth = 0;
        if (TotalSize < MAX_SIZE_TO_READ)
        {
            pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen = TotalSize;
        }
        else
        {
            pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen = MAX_SIZE_TO_READ;
        }

        memcpy(&ReqBuf[sizeof(AMIYAFUWriteFlashReq_T)], BootMem + BlkSize, pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen);

        pAMIYAFUWriteFlashReq->WriteFlashReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUWriteFlashReq->offsettowrite, pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen + 5);
        if (byMedium != KCS_MEDIUM)
        {
#ifndef MSDOS
            errVal = IPMICMD_AMIYAFUWriteFlash(hSession, pAMIYAFUWriteFlashReq, pAMIYAFUWriteFlashRes, RECIEVE_TIME_OUT);
#endif
        }
        else
        {
            ReqLen = sizeof(AMIYAFUWriteFlashReq_T) + pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen;
            pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen += 5;
            ResLen = sizeof(AMIYAFUWriteFlashReq_T);
            FrameIfcReqHdr(CMD_AMI_YAFU_WRITE_FLASH, ReqBuf, ReqLen);
            errVal = SendTimedImbpRequest(&IfcReqHdr, 60000, ResBuf, &ResLen, &CompCode);
        }
#ifndef MSDOS
        if ((ACCESN_OK != errVal) || (pAMIYAFUWriteFlashRes->CompletionCode != 0))
#else
        if ((ACCESN_OK != errVal) || (CompCode != 0))
#endif
        {
            printf("SetBlkUBootVars Communication Failure\n");
            SET_ERR_CODE(ERR_BMC_COMM);
            free(BootMem);
            return -1;
        }

        BlkSize += (pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen - 5);
        TotalSize -= (pAMIYAFUWriteFlashReq->WriteFlashReq.Datalen - 5);
    }

    printf("Setting Env variables...           done\n");
    free(BootMem);
    fflush(stdout);
    return 0;
}

#if defined(__x86_64__) || defined(WIN64)
int CompareMeVersion(IPMI20_SESSION_T *hSession, unsigned int MemOffset, unsigned int ImageSize)
#else
int CompareMeVersion(IPMI20_SESSION_T *hSession, unsigned long MemOffset, unsigned long ImageSize)
#endif
{
    BYTE ReqBuf[TEMP_MAX_REQ_SIZE];
    BYTE ResBuf[MAX_RES_SIZE];
    int ResLen, ReqLen;
    int errVal = 0;
    BYTE CompCode;

    AMIYAFUCompareMeVersionReq_T *pAMIYAFUCompareMeVersionReq = (AMIYAFUCompareMeVersionReq_T *)ReqBuf;
    AMIYAFUCompareMeVersionRes_T pAMIYAFUCompareMeVersionRes;

    ReqLen = sizeof(AMIYAFUCompareMeVersionReq_T);
    ResLen = sizeof(AMIYAFUCompareMeVersionRes_T);

    pAMIYAFUCompareMeVersionReq->CompareMeVersionReq.Seqnum = 0x00000001;
    pAMIYAFUCompareMeVersionReq->CompareMeVersionReq.YafuCmd = CMD_AMI_YAFU_COMPARE_ME_VERSION;
    pAMIYAFUCompareMeVersionReq->CompareMeVersionReq.Datalen = 0x08;
    pAMIYAFUCompareMeVersionReq->Memoffset = MemOffset;
    pAMIYAFUCompareMeVersionReq->Sizeofimage = ImageSize;
    pAMIYAFUCompareMeVersionReq->CompareMeVersionReq.CRC32chksum = CalculateChksum((char *)&pAMIYAFUCompareMeVersionReq->Memoffset, (2 * sizeof(INT32U)));

    if (byMedium != KCS_MEDIUM)
    {
#ifndef MSDOS
        errVal = IPMICMD_AMIYAFUCompareMeVersion(hSession, pAMIYAFUCompareMeVersionReq, &pAMIYAFUCompareMeVersionRes, RECIEVE_TIME_OUT);
#endif
    }
    else
    {
        FrameIfcReqHdr(CMD_AMI_YAFU_COMPARE_ME_VERSION, ReqBuf, ReqLen);
        errVal = SendTimedImbpRequest(&IfcReqHdr, 1000, ResBuf, &ResLen, &CompCode);
        memcpy(&pAMIYAFUCompareMeVersionRes.CompareMeVersionRes, ResBuf, ResLen);
    }

    if (ACCESN_OK != errVal)
    {

        SET_ERR_CODE(ERR_BMC_COMM);
        printf("Error: ME Get feature status..\n");
        return -1;
    }

    if (pAMIYAFUCompareMeVersionRes.CompareMeVersionRes.YafuCmd == CMD_AMI_YAFU_COMMON_NAK)
    {
        AMIYAFUNotAck pAMIYAFUNotAcknowledge;
        memcpy((char *)&pAMIYAFUNotAcknowledge, (char *)&pAMIYAFUCompareMeVersionRes, sizeof(AMIYAFUNotAck));
        if (pAMIYAFUNotAcknowledge.ErrorCode == YAFU_ERR_STATE)
        {
            printf("Compare Me Version Command Failed\n");
            return -1;
        }
    }

    if (CalculateChksum((char *)&pAMIYAFUCompareMeVersionRes.Current_Status, (10 * sizeof(INT8U))) != pAMIYAFUCompareMeVersionRes.CompareMeVersionRes.CRC32chksum)
    {
        printf("Error checksum\n");
        return -1;
    }

    ME_Current_Status = pAMIYAFUCompareMeVersionRes.Current_Status;

    // show the status of getting ME ver
    switch (ME_Current_Status)
    {
    case NORMALME:
        printf("The Existing ME image version: %d.%d.%d.%d\n",
               pAMIYAFUCompareMeVersionRes.Current_Version.Major,
               pAMIYAFUCompareMeVersionRes.Current_Version.Minor,
               pAMIYAFUCompareMeVersionRes.Current_Version.Hotfix,
               pAMIYAFUCompareMeVersionRes.Current_Version.BuildNo);
        break;
    case FAIL_NOT_SUPPORTED:
        printf("Current ME version is not support online updte\n");
        break;
    case FAIL_UNKNOW_ME_VER:
        printf("Unknow current ME Ver\n");
        break;
    case FAIL_GET_ME_VER:
        printf("Error: Get current ME version fail\n");
        break;
    default:
        printf("Error: Invalid return status\n");
        return -1;
    }

    ME_Comparison_Status = pAMIYAFUCompareMeVersionRes.Comparison_Status;

    if (pAMIYAFUCompareMeVersionRes.Comparison_Status != INVALID_ME_IMG)
    {
        printf("The uploaded ME image version: %d.%d.%d.%d\n",
               pAMIYAFUCompareMeVersionRes.New_Version.Major,
               pAMIYAFUCompareMeVersionRes.New_Version.Minor,
               pAMIYAFUCompareMeVersionRes.New_Version.Hotfix,
               pAMIYAFUCompareMeVersionRes.New_Version.BuildNo);
    }

    // show the status of Comparing ME Ver with new img and existing img
    switch (ME_Comparison_Status)
    {
    case NORMALME:
        break;
    case SAME_ME_VER:
        printf("The ME images are same Version\n");
        break;
    case OLDER_ME_VER:
        printf("The New ME image version is older than current existing image version\n");
        break;
    case INVALID_ME_IMG:
        printf("Error: Invalid image\n");
        break;
    default:
        printf("Error: Invalid return status\n");
        return -1;
    }

    return 0;
}

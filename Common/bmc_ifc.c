/*
 * Copyright (c) 1999, 2000
 * Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software must
 *    display the following acknowledgement:
 *
 *    This product includes software developed by Intel Corporation and its
 *    contributors.
 *
 * 4. Neither the name of Intel Corporation or its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL INTEL CORPORATION OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include    <stdio.h>
#include    <stdlib.h>
#include "Types.h"

#ifndef MSDOS
#include "icc_what.h"
#endif

#include "IPMIDefs.h"

#include    <time.h>
//#ifndef LINUX32
#ifndef ICC_OS_LINUX
#include    <conio.h>
#else
        #include    <unistd.h>
#endif
#include    "kcs.h"
#include    "debug.h"
#include    "bmc_ifc.h"
#include    "flashcmds.h"
#if (defined (WINDOWS))
//        #include    "Ipm_ifc.h"
        #include    <windows.h>
        #include    <winioctl.h>
#endif

/* Function prototypes */

/* Module variables */
#ifdef WINDOWS
static HANDLE   m_hDevice;
#endif

typedef struct{
        unsigned char NetFun;
        unsigned char Command;
        int DTime;
} PACKED DInfo_T ;


const DInfo_T m_DInfo [] =
{
        {OEM_AMI_NETFN, CMD_AMI_YAFU_ACTIVATE_FLASH,            8000},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_GET_FLASH_INFO,            1000},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_ALLOCATE_MEMORY,           0},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_WRITE_MEMORY,              1000},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_ERASE_COPY_FLASH,          1000},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_GET_ECF_STATUS,            1000},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_VERIFY_FLASH,              0},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_GET_VERIFY_STATUS,         0},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_DEACTIVATE_FLASH_MODE,     0},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_RESET_DEVICE,              0},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_PROTECT_FLASH,             0},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_FREE_MEMORY,               0},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_GET_FMH_INFO,              1000},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_GET_STATUS,                1000},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_GET_BOOT_CONFIG,           0},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_GET_BOOT_VARS,             0},
        {OEM_AMI_NETFN, CMD_AMI_YAFU_SET_BOOT_CONFIG,           0},
        {OEM_AMI_NETFN, CMD_AMI_VIRTUAL_DEVICE_SET_STATUS,           8000},
        
        
};
const DInfo_T m_DSendInfo [] =
{
       {OEM_AMI_NETFN, CMD_AMI_YAFU_WRITE_MEMORY,              0}
};
/*******************************************************************************
*   Routine: SendTimedImbpRequest
*
*   Arguments:
*       SystemIfcReq_T* Pointer to request data (see declaration for SystemIfcReq_T)
*       timeOut         How long to wait in ms (note: this param is ignored)
*       respDataPtr     Where to put response data (data without completion code)
*       respDataLen     Expected length of data to be returned
*       completionCode  Completion code returned from BMC
*
*   Returns:
*       ACCESN_OK         Reservation ID
*       ACCESN_ERROR      Error
*
*   Description:
*       This routine sends a formed IPMI request message to the BMC
*       and returns the reponse.
*
*******************************************************************************/
ACCESN_STATUS
SendTimedImbpRequest (SystemIfcReq_T*  reqPtr,         // request info and data
                      int              timeOut,        // how long to wait, in mSec units
                      BYTE*            respDataPtr,    // where to put response data
                      int*             respDataLen,    // how much response data there is
                      BYTE*            completionCode) // request status from dest controller
{
    DWORD   Status, resLen;
    BYTE    NetFn, LUN, Cmd, i;
    int delay = 0;
    Status = SendMessageByKcs (reqPtr->netFn, reqPtr->rsLun, reqPtr->cmdType, reqPtr->data, reqPtr->dataLength, timeOut/1000);
    if (IPMI_SUCCESS != Status)
    {
        SET_ERR_CODE (ERR_IFC_SEND_MSG);
        return ACCESN_ERROR;
    }
#ifdef ICC_OS_LINUX
    for (i=0; i< sizeof(m_DInfo)/sizeof(DInfo_T); i++){
        if((reqPtr->netFn == m_DInfo[i].NetFun) && (reqPtr->cmdType == m_DInfo[i].Command)){
                delay = m_DInfo[i].DTime;
                break;
        }
    }
    if(delay > 0)
        SleepMs(delay);

#endif
  
    resLen = *respDataLen;
    for(i=0; i<3; i++){
        Status = GetMessageByKcs(&NetFn, &LUN, &Cmd, completionCode, respDataPtr, &resLen, timeOut/100);
        if(IPMI_SUCCESS == Status)
                break;
        else
                SleepMs(delay+(100*i));
    }
    if (IPMI_SUCCESS != Status)
    {
        DBG_PRINT2 (DBG_LVL3, "GetMessage: Status - 0x%04X%04X\n", (WORD)(Status >> 16), (WORD)(Status & 0xFFFF));
        SET_ERR_CODE (ERR_IFC_GET_MSG);
        return ACCESN_ERROR;
    }
   
    *respDataLen = (int)resLen;

    if (((NetFn & 0xFE) != reqPtr->netFn) || (reqPtr->cmdType != Cmd))
    {
        SET_ERR_CODE (ERR_IFC_MSG_HDR);
        return ACCESN_ERROR;
    }
    return 0;
}


/*******************************************************************************
*   Routine: OpenIfc
*
*   Arguments:
*                   None
*   Returns:
*       0       Success
*       -1      Error
*
*   Description:
*       This routine opens the BMC interface
*
*******************************************************************************/
int
OpenIfc (void)
{
#ifdef WINDOWS
        m_hDevice = CreateFile ("\\\\.\\Imb", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ
                                | FILE_SHARE_WRITE,     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (NULL == m_hDevice)
#else
    if (0 != IsKCS ())
#endif
    {
        SET_ERR_CODE (ERR_IFC_OPEN);
        return -1;
    }
    return 0;
}

/*******************************************************************************
*   Routine: Stop
*
*   Arguments:
*                   None
*   Returns:
*       0        Success
*       -1       Error
*
*   Description:
*       This routine closes the BMC interface
*
*******************************************************************************/
int
CloseIfc (void)
{
#ifdef WINDOWS
        // Close System Interface Driver Handle
        CloseHandle(m_hDevice);
#endif
    return 0;
}

/*******************************************************************************
*   Routine: SetBMCPorts
*
*   Arguments:
*       DataPort        Data Port address
*       CmdStatusPort   Command/Status Port address
*
*   Returns:
*       0        Success
*       -1       Failed
*
*   Description:
*       Sets the BMC port address
*
*******************************************************************************/
int
SetBMCPorts (WORD DataPort, WORD CmdStatusPort)
{
//#if (defined (MSDOS) || defined (LINUX32) || defined (WINDOWS))
    SetKCSPorts (DataPort, CmdStatusPort);
//#endif
    return 0;
}


/*******************************************************************************
*   Routine: SleepMs
*
*   Arguments:
*                   None
*   Returns:
*                   None
*   Description:
*       Delay for specified amount of time (in milli seconds)
*
*******************************************************************************/
void
SleepMs (WORD ms)
{
#ifdef  _EFI
    BS->Stall(ms * 1000);
#endif
#ifdef MSDOS
    _asm
    {
        mov  dx,ms
    start:
        mov  cx,42h
        mov  bx,10h
    wait0:
        in   al,61h
        and  al,10h
        cmp  al,bl
        jz   wait0
        xor  bl,10h
        loop wait0
        dec  dx
        jnz  start
    }
#endif
#ifdef WINDOWS
    Sleep (ms);
#endif
#ifdef ICC_OS_LINUX
    usleep (ms * 1000);
#endif
    return;
}

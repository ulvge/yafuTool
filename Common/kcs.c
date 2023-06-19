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
#include <time.h>

#ifndef MSDOS
#include "icc_what.h"
#include "IPMIDefs.h"
#endif
#ifndef ICC_OS_LINUX
#include <conio.h>
#else
#include <unistd.h>
#include <sys/io.h>
#endif
#include <stdlib.h>
#include <stdio.h>

#include "debug.h"
#include "kcs.h"
#include "main.h"
#ifdef WINDOWS
#include <winioctl.h>
#endif

/* SMS Transfer Stream Control Codes */
#define KCS_GET_STATUS_ABORT 0x60
#define KCS_WRITE_START 0x61
#define KCS_WRITE_END 0x62
#define KCS_READ 0x68

#define KCS_STATE_MASK 0xC0
#define KCS_IDLE_STATE 0x00
#define KCS_READ_STATE 0x40
#define KCS_WRITE_STATE 0x80
#define KCS_ERROR_STATE 0xC0

#define KCS_OBF_FLAG 0x01
#define KCS_IBF_FLAG 0x02
#define KCS_SMS_MSG_FLAG 0x04

#define KCS_OBF ((BYTE)(ioread(m_CmdStatusPort) & KCS_OBF_FLAG))
#define KCS_READ_BMC_DATA ((BYTE)(ioread(m_DataPort)))
#define KCS_WRITE_BMC_DATA(x) (iowrite(m_DataPort, x))
#define KCS_IBF ((BYTE)(ioread(m_CmdStatusPort) & KCS_IBF_FLAG))
#define KCS_WRITE_BMC_CMD(x) (iowrite(m_CmdStatusPort, x))
#define KCS_SMS_MSG ((BYTE)(ioread(m_CmdStatusPort) & KCS_SMS_MSG_FLAG))
#define KCS_STATUS ((BYTE)(ioread(m_CmdStatusPort)))
#define KCS_STATE ((BYTE)(ioread(m_CmdStatusPort) & KCS_STATE_MASK))

/* Module variables */
static BYTE m_Status;
static WORD m_DataPort = 0xCA2;
static WORD m_CmdStatusPort = 0xCA3;
BYTE MsgBuf[MAX_KCS_LENGTH];

/* Function prototypes */
static DWORD KCSSendMessage(BYTE *MsgBuf, DWORD Length, WORD timeOut);
static DWORD KCSReadBMCData(BYTE *MsgBuf, DWORD BufSize, DWORD *NumRead, WORD timeOut);
static int KCSMessageReady(void);
static int WaitWhileIbf(DWORD Timeout);
static int WaitTilObf(DWORD Timeout);
static int CheckMessageReady(BYTE Target, DWORD Timeout);
static int CheckTimeout(DWORD Goal);
static void SetTimeout(DWORD *Goal, DWORD TimeoutInSec);
static DWORD Sec(void);
static void iowrite(WORD Port, BYTE Val);
static BYTE ioread(WORD Port);

/*******************************************************************************
 *   Routine: IsKCS
 *
 *   Arguments:
 *       None
 *
 *   Returns:
 *       0        Is KCS interface
 *       -1       Is NOT KCS interface
 *
 *   Description:
 *       Check if it is KCS interface
 *
 *******************************************************************************/
int IsKCS(void)
{

#ifdef ICC_OS_LINUX
    if (0 != iopl(3))
    {
        printf("ERROR: iopl\n");
        return -1;
    }
#endif

    if ((ioread(m_CmdStatusPort) == 0xFF) && (ioread(m_DataPort) == 0xFF))
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

/*******************************************************************************
 *   Routine: SendMessageByKcs
 *
 *   Arguments:
 *       NetFn               - Network Function code.
 *       LUN                 - Logical Unit Number.
 *       Cmd                 - Command code.
 *                           Specifies the operation that is to be
 *                           executed under the specified Network Function.
 *       *Data               - A pointer to zero or more bytes of data,
 *                           as required by the given command.
 *                           The general convention is to pass data LS-byte first,
 *                           but check the individual command specifications to be sure.
 *           DataLen             - Length of the Data in bytes.
 *
 *   Returns:
 *       IPMI_SUCCESS        The request message is successfully sent to the BMC.
 *       IPMI_FAILURE        The request message is failed to send to the BMC.
 *       IPMI_TIMEOUT        Time out error.
 *       IPMI_BMC_BUSY       BMC is busy.
 *
 *   Description:
 *       Send message to BMC throw Kcs interface.
 *
 *******************************************************************************/
DWORD
SendMessageByKcs(BYTE NetFn,
                 BYTE LUN,
                 BYTE Cmd,
                 BYTE *Data,
                 DWORD DataLen,
                 WORD timeOut)
{
    BYTE NetFnAndLun = 0;
    DWORD Length = 0;
    DWORD i = 0;
    DWORD Status;

    /* Combine netfn and lun */
    NetFnAndLun = (NetFn << 2) | LUN;

    /* Send NetfnLun and cmd along with Message Data */
    Length = 2 + DataLen;

    MsgBuf[0] = NetFnAndLun;
    MsgBuf[1] = Cmd;

    for (i = 0; i < DataLen; i++)
    {
        MsgBuf[i + 2] = Data[i];
    }
    Status = KCSSendMessage(MsgBuf, Length, timeOut);
    return (Status);
}

/*******************************************************************************
 *   Routine: GetMessageByKcs
 *
 *   Arguments:
 *       *NetFn              - Network Function code, which was passed in the Request Mes-sage.
 *       *LUN                - Logical Unit Number, which was passed in the Request Mes-sage.
 *       *Cmd                - Command code. Specifies the operation that is to be executed
 *                           under the specified Network Function.
 *                           This is a return of the Cmd code that was passed in the Request Message.
 *       *CompletionCode     - The IPMI Completion Code.
 *       *Data               - A pointer to zero or more bytes of data, as required
 *                           by the given command. The general convention is to
 *                           pass data LS-byte first, but check the individual command
 *                           specifications to be sure.
 *       *DataLen            - IN: Size of the buffer that used to store data read from the BMC,
 *                           not including the bytes of NetFn, LUN and Cmd.
 *                           OUT: The actually bytes of data read from BMC,
 *                           not including the bytes of NetFn, LUN and Cmd.
 *
 *   Returns:
 *       IPMI_SUCCESS        Successfully get the response message from the BMC.
 *       IPMI_FAILURE        Response data didn't successfully return.
 *       IPMI_BUF_TOO_SMALL  The size of returned buffer is larger than the size of allocated buffer.
 *       IPMI_TIMEOUT        Time out error.
 *       IPMI_BMC_BUSY       BMC is busy.
 *
 *   Description:
 *       Get message from BMC throw Kcs interface
 *
 *******************************************************************************/
DWORD
GetMessageByKcs(BYTE *NetFn,
                BYTE *LUN,
                BYTE *Cmd,
                BYTE *CompletionCode,
                BYTE *Data,
                DWORD *DataLen,
                WORD timeOut)
{
    DWORD Length = 0;
    DWORD Status;
    DWORD OutLength = 0;
    BYTE NetFnAndLun = 0;
    DWORD i = 0;

    /* Read NetfnLun, cmd and completion code along with Message data */
    Length = 3 + *DataLen;
    Status = KCSReadBMCData(MsgBuf, Length, &OutLength, timeOut);

    if (Status != IPMI_SUCCESS)
    {
        return (Status);
    }

    /* decompose returned data. */
    NetFnAndLun = MsgBuf[0];
    *Cmd = MsgBuf[1];
    *CompletionCode = MsgBuf[2];

    if (OutLength < 3)
    {
        return IPMI_FAILURE;
    }
    for (i = 0; i < (OutLength - 3); i++)
    {
        Data[i] = MsgBuf[i + 3];
    }
    *DataLen = OutLength - 3;

    *NetFn = NetFnAndLun >> 2;
    *LUN = NetFnAndLun & 0x03;

    return (IPMI_SUCCESS);
}

/*******************************************************************************
 *   Routine: SetKCSPorts
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
 *       Sets the KCS port address
 *
 *******************************************************************************/
int SetKCSPorts(WORD DataPort, WORD CmdStatusPort)
{
    m_DataPort = DataPort;
    m_CmdStatusPort = CmdStatusPort;
    if (vdbg)
        printf("Setting the port numbers %x %x\n", m_DataPort, m_CmdStatusPort);
    return 0;
}

/*******************************************************************************
 *   Routine: WaitWhileIbf
 *
 *   Arguments:
 *       Timeout     Seconds for time out
 *
 *   Returns:
 *       -1        Time out
 *       0         No time out
 *
 *   Description:
 *       IBF per IPMI spec, check time out
 *
 *******************************************************************************/
static int
WaitWhileIbf(DWORD Timeout)
{
    DWORD OverallTimeout = 0;
    SetTimeout(&OverallTimeout, Timeout);
    while (KCS_STATUS & KCS_IBF_FLAG)
    {
        if (CheckTimeout(OverallTimeout))
        {
            return -1;
        }
    }
    return 0;
}

/*******************************************************************************
 *   Routine: WaitTilObf
 *
 *   Arguments:
 *       Timeout     Milliseconds for time out
 *
 *   Returns:
 *       -1       Time out
 *        0       No time out
 *
 *   Description:
 *       OBF per IPMI spec, check time out
 *
 *******************************************************************************/
static int
WaitTilObf(DWORD Timeout)
{
    DWORD OverallTimeout;
    SetTimeout(&OverallTimeout, Timeout);

    while (!(KCS_STATUS & KCS_OBF_FLAG))
    {
        if (CheckTimeout(OverallTimeout))
        {
            return -1;
        }
    }

    return 0;
}
#if 0
/*******************************************************************************
*   Routine: KCSMessageReady
*
*   Arguments:
*       None
*
*   Returns:
*       0        Ready
*       -1       Not ready
*
*   Description:
*       Check KCS message ready or not
*
*******************************************************************************/
static int 
KCSMessageReady (void)
{
    return KCS_SMS_MSG ? 0 : -1;
}

/*******************************************************************************
*   Routine: CheckMessageReady
*
*   Arguments:
*       Target      Target that owns the message
*       Timeout     Seconds for time out
*
*   Returns:
*       0        Message ready
*       -1       Not ready
*
*   Description:
*       Check message ready for specified target
*
*******************************************************************************/
static int
CheckMessageReady (BYTE Target, DWORD Timeout)
{
    DWORD OverallTimeout;

    if (Target == BMC_SLAVE_ADDR)
    {
        SetTimeout (&OverallTimeout, Timeout);

        do
        {
            if (CheckTimeout (OverallTimeout)) { return -1; }
            m_Status = KCS_STATUS;
        } while ((m_Status & KCS_OBF_FLAG) == 0);

        return 0;
    }
    else
    {
        SetTimeout (&OverallTimeout, Timeout);

        while (!(KCS_SMS_MSG))
        {
            if (CheckTimeout (OverallTimeout)) { return -1; }
                }
        return 0;
    }
}
#endif
/*******************************************************************************
 *   Routine: KCSSendMessage
 *
 *   Arguments:
 *       *MsgBuf         Message buffer to be sent
 *       Length          Message length
 *
 *   Returns:
 *       IPMI_SUCCESS    The request message successfully sent to the BMC.
 *       IPMI_FAILURE    The request message failed to send to the BMC.
 *       IPMI_TIMEOUT    Time out error.
 *       IPMI_BMC_BUSY   BMC is busy.
 *
 *   Description:
 *       KCS send message to BMC
 *
 *******************************************************************************/
static DWORD
KCSSendMessage(BYTE *MsgBuf, DWORD Length, WORD timeOut)
{
    DWORD i;
    char Data;

    /* BMC could be busy */
    if (WaitWhileIbf(timeOut))
    {
        DBG_PRINT(DBG_LVL4, "WaitWhileIbf1 timeOut\n");
        return IPMI_BMC_BUSY;
    }

    /* Read status - if OBF == 1, read data byte to clear OBF */
    if (KCS_STATUS & KCS_OBF_FLAG)
    {
        Data = KCS_READ_BMC_DATA;
    }

    /* Set BMC to Write state */
    KCS_WRITE_BMC_CMD(KCS_WRITE_START);
    /* Wait for IBF == 0 */
    if (WaitWhileIbf(timeOut))
    {
        DBG_PRINT(DBG_LVL4, "WaitWhileIbf2 timeOut\n");
        return IPMI_TIMEOUT;
    }

    /* Is BMC in Write State ? */
    if ((KCS_STATUS & KCS_STATE_MASK) != KCS_WRITE_STATE)
    {
        DBG_PRINT(DBG_LVL4, "BMC in Write State\n");
        return IPMI_FAILURE;
    }

    /* Clear OBF if set (clear by reading data) */
    if (KCS_STATUS & KCS_OBF_FLAG)
    {
        Data = KCS_READ_BMC_DATA;
    }

    /* Send message - 1 byte (last byte is handled after the loop */
    for (i = 0; i < Length - 1; ++i)
    {
        /* Output data byte */
        // usleep(100);
        // printf("posting for i =%d time\n",i);
        KCS_WRITE_BMC_DATA(MsgBuf[i]);

        /* Wait for IBF == 0 */

        if (WaitWhileIbf(timeOut))
        {
            DBG_PRINT(DBG_LVL4, "WaitWhileIbf3 timeOut\n");
            return IPMI_TIMEOUT;
        }
        /* Is BMC in Write State ? */
        if ((KCS_STATUS & KCS_STATE_MASK) != KCS_WRITE_STATE)
        {
            DBG_PRINT(DBG_LVL4, "BMC in Write State\n");

            return IPMI_FAILURE;
        }

        /* Clear OBF if set (clear by reading) */
        if (KCS_STATUS & KCS_OBF_FLAG)
        {
            Data = KCS_READ_BMC_DATA;
        }
    }

    /* Write-End command */
    KCS_WRITE_BMC_CMD(KCS_WRITE_END);

    /* Wait for IBF == 0 */
    if (WaitWhileIbf(timeOut))
    {
        DBG_PRINT(DBG_LVL4, "WaitWhileIbf4 timeOut\n");

        return IPMI_TIMEOUT;
    }

    /* Is BMC in Write State ? */
    if ((KCS_STATUS & KCS_STATE_MASK) != KCS_WRITE_STATE)
    {
        return IPMI_FAILURE;
    }

    /* Clear OBF if set (clear by reading) */
    if (KCS_STATUS & KCS_OBF_FLAG)
    {
        Data = KCS_READ_BMC_DATA; // Clear by reading data
    }

    /* Output last data byte */
    KCS_WRITE_BMC_DATA(MsgBuf[i]);

    return IPMI_SUCCESS;
}

/*******************************************************************************
 *   Routine: KCSReadBMCData
 *
 *   Description:
 *       KCS read data from BMC
 *
 *   Arguments:
 *       *MsgBuf             Message buffer to be returned
 *       BufSize             Size of message buffer
 *       *NumRead            Actual number of bytes read from BMC
 *
 *   Returns:
 *       IPMI_SUCCESS        Successfully received response message from the BMC.
 *       IPMI_FAILURE        Response data didn't successfully return.
 *       IPMI_BUF_TOO_SMALL  Buffer is too small.
 *       IPMI_TIMEOUT        Time out error.
 *       IPMI_BMC_BUSY       BMC is busy.
 *
 *   Description:
 *       KCS read data from BMC
 *
 *******************************************************************************/
static DWORD
KCSReadBMCData(BYTE *MsgBuf, DWORD BufSize, DWORD *NumRead, WORD timeOut)
{
    /* For last byte dummy read */
    BufSize++;
    /* Read the message */
    for (*NumRead = 0; *NumRead < BufSize; ++*NumRead)
    {
        /* Wait for IBF == 0 */
        if (WaitWhileIbf(timeOut))
        {
            DBG_PRINT(DBG_LVL4, "WaitWhileIbf timedout!\n");
            return IPMI_TIMEOUT;
        }

        /* Read BMC status */
        m_Status = KCS_STATUS & KCS_STATE_MASK;
        /* Is BMC in Read State ? */
        if (m_Status != KCS_READ_STATE)
        {
            /* Is BMC in idle state ? */
            if (m_Status != KCS_IDLE_STATE)
            {
                DBG_PRINT(DBG_LVL4, "KCS not in idle state!\n");
                return IPMI_FAILURE;
            }

            /* Wait for the OBF */
            if (WaitTilObf(timeOut))
            {
                DBG_PRINT(DBG_LVL4, "WaitTilObf1 timedout!\n");
                return IPMI_TIMEOUT;
            }
            /* Read dummy byte */
            KCS_READ_BMC_DATA;
            return IPMI_SUCCESS;
        }

        /* Wait for the OBF */
        if (WaitTilObf(timeOut))
        {
            DBG_PRINT(DBG_LVL4, "WaitTilObf2 timedout!\n");
            return IPMI_TIMEOUT;
        }

        /* Read data byte */
        MsgBuf[*NumRead] = KCS_READ_BMC_DATA;
        DBG_PRINT1(DBG_LVL4, "KCS: Data - %X\n", MsgBuf[*NumRead]);

        /* Send the READ command to the BMC */
        KCS_WRITE_BMC_DATA(KCS_READ);
    }

    /* If we fell thru, the buffer is too small! */
    DBG_PRINT(DBG_LVL4, "the buffer is too small!\n");
    return IPMI_BUF_TOO_SMALL;
}

/*******************************************************************************
 *   Routine: CheckTimeout
 *
 *   Arguments:
 *       Goal        Seconds value to time out
 *
 *   Returns:
 *       -1      Time out
 *       0       Not time out
 *
 *   Description:
 *       Check time out in seconds
 *
 *******************************************************************************/
static int
CheckTimeout(DWORD Goal)
{
    if (Sec() >= Goal)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

/*******************************************************************************
 *   Routine: SetTimeout
 *
 *   Arguments:
 *       *Goal           Time that to be time out
 *       TimeoutInSec    Time out value in seconds
 *
 *   Returns:
 *       None
 *
 *   Description:
 *       Set time out value
 *
 *******************************************************************************/
static void
SetTimeout(DWORD *Goal, DWORD TimeoutInSec)
{
    *Goal = Sec() + TimeoutInSec;
}

/*******************************************************************************
 *   Routine: Sec
 *
 *   Arguments:
 *       None
 *
 *   Returns:
 *       Time in seconds
 *
 *   Description:
 *       Get time in seconds
 *
 *******************************************************************************/
static DWORD
Sec(void)
{
    DWORD Seconds = 0;
#ifdef _EFI
    EFI_TIME Time;
    EFI_TIME_CAPABILITIES cap;
    RT->GetTime(&Time, &cap);
    Seconds = (Time.Day * 24 * 60 * 60 + Time.Hour * 60 * 60 + Time.Minute * 60 + Time.Second);
#else
    time_t t;
    time(&t);
    Seconds = (DWORD)t;
#endif

    return Seconds;
}

/*******************************************************************************
 *   Routine: iowrite
 *
 *   Arguments:
 *       Port        IO Port to write byte value to
 *       Val         Byte value to write to IO port
 *
 *   Returns:
 *       None
 *
 *   Description:
 *       Write byte to IO port
 *
 *******************************************************************************/
static void
iowrite(WORD Port, BYTE Val)
{
#ifdef _EFI
    outp(Port, Val);
#endif
#ifdef MSDOS
    outp(Port, Val);
#endif
#ifdef ICC_OS_LINUX
    outb(Val, Port);
#endif
#ifdef WINDOWS
    WriteIOByte(Port, 1, Val);
#endif
}

/*******************************************************************************
 *   Routine: ioread
 *
 *   Arguments:
 *       Port        IO Port to read byte value from
 *
 *   Returns:
 *       None
 *
 *   Description:
 *       Read byte from IO port
 *
 *******************************************************************************/
static BYTE
ioread(WORD Port)
{
    BYTE Val;
#ifdef _EFI
    Val = inp(Port);
#endif
#ifdef MSDOS
    Val = inp(Port);
#endif
#ifdef ICC_OS_LINUX
    Val = inb(Port);
#endif
#ifdef WINDOWS
    ReadIOByte(Port, 1, &Val);
#endif
    return Val;
}

#ifdef WINDOWS
// =======================Function Description==================================
//  FUNCTION : ReadIOByte
//
//  Description :
//     This routine is used to Read the given port.
//
//  Input Params :
//     [Paramter 1] : Indicates port address
//     [Paramter 2] : Indicates whether I/O mapped or memory mapped
//     [Paramter 3] : Indicates readed data
//
//  return : TRUE indicates read port was successfully read; otherwise FALSE;
//==============================================================================
BOOL ReadIOByte(DWORD dwPort, BOOL bIsIO, PBYTE pbyDataByte)
{
    DWORD dwBytesReturned = 0;
    DWORD dwIOCtlCode = 0;
    GENPORTDATA GenPortData;

    GenPortData.ulPortNumber = dwPort;
    GenPortData.ulMemType = bIsIO;
    GenPortData.DataType = SINGLE_BYTE;

    //	if(g_boIsNT)
    dwIOCtlCode = IOCTL_GENERICDRV_PORT_READ;
    //	else
    //		dwIOCtlCode = IOCTL_GENERICDRV_PORT_READ95;

    if (DeviceIoControl(m_hD, dwIOCtlCode, (PVOID)&GenPortData, sizeof(GenPortData),
                        (PVOID)&GenPortData, sizeof(GenPortData), &dwBytesReturned, NULL))
    {
        *pbyDataByte = GenPortData.Value.CharData;
        return TRUE;
    }
    else
        return FALSE;
}
// =======================Function Description==================================
//  FUNCTION : WriteIOByte
//
//  Description :
//     This routine is used to Write the given port.
//
//  Input Params :
//     [Paramter 1] : Indicates port address
//     [Paramter 2] : Indicates whether I/O mapped or memory mapped
//     [Paramter 3] : Indicates writed data
//
//  return : TRUE indicates Write port was successfully write; otherwise FALSE;
//==============================================================================
BOOL WriteIOByte(DWORD dwPort, BOOL bIsIO, BYTE byDataByte)
{
    DWORD dwBytesReturned = 0;
    DWORD dwIOCtlCode = 0;
    GENPORTDATA GenPortData;

    //	if(g_boIsNT)
    dwIOCtlCode = IOCTL_GENERICDRV_PORT_WRITE;
    //	else
    //		dwIOCtlCode = IOCTL_GENERICDRV_PORT_WRITE95;

    GenPortData.ulPortNumber = dwPort;
    GenPortData.ulMemType = bIsIO;
    GenPortData.DataType = SINGLE_BYTE;
    GenPortData.Value.CharData = byDataByte;

    if (!DeviceIoControl(m_hD, dwIOCtlCode, (PVOID)&GenPortData, sizeof(GenPortData),
                         (PVOID)&GenPortData, sizeof(GenPortData), &dwBytesReturned, NULL))
        return FALSE;

    return TRUE;
}
#endif

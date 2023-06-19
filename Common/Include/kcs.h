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
/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2002-2005, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/****************************************************************
  $Header: $

  $Revision: $

  $Date: $
 *****************************************************************/
/*****************************************************************
 *
 * kcs.h
 *
 *  Author: Jason Messer <jasonm@ami.com>
 *          Basavaraj Astekar <basavaraja@ami.com>
 *
 *****************************************************************/
#ifndef KCS_H
#define KCS_H

#define BMC_SLAVE_ADDR          0x20
#define MAX_KCS_LENGTH          0x50000

/* Error codes for STATUS */
#define IPMI_SUCCESS            0x00000000
#define IPMI_BUF_TOO_SMALL      0x00000001
#define IPMI_TIMEOUT            0x00000002
#define IPMI_FAILURE            0x00000003
#define IPMI_BMC_BUSY           0x00000004

/* Function prototypes */
extern int   IsKCS (void);
extern DWORD SendMessageByKcs (BYTE NetFn, BYTE LUN, BYTE Cmd, BYTE *Data, DWORD DataLen, WORD  timeOut);
extern DWORD GetMessageByKcs (BYTE *NetFn, BYTE *LUN, BYTE *Cmd, BYTE *CompletionCode, BYTE *Data, DWORD *DataLen, WORD  timeOut);
extern int   SetKCSPorts (WORD DataPort, WORD CmdStatusPort);

#ifdef WINDOWS
typedef union{
	ULONG	LongData;
	USHORT	ShortData;
	UCHAR	CharData;

}PORTVALUE, *PPORTVALUE;
//Number of Bytes of data to be read or written into the port.
typedef enum{
	SINGLE_BYTE	= 0,
	TWO_BYTES	= 1,
	FOUR_BYTES	= 2

}DATATYPE, *PDATATYPE;
//Structure for Port access.
typedef struct{
	ULONG		ulPortNumber;	//Specify the Port No from which to be accessed.
	ULONG		ulMemType;
	DATATYPE	DataType;
	PORTVALUE	Value;

}GENPORTDATA, *PGENPORTDATA;

//                                WIN XP/NT

#define FILE_DEVICE_GENERICDRV			64000
#define	GENERICDRV_IOCTL_INDEX			3000


#define IOCTL_GENERICDRV_ALLOC_BUFFER	CTL_CODE(FILE_DEVICE_GENERICDRV,		\
						GENERICDRV_IOCTL_INDEX + 0,	\
						METHOD_BUFFERED,		\
						FILE_ANY_ACCESS)

#define IOCTL_GENERICDRV_DEALLOC_BUFFER	CTL_CODE(FILE_DEVICE_GENERICDRV,		\
						GENERICDRV_IOCTL_INDEX + 1,	\
						METHOD_BUFFERED,		\
						FILE_ANY_ACCESS)

#define IOCTL_GENERICDRV_PHY_TO_VIRTUAL	CTL_CODE(FILE_DEVICE_GENERICDRV,		\
						GENERICDRV_IOCTL_INDEX + 2,	\
						METHOD_BUFFERED,		\
						FILE_ANY_ACCESS)


#define IOCTL_GENERICDRV_UNMAP	CTL_CODE(FILE_DEVICE_GENERICDRV,			\
					GENERICDRV_IOCTL_INDEX + 3,		\
					METHOD_BUFFERED,			\
					FILE_ANY_ACCESS)

#define IOCTL_GENERICDRV_PORT_READ		CTL_CODE(FILE_DEVICE_GENERICDRV,	\
						GENERICDRV_IOCTL_INDEX + 4,	\
						METHOD_BUFFERED,		\
						FILE_ANY_ACCESS)

//Out Buffer can be NULL.
#define IOCTL_GENERICDRV_PORT_WRITE		CTL_CODE(FILE_DEVICE_GENERICDRV,	\
						GENERICDRV_IOCTL_INDEX + 5,	\
						METHOD_BUFFERED,		\
						FILE_ANY_ACCESS)

//Special case for DMI thro' SMI port access - Ashrafj
#define	IOCTL_GENERICDRV_PORT_DMIACCESS	CTL_CODE(FILE_DEVICE_GENERICDRV,		\
						GENERICDRV_IOCTL_INDEX + 6,	\
						METHOD_BUFFERED,		\
						FILE_ANY_ACCESS)

//This interface is for getting driver version - Ashrafj
#define	IOCTL_GENERICDRV_GET_VERSION	CTL_CODE(FILE_DEVICE_GENERICDRV,		\
						GENERICDRV_IOCTL_INDEX + 7,	\
						METHOD_BUFFERED,		\
						FILE_ANY_ACCESS)
#endif
#endif //KCS_H

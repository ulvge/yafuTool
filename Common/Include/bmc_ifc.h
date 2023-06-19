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
* Filename: main.h
*
* Author   : Winston <winstonv@amiindia.co.in>
*
******************************************************************/

#ifndef BMC_IFC_H
#define BMC_IFC_H


#define BMC_SLAVE_ADDR          0x20
#define BMC_LUN                 0
#define PUBLIC_BUS              0

#define NUM_RETRIES             4

/* Structures */
#ifdef _EFI
#pragma pack (push, 1)
#endif
#if (defined(MSDOS) || defined(WINDOWS))
#pragma pack(1)
#endif

typedef enum {
    ACCESN_OK,
    ACCESN_ERROR,
    ACCESN_OUT_OF_RANGE,
    ACCESN_END_OF_DATA,
    ACCESN_UNSUPPORTED,
    ACCESN_INVALID_TRANSACTION,
    ACCESN_TIMED_OUT
} ACCESN_STATUS;

typedef struct {
    unsigned char   cmdType;    // command
    unsigned char   rsSa;       // slave address
    unsigned char   busType;    // always PUBLIC
    unsigned char   netFn;      // network function
    unsigned char   rsLun;      // lun
    unsigned char * data;       // request data
    int             dataLength; // request data size
} SystemIfcReq_T;

#ifdef _EFI
#pragma pack (pop)
#endif
#if (defined(MSDOS) || defined(WINDOWS))
#pragma pack()
#endif

/* Prototypes */
extern int OpenIfc (void);
extern int CloseIfc (void);
ACCESN_STATUS SendTimedImbpRequest (
    SystemIfcReq_T*  reqPtr,         // request info and data
    int              timeOut,        // how long to wait, in mSec units
    BYTE*            respDataPtr,    // where to put response data
    int*             respDataLen,    // how much response data there is
    BYTE*            completionCode  // request status from dest controller
                                                                        );
extern void SleepMs(WORD ms);
extern int  SetBMCPorts (WORD DataPort, WORD CmdStatusPort);
#endif  // BMC_IFC_H

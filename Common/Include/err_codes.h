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
* Filename: err_codes.h
*
* Author   : Winston <winstonv@amiindia.co.in>
*
******************************************************************/

#ifndef	ERR_CODES_H
#define ERR_CODES_H

#include "Types.h"

#ifndef MSDOS
#include "IPMIDefs.h"
#endif

#define ERR_FILENAME_INVALID	   21
#define ERR_INVALID_OPTION           25
#define ERR_INVALID_ARG		   17
#define DISPLAY_HELP_MESSAGE       25  



#define INIT_ERR_CODE()         (g_ErrCode = 0);
#define SET_ERR_CODE(ERR_CODE)  (g_ErrCode |= ERR_CODE)
#define GET_ERR_CODE()          g_ErrCode
extern DWORD g_ErrCode;

/* BMC ifc API error codes */

#define ERR_IFC_OPEN        0x00000001L
#define ERR_IFC_CLOSE       0x00000002L
#define ERR_IFC_GET_MSG     0x00000003L
#define ERR_IFC_SEND_MSG    0x00000004L
#define ERR_IFC_MSG_HDR     0x00000005L

/* Command validation error codes */
#define ERR_BMC_COMM        0x00000010L
#define ERR_COMP_CODE       0x00000020L
#define ERR_RES_LEN         0x00000030L
#define ERR_RES_DATA        0x00000040L

/* File operation error codes */

#define ERR_OPEN_FILE       0x00000100L
#define ERR_SIZE_FILE       0x00000200L
#define ERR_READ_FILE       0x00000300L
#define ERR_WRITE_FILE      0x00000400L


#endif	// ERR_CODES_H

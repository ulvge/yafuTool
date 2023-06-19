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
* Filename: utils.h
*
* Author   : Winston <winstonv@amiindia.co.in>
*
******************************************************************/


#ifndef __UTILS_H__
#define __UTILS_H__

#if defined (__x86_64__) || defined (WIN64)
extern unsigned int CalculateChksum (char *data, unsigned int size);
#else
extern unsigned long CalculateChksum (char *data, unsigned long size);
#endif

extern int GetReleaseaandCodeBaseVersion(char *Firmware_Module,INT32U ModuleSize,char *string_to_search, char *string_to_copy);
extern int FrameIfcReqHdr( BYTE command, BYTE* ReqBuf, int ReqLen);
#endif /* __UTILS_H__ */


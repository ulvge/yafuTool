/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2009, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
* 
* Filename: Ipm_Ifc.c
*
* Descriptions: contains API to access MS IPMI Driver
*
* Author: Rajaganesh R <rajaganeshr@amiindia.co.in>
*         Ramamoorthy Venkatesh  <ramamoorthyv@ami.com>
*
******************************************************************/
//#ifndef IPM_IFC_H
#define IPM_IFC_H
#include    "bmc_ifc.h"

#if ( defined(WIN32) || defined (WIN64))

extern  void ReleaseIPMVars(void);
extern  int IPMOpenIfc(void);
extern  int IPMCloseIfc (void);
extern  int IPMRequestResp(SystemIfcReq_T* reqPtr, int timeOut, BYTE* respDataPtr, int* respDataLen, BYTE* completionCode);
#endif

//#endif  // IPM_IFC_H


/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2015, American Megatrends Inc.         ***
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
* Filename: meupdate.h
*
* Author   : Austinlin <AustinLin@ami.com.tw>
*
******************************************************************/

#ifndef _MEUPDATE_H_
#define _MEUPDATE_H_

#ifdef MSDOS
//return status of CompareMeVersion command
#define NORMALME           0X00
//Current ME status
#define FAIL_NOT_SUPPORTED 0X01
#define FAIL_UNKNOW_ME_VER 0X02
#define FAIL_GET_ME_VER    0X03
//Uploaded ME status
#define SAME_ME_VER        0X04
#define OLDER_ME_VER       0X05
#define INVALID_ME_IMG     0X06
#endif

#define MAX_SIZE_TO_READ_ME  0x4000

extern int DONMMEUpdate;
extern int StartDoMEUpdate(IPMI20_SESSION_T *hSession);

/*Get ME version comparison status*/
extern INT8U ME_Current_Status;
extern INT8U ME_Comparison_Status;


#endif //_MEUPDATE_H_

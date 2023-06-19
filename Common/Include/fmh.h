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
* Filename: fmh.h
*
* Author   : Winston <winstonv@amiindia.co.in>
*
******************************************************************/

#ifndef FMH_H
#define FMH_H

#define FMH_SIGNATURE		"$MODULE$"
#define FMH_SIGNATURE_SIZE		8
#define FMH_END_SIGNATURE 	0x55AA
#define FMH_SIZE			64
#define FMH_MAJOR			1
#define FMH_MINOR		 	7

#define CONFIG_SECTION_NAME		"params"
#define BOOT_SECTION_NAME		"boot"


/*  Module Types */
#define MODULE_UNKNOWN		0x00	/* Unknown Module Type  */
#define MODULE_BOOTLOADER	0x01	/* Boot Loader 	        */	
#define MODULE_FMH_FIRMWARE	0x02  	/* Info about  firmware */
#define MODULE_KERNEL		0x03	/* OS Kernel 	        */


#ifdef MSDOS 
/* CRC32 Related */ 
#if defined (__x86_64__) || defined (WIN64)
unsigned int CalculateCRC32(unsigned char *Buffer, unsigned int Size);
void BeginCRC32(unsigned int *crc32);
void DoCRC32(unsigned int *crc32, unsigned char Data);
void EndCRC32(unsigned int *crc32);
#else
unsigned long CalculateCRC32(unsigned char *Buffer, unsigned long Size);
void BeginCRC32(unsigned long *crc32);
void DoCRC32(unsigned long *crc32, unsigned char Data);
void EndCRC32(unsigned long *crc32);
#endif
#endif
#endif	

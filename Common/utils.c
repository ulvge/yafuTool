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
* Filename: utils.c
*
* Author   : Winston <winstonv@amiindia.co.in>
*
******************************************************************/

#include "main.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "fmh.h"
#ifndef MSDOS
#include "icc_what.h"
#include "IPMIDefs.h"
#include "IPMI_AMIDevice.h"
#endif
#include "flashcmds.h"
#include "bmc_ifc.h"

#ifdef ICC_OS_WINDOWS
#include <malloc.h>
#endif

#ifdef MSDOS
#include "crc32.h"
#endif

#define FMH_MODULE_CHECKSUM_START_OFFSET	0x32
#define FMH_MODULE_CHECKSUM_END_OFFSET		0x35
#define FMH_FMH_HEADER_CHECKSUM_OFFSET 		0x17
FlashMH* FwModuleHdr[MAX_MODULE];

extern int SignedImageSup;
extern SystemIfcReq_T IfcReqHdr;
extern char BootValues[MAX_UBOOT_VAR_COUNT][MAX_UBOOT_VAR_LENGTH];

#ifdef MSDOS  
#if defined (__x86_64__) || defined (WIN64)
void BeginCRC32(unsigned int *crc32)
#else
void BeginCRC32(unsigned long *crc32)
#endif
{        
    *crc32 = 0xFFFFFFFF;
    return;	
}

#if defined (__x86_64__) || defined (WIN64)
void DoCRC32(unsigned int *crc32, unsigned char Data)
#else
void DoCRC32(unsigned long *crc32, unsigned char Data)
#endif
{
    *crc32=((*crc32) >> 8) ^ CrcLookUpTable[Data ^ ((*crc32) & 0x000000FF)];
    return;
}

#if defined (__x86_64__) || defined (WIN64)
void EndCRC32(unsigned int *crc32)
#else
void EndCRC32(unsigned long *crc32)
#endif
{
    *crc32 = ~(*crc32);
    return;
}
#endif

#if defined (__x86_64__) || defined (WIN64)
unsigned int CalculateChksum (char *data, unsigned int size)
{
    unsigned int crc32val = 0;
#else
unsigned long CalculateChksum (char *data, unsigned long size)
{
    unsigned long crc32val = 0;
#endif
    unsigned int i = 0;

    BeginCRC32(&crc32val);

    //  Begin calculating CRC32 
    for(i = 0;i < size;i++)
    { 
        DoCRC32(&crc32val, data[i]);
    }

    EndCRC32(&crc32val);
    return crc32val;
}
/*
 * @fn VerfyImageChecksum
 * @brief This Function verifies Checksum of Image File
 * Return 0 on Success
*/
#if defined (__x86_64__) || defined (WIN64)
int VerifyImageChecksum(unsigned int ImageSize,char* Buffer,int ModuleCount)
#else
int VerifyImageChecksum(unsigned long ImageSize,char* Buffer,int ModuleCount)
#endif
{
    int count,FwModFound = 0;
    INT32U ChkVal,i;
#if defined (__x86_64__) || defined (WIN64)
    unsigned int crc32,StartImgAddr;
#else
    unsigned long crc32,StartImgAddr;
#endif

    if(ModuleCount == 0)
    {
	return -1;
    }

    for(count=0;count<ModuleCount;count++)
    {
        if(FwModuleHdr[count]->ModuleType == MODULE_FMH_FIRMWARE)
        {
            ChkVal=FwModuleHdr[count]->ModuleChecksum;
            StartImgAddr=FwModuleHdr[count]->FmhLocation;
            FwModFound =1;
            //break;
        }
    }
    if(FwModFound != 1)
    {
        printf("WARNING!! Can not find the Firmware Module\nInvalid Image File!!\n");
        exit(YAFU_FW_MOD_NOT_FOUND);
    }
    BeginCRC32(&crc32);

    for(i=0;i<ImageSize;i++)
    {
        if((i>=(StartImgAddr+FMH_MODULE_CHECKSUM_START_OFFSET)&& i <=(StartImgAddr+FMH_MODULE_CHECKSUM_END_OFFSET))
            || (i==(StartImgAddr+FMH_FMH_HEADER_CHECKSUM_OFFSET)))
        {
            continue;
        }

        DoCRC32(&crc32,Buffer[i]);
    }

    EndCRC32(&crc32);

    if(ChkVal!=crc32)
    {
        return -1;
    }

    return 0;
}


#ifdef ICC_OS_WINDOWS
#include <conio.h>
int GetPasswordInput(char* pwd)
{
 int  l=0;
 char c;

 printf("Password  : ");
 fflush(stdout);

 for ( ;; )
 {
   c = (char)getch();
   switch ( c )
   {
     case '\b':  // Backspace
       if ( l>0 )
       {
         putchar('\b');
         putchar(' ');
         putchar('\b');
         l--;
       }
       break;
     case '\r':  // Return (Enter)
       pwd[l] = '\0';
       return 0;
     default:
       putchar('*');
       if ( l<256 )
         pwd[l++] = (char)c;
       break;
   }

 }
    return 0;
}
#endif  /*  ICC_OS_WINDOWS  */

int FrameIfcReqHdr( BYTE command, BYTE* ReqBuf, int ReqLen)
{
         IfcReqHdr.cmdType  = command;
        IfcReqHdr.rsSa        = BMC_SLAVE_ADDR;
        IfcReqHdr.rsLun       = BMC_LUN;
        IfcReqHdr.netFn       = OemNetFn;
        IfcReqHdr.busType     = PUBLIC_BUS;
        IfcReqHdr.data             =         ReqBuf;
        IfcReqHdr.dataLength  = ReqLen;

	return 0;
}


#ifdef ICC_OS_LINUX
int GetPasswordInput(char* pwd)
{
    strcpy(pwd,(const char *) getpass("Password  : "));
    return 0;
}
#endif

int GetReleaseaandCodeBaseVersion(char *Firmware_Module,INT32U ModuleSize,char *string_to_search, char *string_to_copy)
{ 
	char *stringsep,*nextsepstring,*FirmwareInfo; 
	int len=0,i=0,stringlen=0; 

       if((Firmware_Module == NULL) || (string_to_search == NULL) || (string_to_copy == NULL))
       {
           return -1;
       }

	FirmwareInfo = malloc(ModuleSize); 
	if(FirmwareInfo == NULL) 
	{ 
		return -1; 
	} 

	memset(string_to_copy,0,MAX_MODULE_NAME);

	memcpy(FirmwareInfo,Firmware_Module,ModuleSize); 
	
	stringsep = strstr(FirmwareInfo,string_to_search);
	if(stringsep == NULL) 
	{ 
		free(FirmwareInfo); 
		return -1; 
	} 

	nextsepstring = strtok(stringsep,"\n"); 
	if(nextsepstring == NULL) 
	{ 
		free(FirmwareInfo); 
		return -1; 
	} 

	len = strlen(nextsepstring); 
	while(len) 
	{ 
		if(stringsep[i] == '=') 
		{ 
			i++; 
			stringlen = strlen(&nextsepstring[i]); 
			strncpy(string_to_copy,&nextsepstring[i],stringlen); 
			break; 
		} 
		else 
		{ 
			i++; 
			len--; 
		} 
	} 
	free(FirmwareInfo); 
return 0; 
} 

/*
 * s1 is either a simple 'name', or a 'name=value' pair.
 * s2 is a 'name=value' pair.
 * If the names match, return the value of s2, else NULL.
 */
u8 *envmatch (u8 * s1, u8 * s2)
{

    while (*s1 == *s2++)
        if (*s1++ == '=')
            return (s2);
    if (*s1 == '\0' && *(s2 - 1) == '=')
        return (s2);
    return (NULL);
}

/*
 * Deletes or sets environment variables.
 * 0    - OK
 * 1    - Error
 */
int fw_setenv (env_t *environment,char *name, char *value)
{
    int len = 0;
    u8 *env = NULL, *nxt = NULL;
    u8 *oldval = NULL;

    /*
     * search if variable with this name already exists
     */
    for (nxt = env = environment->data; *env; env = nxt + 1) {
        for (nxt = env; *nxt; ++nxt) {
            if (nxt >= &environment->data[environment->size]) {
                fprintf (stderr, "## Error: "
                    "environment not terminated\n");
                return 1;
            }
        }
        if ((oldval = envmatch ((unsigned char *)name, env)) != NULL)
        {
            break;
        }
    }
    /*
     * Delete any existing definition
     */
    if (oldval) {
        if (*++nxt == '\0') {
            *env = '\0';
        } else {
            for (;;) {
                *env = *nxt++;
                if ((*env == '\0') && (*nxt == '\0'))
                    break;
                ++env;
            }
        }
        *++env = '\0';
    }

    /* Delete only ? */
    if ((value == NULL) || (*value == '\0'))
    {
        goto done_setenv;
    }

    /*
     * Append new definition at the end
     */
    for (env = environment->data; *env || *(env + 1); ++env)
    {
      if(*env == 0xff)
      {
          break;
      }
    };
    if (env > environment->data)
    {
        ++env;
    }
    /*
     * Overflow when:
     * "name" + "=" + "val" +"\0\0"  > CFG_ENV_SIZE - (env-environment)
     */
    len = strlen (name) + 2 + strlen (value);

    if (len > (&environment->data[environment->size] - env)) 
    {
        fprintf (stderr,
            "Error: environment overflow, \"%s\" deleted\n",
            name);
        return 1;
    }
    while ((*env = *name++) != '\0')
        env++;
    *env = '=';
    while ((*++env = *value++) != '\0');

    /* end is marked with double '\0' */
    *++env = '\0';
    /* Fill All the remaining space with NULL*/
    while(&environment->data[environment->size] - env)
    {
        *++env = '\0';
    }

  done_setenv:
    /* Update CRC */
    environment->crc = CalculateChksum(environment->data, environment->size);

    return (0);
}

int RegenerateUBootVars(env_t *env,char *BootVarsName,INT16U BootVarCnt,unsigned int EnvSize)
{
#ifndef MSDOS
    FILE *fp = NULL;
#endif

    char *BootVal;
    char *BootVar;
    char valarr[MAX_BOOTVAL_LENGTH]={0};  
    char vararr[MAX_BOOTVAR_LENGTH]={0}; 
    int Count =0,TotalLen=0,len=0;
    TotalLen =0;
		
    BootVar = vararr;

    printf("Regenerating Boot Variables\n");

    memset(BootVar,0, MAX_BOOTVAR_LENGTH);
    BootVarsName += sizeof(AMIYAFUGetBootVarsRes_T);

    env->size=EnvSize-8;

    env->data=malloc(EnvSize);
    if(env->data == NULL)
    {
	printf("Cannot Allocate Memory to read env variables from Image File\n");
	perror("");
	fflush(stdout);
	return -1;
    }

    memset(env->data,0,EnvSize);

    for(Count = 0; Count < BootVarCnt; Count++)
    {
	  strcpy(BootVar,(BootVarsName+ TotalLen));
#if defined (__x86_64__) || defined (_WIN64)
        len = (unsigned)strlen(BootVar);
#else
        len = strlen(BootVar);
#endif
        TotalLen += (len+1);

        BootVal = valarr;
        memset(BootVal, 0, MAX_BOOTVAL_LENGTH);
        memcpy(BootVal,BootValues[Count],(strlen(BootValues[Count])+1));

	 if(fw_setenv(env,BootVar,BootVal) != 0)
        {
            printf ("\nError: SetBootConfig failed\n"); 
	     fflush (stdout);
            return -1;
        }
    }

#ifndef MSDOS
    fp = fopen("/var/boot.bin","wb");
    fwrite((char *)&env->crc,1,sizeof(env->crc),fp);
    fwrite((char *)env->data,1,EnvSize-8,fp);
    fclose(fp);
#endif

    return 0;
}


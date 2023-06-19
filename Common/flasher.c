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
* Filename: flasher.c
*
* Author   : Winston <winstonv@amiindia.co.in>
*
******************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "fmh.h"
#include "flashcmds.h"
#include "main.h"
#include "utils.h"

#ifndef MSDOS
#include "icc_what.h"
#include "libipmi_session.h"
#include "libipmi_errorcodes.h"
#include "IPMI_AMIDevice.h"
#include "libipmi_struct.h"
#include "libipmi_AMIOEM.h"
#else
#include "flashlib.h"
#endif

extern INT8U byMedium ;
extern FEATURE_LIST featuresList;
extern env_t env;
extern int ImgOpt;
extern int FlashBothImg;
extern int ImgCount;
extern int NoOfImg;
#define ENABLEFLASH 1
extern FlashMH* FwModuleHdr[MAX_MODULE];
extern FlashMH*	CurrFwModHdr[MAX_MODULE];
extern FlashMH* DualFwModHdr[MAX_MODULE];
extern FlashMH* PrimFwModHdr[MAX_MODULE];
#if defined (__x86_64__) || defined (WIN64)
extern unsigned int AddofAllocMem;
extern unsigned int ActualCopySize;
extern unsigned int ActualSizetoCopy;
extern unsigned int ActualConfig;
unsigned int MemOffset = 0;
extern int Mode;
extern unsigned int NewImageSize;
extern unsigned int oldVersion;
#else
extern unsigned long AddofAllocMem;
extern unsigned long ActualCopySize;
extern unsigned long ActualSizetoCopy;
extern unsigned long ActualConfig;
unsigned long MemOffset = 0;
extern int Mode;
extern unsigned long NewImageSize;
extern unsigned long oldVersion;
#endif
extern INT32U ConfigFirmAddr,ConfigRomAddr,ConfigRomAllocAddr,ConfigFirmAllocAddr;
extern INT32U BootFirmAddr,BootRomAddr,BootRomAllocAddr,BootFirmAllocAddr;
extern INT32U ExtlogFirmAddr,ExtlogFirmAllocAddr,ExtlogRomAddr;
extern INT32U FreeSpaceFirmAddr,FreeSpaceFirmSize,FreeSpaceRomAddr;
extern INT32U BootImgFirmAddr,BootImgRomAddr;
extern int  RecoveryFlashMode;
extern INT32U BootVarsCount;
extern INT32U EraseBlkSize;
extern UPDATE_INFO Parsing;
extern char bvnArr[MAX_UBOOT_VAR_COUNT * MAX_UBOOT_VAR_NAME_LENGTH]; // memory for BootVarsName
extern int DualImageSup,SignedImageSup;
extern int ImgCount,NoOfImg;
extern int ModCount,ModuleCount,FlashFMH;
extern int CommonConf;
extern int SkipFreeArea;
extern int SPIDevice;
extern int DOMMCUpdate;
extern int IsRunLevelON;	 	 
extern INT8U PreserveFlag;

INT8U BootFlag=0xFE, ConfigFlag=0xFD,ExtlogFlag = 0xFB;
char bufArr [MAX_SIZE_TO_READ]; // memory for buf
int firsttime = 1;
/*
 * @fn WarningMessage
 * @brief Display Warning Message
 * @return Returns 0 on success
*/
int WarningMessage()
{ 
    printf("\n****************************************************************************\n");
    printf(" WARNING!\n");
    printf("        FIRMWARE UPGRADE MUST NOT BE INTERRUPTED ONCE IT IS STARTED.\n");
    printf("        PLEASE DO NOT USE THIS FLASH TOOL FROM THE REDIRECTION CONSOLE.\n");
    printf("****************************************************************************\n");
    return 0;   
}

/*
 * @fn MemoryDealloc
 * @brief this function Deallocate the memory in RAM
 * @param hSession - Current Session Pointer
 * @param Offset - Memory to Deallocate
 * @return Returns 0 on success
*/
int MemoryDealloc(IPMI20_SESSION_T *hSession,INT32U Offset)
{
    if(FreeMemory(hSession,Offset) != 0)
    {
             printf("Error in Free Memory  %x\n",Offset); 
         return -1;
    }      
    return 0;
}
/*
 * @fn FlashAndVerify
 * @brief this function Flash and verify the image form RAM to Flash Area
 * @param hSession - Current Session Pointer
 * @param WriteMemOff - Memory Offset in RAM
 * @param FlashOffset - Flash Offset in the Flash Area of Firmware
 * @param Sizetocpy - Size to copy from RAM to Flash Area of Firmware
 * @param PreserveFlag - Preserving module
 * @return Returns 0 on success
*/
#if defined (__x86_64__) || defined (WIN64)
int FlashAndVerify (IPMI20_SESSION_T *hSession,unsigned int WriteMemOff,unsigned int FlashOffset,unsigned int Sizetocpy,INT8U PreserveFlag)
#else
int  FlashAndVerify (IPMI20_SESSION_T *hSession,unsigned long WriteMemOff,unsigned long FlashOffset,unsigned long Sizetocpy,INT8U PreserveFlag)
#endif
{
     AMIYAFUMiscellaneousRes_T AMIYAFUMiscellaneousRes;
     if(SilentFlash == 0x01 && Progressdis == 1 && Parsing.versioncmpflash !=1)
     {
         printf("\nFlashing  Firmware Image...");
         fflush(stdout);
     }

    if( !RecoveryFlashMode )	
    {
   	 if(SendMiscellaneousInfo(hSession,PreserveFlag, &AMIYAFUMiscellaneousRes) != 0)
    	 {
         	if (vdbg)
         		printf("Error in SendMiscellaneousInfo \n");
        	//return -1;
    	 }
    }
	
    if(EraseAndFlash(hSession,WriteMemOff,FlashOffset,Sizetocpy) != 0)
    {
        if (vdbg)
            printf("Error in EraseAndFlashing \n");
       return -1;
    }
     if(RecoveryFlashMode == 0x00)
    {

      if((Parsing.Full == 0x01) || (BIOS_FLASH == SPIDevice) || (CPLD_FLASH == SPIDevice))
	    {
	        Progressdis=1;
	    }
	    else
	    {
	        Progressdis=0;
	    }

	    if(oldVersion != 0x01)
	    {
	        if(ECFStatus(hSession)==-1)
	        {
                  printf("Error in ECFStatus \n");
	              return -1;
	        }
	    }
	    if(SilentFlash == 0x01 && Progressdis == 1 && Parsing.versioncmpflash !=1)
       {    
           printf("\rFlashing  Firmware Image ... Done\n");
           printf("Verifying  Firmware Image...");
           fflush(stdout);
	   } 
	    if(VerifyFlash(hSession,WriteMemOff,FlashOffset,Sizetocpy) != 0)
	   {
	        if (vdbg)
	           printf("Error in Verify Flash \n");
	       return -1;
	   }
	
	    if(oldVersion!=0x01)
	    {
	        if(VerifyStatus(hSession)==-1)
	        {
                 printf("Error in Verify VerifyStatus \n");
	             return -1;
	        }
	    }
	    if(Parsing.Silent == 0x01 && Progressdis == 1)
        {    
            printf("\rVerifying  Firmware Image ... Done\n");
	    }
     }
    return 0;
}
/*
 * @fn FlashFirmware
 * @brief this function starts the Flash in Firmware
 * @param hSession - Current Session Pointer
 * @param img - Current Image file
 * @param SizeToFlash - Size of block to flash
 * @param UpgradedBytes - Bytes upgraded in Flash
 * @param seekpos - Seek the Postion in the image file
 * @return Returns 0 on success
*/
int FlashFirmware(IPMI20_SESSION_T *hSession,FILE *img,INT32U SizeToFlash,INT32U UpgradedBytes,INT32U seekpos)
{
#if defined (__x86_64__) || defined (WIN64)
    unsigned int SizetoAlloc =0,WriteMemOff = 0,SizeToRead = 0,MemOffsetLoc = 0;
    unsigned int FlashOffset = 0,MemOffLoc = 0,FlashOffLoc = 0;
    unsigned int SizetoCpy = 0,Size =0;
#else
    unsigned long SizetoAlloc    = 0,WriteMemOff = 0,SizeToRead = 0,MemOffsetLoc =0;
    unsigned long FlashOffset    = 0,MemOffLoc = 0,FlashOffLoc = 0;
    unsigned long SizetoCpy  = 0,Size =0;
#endif
    AMIYAFUMiscellaneousRes_T AMIYAFUMiscellaneousRes;
    int ModuleFound=0,Config=0,Extlog=0,BootPreserve=0, ConfigPreserve=0, ExtlogPreserve=0; 
    INT32U Offset=0;
    INT16U Datalen = 0;
    char *Buf = 0;
    INT8U Count = 0,ReadConf = 0,ReadBkupConf= 0;
    int RetVal = 0, i = 0, j = 0;
    int Percentage= 100;
    char ModName[MAX_NAME_LEN+MAX_BKUP_LEN] = {0};
    char CurrFwModName[MAX_NAME_LEN+1] = {0};
    char FwModuleName[MAX_NAME_LEN+1] = {0};
    SizetoAlloc = SizeToFlash;
    ActualConfig=0;
    if(Parsing.Full == 0x02)
        firsttime=1;
    if(firsttime) 
    {
        if(RecoveryFlashMode == 0x00)
        {
            if(MemoryAllocation(hSession,SizetoAlloc) != 0 || AddofAllocMem == 0xffffffff )
            {
                fprintf(stderr,"Error in Memory Allocation  %x\n", (unsigned int)(SizetoAlloc));
                return -1;
            }
            if( AddofAllocMem == 0xfffffffe )
            {
                return 1;
            }
            if(Parsing.Full== 0x01)
            {
               if(Parsing.Silent == 0x01)
               {          
                   printf("Uploading Firmware Image ..."); 
                   SilentFlash = 0x01;
               }
               fflush(stdout);
           }
            MemOffset = AddofAllocMem;
        }
    
        Offset = UpgradedBytes;
        while(Offset < (UpgradedBytes + SizeToFlash))
        {
            AddofAllocMem += SizeToRead;
    
            if((Offset + MAX_SIZE_TO_READ ) > (Offset + SizeToFlash))
            {
                SizeToRead = (Offset + SizeToFlash) - Offset;
            }
            else if((SignedImageSup == 1) && ((Offset +MAX_SIZE_TO_READ) > SizeToFlash))
            {
                if(RecoveryFlashMode == 0x01)
                    SizeToRead = MAX_SIZE_TO_READ;
            else
                SizeToRead = SizeToFlash-Offset; 
            }
            else
            {
                SizeToRead = MAX_SIZE_TO_READ;
                if((SPIDevice == CPLD_FLASH) && (Offset +MAX_SIZE_TO_READ) > SizeToFlash){
                    SizeToRead = SizeToFlash-Offset;
                } 
            }
    
            Buf=bufArr;
            memset(Buf, 0, SizeToRead);
            if(fseek (img, seekpos, SEEK_SET) !=0)
                printf("Error in fseek\n");
            if(fread(Buf,1,SizeToRead,img) != SizeToRead)
                printf("Error in fread \n");
    
            WriteMemOff = AddofAllocMem;
            Datalen = (INT16U)SizeToRead;
    
            if(RecoveryFlashMode == 0x01)
            {
                WriteMemOff = Offset;
            }
            
            if(WritetoMemory(hSession,WriteMemOff,Datalen,Buf) != 0)
            {
                if(RecoveryFlashMode == 0x00)
                    MemoryDealloc(hSession,MemOffset);
                return -1;
            }
            if( Parsing.Silent == 0x00)
            {
                if(Parsing.Full == 0x01)
                {
                    printf("\rUploading Firmware Image : %d%%\r",(int)((Offset*100)/NewImageSize));
                }
                else
                {
                    printf("\rUpgrading Firmware Image : %d%%\r",(int)((Offset*100)/NewImageSize));
                }
                fflush(stdout);
            }
            Offset += SizeToRead;
            seekpos +=SizeToRead;
            i +=1;
        }
    }
    if(FlashBothImg != TRUE) 
        firsttime = 0;
    if(RecoveryFlashMode == 0x01)
    {
        WriteMemOff = UpgradedBytes;
    }
    else
    {
        WriteMemOff = MemOffset;
    }

    FlashOffset = UpgradedBytes;
    SizetoCpy = SizeToFlash;

    if(Parsing.Full == 0x01)
    {
        if(Parsing.Silent == 0x00)
            printf("Uploading Firmware Image : %d%%... done\n",(int)(Percentage));
        else
            printf("\rUploading Firmware Image ...  Done");

        for (i=0; i<ModuleCount; i++)
        {
            ModuleFound = 0x00;
            for (j=0; j<FlashFMH; j++ )
            {
                strncpy(FwModuleName,FwModuleHdr[i]->ModuleName,MAX_NAME_LEN);
                strncpy(CurrFwModName,CurrFwModHdr[j]->ModuleName,MAX_NAME_LEN);
                if(strncmp(FwModuleName,CurrFwModName,MAX_NAME_LEN) == 0)
                {
                    ModuleFound = 0x01;
                    if((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"boot") == 0) && Parsing.BootPreserve == 1)
                    {
                        BootPreserve++;
                    }
                    else if(spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"conf") == 0)
                    {
                        Config++;
                        
                        if(Parsing.ConfigPreserve == 1)
                        {
                                ConfigPreserve++;
                        }
                    }
                    else if((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"extlog") == 0) && Parsing.ExtlogPreserve == 1)
                    {
                        Extlog++;

                        if(Parsing.ExtlogPreserve == 1)
                            ExtlogPreserve++;
                    }
                    break;
                }
            }

            /* If it is split image, module count may differ and module should be matched in both current firmware and existing firmware */
            if(ModuleFound != 0x01 && Parsing.SplitImg == 0x01 && SplitFlashOffset != 0)
            {
                continue;
            }

            /* If preserve flag is enabled and module is not duplicated ( count of individual section is not greater than 1 ) then preserve that module*/
            if((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"boot") == 0) && BootPreserve == 1)
            {
                printf("Skipping [%s] Module ....\n",(char *)FwModuleHdr[i]->ModuleName);
                if((ImgOpt == -1) && (Parsing.ConfigPreserve == 1))
                {
                        for(Count=0;Count<ModuleCount;Count++)
                        {
                                if(spstrcasecmp((char *)FwModuleHdr[Count]->ModuleName,"conf") == 0)
                                {
                                        if(ReadConf == 0)
                                        {
                                                ReadConf++;
                                        }
                                        else if(ReadBkupConf == 0)
                                        {
                                                ReadBkupConf++;
                                                break;
                                        }
                                }
                        }

                        if(ReadConf == 1)
                        {
                                ReadBackupConfnBkconf(hSession,1);
                        }
                         if(ReadBkupConf == 1)
                        {
                                ReadBackupConfnBkconf(hSession,2);
                        }
                }
                continue;
            }
            else if((spstrcasecmp(FwModuleName,"boot_img") == 0) && BootPreserve == 1)
            {
                printf("Skipping [%s] Module ....\n",(char *)FwModuleHdr[i]->ModuleName);
                continue;
            }
            else if((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"conf") == 0) && (ConfigPreserve == 1) && (ImgOpt  != -1))
            {
                printf("Skipping [%s] Module ....\n",(char *)FwModuleHdr[i]->ModuleName);
                continue;
            }
            else if((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"extlog") == 0) && ExtlogPreserve == 1)
            {
                printf("Skipping [%s] Module ....\n",(char *)FwModuleHdr[i]->ModuleName);
                continue;
            }

            Size = FwModuleHdr[i]->AllocatedSize;
            MemOffLoc = FlashOffLoc = FwModuleHdr[i]->FmhLocation;
            if(spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"boot") == 0)
            {
                MemOffLoc = 0;
                FlashOffLoc = 0;
                                
                if((ImgOpt == -1) && (Parsing.ConfigPreserve == 1))
                {
                        for(Count=0;Count<ModuleCount;Count++)
                        {
                                if(spstrcasecmp((char *)FwModuleHdr[Count]->ModuleName,"conf") == 0)
                                {
                                        if(ReadConf == 0)
                                        {
                                                ReadConf++;
                                        }
                                        else if(ReadBkupConf == 0)
                                        {
                                                ReadBkupConf++;
                                                break;
                                        }
                                }
                        }

                        if(ReadConf == 1)
                        {
                                ReadBackupConfnBkconf(hSession,1);
                        }
                         if(ReadBkupConf == 1)
                        {
                                ReadBackupConfnBkconf(hSession,2);
                        }
                }
            }
            /*In split image case current firmware FMH location and module count will get differ from existing firmware
              so taking flashoffset from existing firmware and Memoryoffset from current firmware*/
            else if((Parsing.SplitImg == 0x01) && (SplitFlashOffset != 0) && (spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"boot") != 0))
            {
                MemOffLoc = FwModuleHdr[i]->FmhLocation;
                FlashOffLoc = CurrFwModHdr[j]->FmhLocation;
                Size = CurrFwModHdr[j]->AllocatedSize;
            }

            if((spstrcasecmp(FwModuleName,"boot") == 0) ||
              (spstrcasecmp(FwModuleName,"boot_img") == 0))
            {
                PreserveFlag &= BootFlag;
            }
            else if(spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"conf") == 0 && Parsing.ConfigPreserveBitMask != 0)
            {
                PreserveFlag &= ConfigFlag;
                PreserveFlag |= BIT0;
            }
            else
            {
                PreserveFlag |= BIT0;
            }

            #if defined (CONFIG_SPX_FEATURE_YAFUFLASH_SKIP_FLASHING_FREE_AREA)
                if(SkipFreeArea == 1)
                {
                    /* If it is a last section then flash free area in SPI along with last section */
                    if(i+1  == ModuleCount && (Parsing.SplitImg == 0x00 || (Parsing.SplitImg == 0x01 && SplitFlashOffset == 0x00)))
                    {
                        MemOffLoc = FlashOffLoc = FwModuleHdr[i-1]->FmhLocation + FwModuleHdr[i-1]->AllocatedSize;
                        Size = NewImageSize - FlashOffLoc;
                    }
                }
            #else
                    /* If it is a last section then flash free area in SPI along with last section */
                    if(i+1  == ModuleCount && (Parsing.SplitImg == 0x00 || (Parsing.SplitImg == 0x01 && SplitFlashOffset == 0x00)))
                    {
                        MemOffLoc = FlashOffLoc = FwModuleHdr[i-1]->FmhLocation + FwModuleHdr[i-1]->AllocatedSize;
                        Size = NewImageSize - FlashOffLoc;
                    }
            #endif

            MemOffsetLoc=WriteMemOff+MemOffLoc;
            FlashOffset=FlashOffLoc;
            SizetoCpy=Size;
            ActualSizetoCopy=SizetoCpy;
            Progressdis=1;
            ModuleType = FwModuleHdr[i]->ModuleType;            

            /* If duplicate conf is enabled in new MAP then bkcupconf will have duplicate of conf section. In order to avoid the confusion second conf module will be show cased as bkupconf */
            strncpy(ModName,FwModuleHdr[i]->ModuleName,MAX_NAME_LEN);
            if((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"conf") == 0) && Config > 1)
                spsnprintf(ModName,strlen((char *)FwModuleHdr[i]->ModuleName)+MAX_BKUP_LEN,"bkup%s",(char *)FwModuleHdr[i]->ModuleName);
            else if((spstrcasecmp((char *)FwModuleHdr[i]->ModuleName,"extlog") == 0) && Extlog > 1)
                spsnprintf(ModName,strlen((char *)FwModuleHdr[i]->ModuleName)+MAX_BKUP_LEN,"bkup%s",(char *)FwModuleHdr[i]->ModuleName);

            printf("Flashing [%s] Module ....\n",ModName);
            if( !RecoveryFlashMode ) 
            {
            	if(SendMiscellaneousInfo(hSession,PreserveFlag,&AMIYAFUMiscellaneousRes) != 0)
		{
                	if (vdbg)
                		printf("Error in SendMiscellaneousInfo \n");
                	//return -1;
		}
            }
            if(EraseAndFlash(hSession,MemOffsetLoc,FlashOffset,SizetoCpy) != 0)
            {
                printf("Error in EraseAndFlashing \n");
                return -1;
            }

            if(RecoveryFlashMode == 0x00)
            {
                if(oldVersion!=0x01)
                {
                    RetVal = ECFStatus(hSession);
                    if(RetVal == 2)
                    {
                        goto exit;
                    }
                    else if(RetVal == -1)
                    {
                        return -1;
                    }
                }

                if(VerifyFlash(hSession,MemOffsetLoc,FlashOffset,SizetoCpy) != 0)
                {
                    printf("Error in Verify Flash \n");
                    return -1;
                }
                if(oldVersion!=0x01)
                {
                    if(VerifyStatus(hSession)==-1)
                    {
                        printf("Error in Verify \n");
                        return -1;
                    }
                }
            }
        } 
    }
    else
    {
        if ((BIOS_FLASH == SPIDevice) || (CPLD_FLASH == SPIDevice))
        {
            printf("Uploading Image : %d%%... done\n",Percentage);
        }
    
        if(FlashAndVerify(hSession,WriteMemOff,FlashOffset,SizetoCpy,PreserveFlag) != 0)
        {
            if(RecoveryFlashMode == 0x00)
            {
                MemoryDealloc(hSession,MemOffset);
            }
            return -1;
       }
    }

exit:	
    if(!firsttime || Parsing.Full == 0x02) 
    {
        if(RecoveryFlashMode == 0x00)
        {
            if(MemoryDealloc(hSession,MemOffset) != 0)
            {
                return -1;
            }
        }
    }
    
    if((FlashBothImg == TRUE) && (NoOfImg != ImgCount))
        firsttime = 0;

    return 0;

}

/*
 * @fn VersionCmpFlash
 * @brief this function compares firmware module versions
 * @param ModNo - Holds the module number which is going to be flashed.
 * @param Modcnt - total number of modules which is having different module
                                 version number
*/
int 
VersionCmpFlash (int *ModNo, int *Modcnt)
{
    int i =0,j=0, k=0, FullFirmwareup =0;
    int Found = 0;
    char FwModuleName[MAX_NAME_LEN+1] = {0};
    char CurrFwModName[MAX_NAME_LEN+1] = {0};
    FlashMH **CurrFwModInfo = NULL;

    if(NoOfImg == IMAGE_1)
    {
        CurrFwModInfo = CurrFwModHdr;
    }
    else if(NoOfImg == IMAGE_2)
    {
        CurrFwModInfo = DualFwModHdr;
    }

	if(1 == Parsing.SplitImg)
	{
		for(i=0;i<ModuleCount;i++)
		{
	        for (j=0;j<ModCount;j++)
	        {
	            Found = 0;
                strncpy(FwModuleName,(char *)FwModuleHdr[i]->ModuleName,MAX_NAME_LEN);
                strncpy(CurrFwModName,(char *)CurrFwModInfo[j]->ModuleName,MAX_NAME_LEN);
	            if (0 == strncmp(FwModuleName, CurrFwModName, MAX_NAME_LEN))
	            {
                    #ifdef CONFIG_SPX_FEATURE_YAFUFLASH_FLASH_UBOOT_ON_ACTIVE_IMAGE
                        if((spstrcasecmp(FwModuleName,"boot") == 0) ||
                           (spstrcasecmp(FwModuleName,"boot_img") == 0))
                        {
                            if ((FwModuleHdr[i]->Module_Version.ModMajorVer != PrimFwModHdr[j]->Module_Version.ModMajorVer) ||
                               (*(&(FwModuleHdr[i]->Module_Version.ModMinorVer)+1)  != *(&(PrimFwModHdr[j]->Module_Version.ModMinorVer)+1)) ||
                               ((FMH_MAJOR>=1 && FMH_MINOR==6) && (strncmp(FwModuleHdr[i]->ModuleAuxVer, PrimFwModHdr[j]->ModuleAuxVer,2)!=0)) ||
                               ((FMH_MAJOR>=1 && FMH_MINOR>=7) && (strncmp(FwModuleHdr[i]->ModuleAuxVer, PrimFwModHdr[j]->ModuleAuxVer,6)!=0)))
                            {
                                *Modcnt+= 1;
                                ModNo[k] = i;
                                k++;
                            }
                            Found = 1;
                        }
                        if(Found == 1)
                            break;
                    #endif
					
	                if ((FwModuleHdr[i]->Module_Version.ModMajorVer != CurrFwModInfo[j]->Module_Version.ModMajorVer) || 
	                (*(&(FwModuleHdr[i]->Module_Version.ModMinorVer)+1)  != *(&(CurrFwModInfo[j]->Module_Version.ModMinorVer)+1)) ||
	                ((FMH_MAJOR>=1 && FMH_MINOR==6) && (strncmp(FwModuleHdr[i]->ModuleAuxVer, CurrFwModInfo[j]->ModuleAuxVer,2)!=0)) ||
					((FMH_MAJOR>=1 && FMH_MINOR>=7) && (strncmp(FwModuleHdr[i]->ModuleAuxVer, CurrFwModInfo[j]->ModuleAuxVer,6)!=0)))
	                {
	                	*Modcnt+= 1;
	                    ModNo[k] = i;
	                    k++;
	                    break;
	                }
	                break;
	            }
	        }
       }
	}
	else
	{
	    for(i=0;i<ModCount;i++)
	    {
	        for (j=0;j<ModCount;j++)
	        {
	            Found = 0;
                strncpy(FwModuleName,(char *)FwModuleHdr[i]->ModuleName,MAX_NAME_LEN);
                strncpy(CurrFwModName,(char *)CurrFwModInfo[j]->ModuleName,MAX_NAME_LEN);
	            if (0 == strncmp(FwModuleName,CurrFwModName,MAX_NAME_LEN))
	            {
	                if ((FwModuleHdr[i]->FmhLocation != CurrFwModInfo[j]->FmhLocation) || (FwModuleHdr[i]->AllocatedSize != CurrFwModInfo[j]->AllocatedSize))
	                {
	                    printf ("proceeding for full firmware upgrade as module sizes are different\n");
	                    FullFirmwareup = 1;
	                    break;
	                }

                    #ifdef CONFIG_SPX_FEATURE_YAFUFLASH_FLASH_UBOOT_ON_ACTIVE_IMAGE
                        if((spstrcasecmp(FwModuleName,"boot") == 0) ||
                           (spstrcasecmp(FwModuleName,"boot_img") == 0))
                        {
                            if ((FwModuleHdr[i]->Module_Version.ModMajorVer != PrimFwModHdr[j]->Module_Version.ModMajorVer) ||
                               (*(&(FwModuleHdr[i]->Module_Version.ModMinorVer)+1)  != *(&(PrimFwModHdr[j]->Module_Version.ModMinorVer)+1)) ||
                               ((FMH_MAJOR>=1 && FMH_MINOR==6) && (strncmp(FwModuleHdr[i]->ModuleAuxVer, PrimFwModHdr[j]->ModuleAuxVer,2)!=0)) ||
                               ((FMH_MAJOR>=1 && FMH_MINOR>=7) && (strncmp(FwModuleHdr[i]->ModuleAuxVer, PrimFwModHdr[j]->ModuleAuxVer,6)!=0)))
                            {
                                *Modcnt+= 1;
                                ModNo[k] = i;
                                k++;
                            }
                            Found = 1;
                        }
                        if(Found == 1)
                            break;
                    #endif
					
	                if ((FwModuleHdr[i]->Module_Version.ModMajorVer != CurrFwModInfo[j]->Module_Version.ModMajorVer) || 
	                (*(&(FwModuleHdr[i]->Module_Version.ModMinorVer)+1)  != *(&(CurrFwModInfo[j]->Module_Version.ModMinorVer)+1)) ||
	                ((FMH_MAJOR>=1 && FMH_MINOR==6) && (strncmp(FwModuleHdr[i]->ModuleAuxVer, CurrFwModInfo[j]->ModuleAuxVer,2)!=0)) ||
			((FMH_MAJOR>=1 && FMH_MINOR>=7) && (strncmp(FwModuleHdr[i]->ModuleAuxVer, CurrFwModInfo[j]->ModuleAuxVer,6)!=0)))
	                {
	                    *Modcnt+= 1;
	                    ModNo[k] = i;
	                    k++;
	                    break;
	                }
	                break;
	            }
	        }
	        if (j == ModCount)
	        {
	            printf ("Unable to locate the module name \n\n");
	            FullFirmwareup =1;
	        }

	        if (1 == FullFirmwareup)
	        {
	            break;
	        }
	    }
	}
    if (*Modcnt == ModCount)
    {
        FullFirmwareup = 1;
        Parsing.versioncmpflash = 0;
    }
    return FullFirmwareup;
}

/* @fn FullFlashThreadFn
 * @brief this function is 
 * @param hSession - Current Session Pointer
 * @param img - Image file
 * @param Config - Prserve or non-preserver the Config Params
 * @param Boot - Prserve or non-preserver the Boot Params
 * @return Returns 0 on success
*/
 
int FullFlashThreadFn(IPMI20_SESSION_T *hSession,FILE *img,int Config,int Boot)
{

    INT8U Protect =0,Configstatus = 00,Bootstatus = 0x00,BootImgstatus = 0x00,Extlogstatus = 0x00, FreeSpaceStatus = 0x00;
    INT32U Blknum = 0,UpgradedBytes=0,tempUpBytes=0;  
    INT32U SizeToFlash=0,seekpos=0,PreserveBootAddr=0;
    int Percentage= 100;
    int retval=0;
    int i =0, iter = 0;
    int ModNo [MAX_MODULE];
    int Modver = 0;
    INT8U Count = 0,ReadConf = 0,ReadBkupConf = 0;
    #if defined (__x86_64__) || defined (WIN64) 
        unsigned int SizetoCopy = 0, CurrFMHLoc = 0, FMHLocation = 0; 
    #else 
        unsigned long SizetoCopy = 0, CurrFMHLoc = 0, FMHLocation = 0; 
    #endif 
    rewind(img);
    EnvWritten = 0;

    if(GetStatus(hSession) !=0)
    {
        printf("Error in GetStatus \n");
        return -1;
    }
	
    if(Parsing.ExtlogPreserve != 1)
        PreserveFlag &= ExtlogFlag;

    if(SPIDevice != CPLD_FLASH)
    {
        Blknum = 0xffffffff;
        Protect  = 0x00;    
        if(protectFlash(hSession,Blknum,Protect) !=0)
        {
            printf("Error in Protect Flash \n");
            return -1;
        }
    }
    if ((1 == Parsing.versioncmpflash) && (0 == VersionCmpFlash (ModNo, &Modver)))
    {
        if(1 == Parsing.SplitImg)
        {
            printf ("Upgrading Firmware by comparing Module version..\n");
            for (i =0; i< Modver;i++)
            {
            
                FwModuleHdr[ModNo[i]]->ModuleName[8] = '\0';
                if (CalculateOffset(FwModuleHdr[ModNo[i]]->ModuleName,&SizetoCopy,&CurrFMHLoc,&FMHLocation) != 0 )
                {
                    continue;
                }

                printf ("Upgrading module %s...                \r", FwModuleHdr[ModNo[i]]->ModuleName);
                fflush(stdout);
                if(FlashModule(hSession, img, SizetoCopy,CurrFMHLoc,FMHLocation,FwModuleHdr[ModNo[i]]->ModuleName) != 0 )
                {
                    printf ("Upgrading module %s...                Failed\n", FwModuleHdr[ModNo[i]]->ModuleName);
                    fclose(img);
                    return -1;
                }
                printf ("Upgrading module %s...                done\n", FwModuleHdr[ModNo[i]]->ModuleName);
            }
            
        }
        else
        {
            printf ("Upgrading Firmware by comparing Module version..\n");
            for (i =0; i< Modver;i++)
            {
                SilentFlash= 0x01;
                if (0 == strcmp(FwModuleHdr[ModNo[i]]->ModuleName, BOOT_SECTION_NAME) && (1 == Boot))
                {
                     continue;
                }
                if (0 == strcmp(FwModuleHdr[ModNo[i]]->ModuleName, "conf") && (1 == Config))
                {
                     continue;
                }
                printf ("Upgrading module %s...                \r", FwModuleHdr[ModNo[i]]->ModuleName);
                fflush(stdout);
                if (0 == ModuleUpgrade(hSession, img, ModNo[i], 1,iter,Modver))
                      printf ("Upgrading module %s...              done\n", FwModuleHdr[ModNo[i]]->ModuleName);
                else
                {
                    printf ("Upgrading module %s...                Failed\n", FwModuleHdr[ModNo[i]]->ModuleName);
                    return -1;
                }
                iter = i;
            }
        }
    }
    else
    {
        if (2 == Parsing.versioncmpflash)
        {
            Parsing.versioncmpflash = 1;
        }
    
        if((Parsing.Full== 0x01) || (CPLD_FLASH == SPIDevice) || (BIOS_FLASH == SPIDevice))
        {
            SizeToFlash=NewImageSize;
            if((retval=FlashFirmware(hSession,img,SizeToFlash,UpgradedBytes,seekpos)) !=0)
            {
                if(retval==1)
                        return retval;
                else
                        return -1;
            }
        }
        if(Parsing.Full == 0x02)
        {
            if(Parsing.Silent == 0x01)
            {          
                printf("Upgrading Firmware Image...");
                fflush(stdout);
            }
			
            while(UpgradedBytes < NewImageSize)
            {
                if(CommonConf == FALSE)
                {
                    if(Config == 0x01)
                    {
                         if ((Parsing.SplitImg == 0x01 && SplitFlashOffset != 0) && (UpgradedBytes == ConfigRomAddr) && (Configstatus == 0x00))
                        {
                            UpgradedBytes += ConfigFirmAllocAddr;
                            seekpos += ConfigFirmAllocAddr;
                            Configstatus = 0x01;
                            continue;
                        }

                        if(((UpgradedBytes == ConfigFirmAddr) ||((UpgradedBytes +EraseBlkSize) > ConfigFirmAddr))&& Configstatus == 0x00)
                        {
                            UpgradedBytes += ConfigFirmAllocAddr;
                            seekpos += ConfigFirmAllocAddr;
                            Configstatus = 0x01;
                            continue;
                        }
                    }
                }
				
                if(Boot == 0x01)
                {
                    
                    if ((Parsing.SplitImg == 0x01) && (UpgradedBytes == BootRomAddr) && (Bootstatus == 0x00))
                    {
                        UpgradedBytes += BootFirmAllocAddr;
                        seekpos += BootFirmAllocAddr;
                        if(BootImgRomAddr != 0)
                        {
                            UpgradedBytes += EraseBlkSize;
                            seekpos += EraseBlkSize;
                        }
                        Bootstatus = 0x01;
                        BootImgstatus = 0x01;
                        continue;
                    }
                    
                    if(((UpgradedBytes == BootFirmAddr) ||((UpgradedBytes +EraseBlkSize) > BootFirmAddr))&& Bootstatus == 0x00)
                    {
                        if (Parsing.SplitImg != 0x01 || SplitFlashOffset == 0) 
                        {
                            UpgradedBytes += BootFirmAllocAddr;
                            seekpos += BootFirmAllocAddr;
                            if(BootImgRomAddr != 0)
                            {
                                UpgradedBytes += EraseBlkSize;
                                seekpos += EraseBlkSize;
                            }
                            Bootstatus = 0x01;
                            BootImgstatus = 0x01;
                            continue;
                        }
                    }
                }

                if(Parsing.ExtlogPreserve == 0x01)
                {
                    if ((Parsing.SplitImg == 0x01 && SplitFlashOffset != 0) && (UpgradedBytes == ExtlogRomAddr) && (Extlogstatus == 0x00))
                    {
                        UpgradedBytes += ExtlogFirmAllocAddr;
                        seekpos += ExtlogFirmAllocAddr;
                        Extlogstatus = 0x01;
                        continue;
                    }

                    if(((UpgradedBytes == ExtlogFirmAddr) ||((UpgradedBytes +EraseBlkSize) > ExtlogFirmAddr))&& Extlogstatus == 0x00)
                    {
                        UpgradedBytes += ExtlogFirmAllocAddr;
                        seekpos += ExtlogFirmAllocAddr;
                        Extlogstatus = 0x01;
                        continue;
                    }
                }


                if(BootRomAllocAddr != 0)
                {
                    PreserveBootAddr = BootRomAllocAddr;
                    if(BootImgRomAddr != 0)
                        PreserveBootAddr += EraseBlkSize;

                    if(UpgradedBytes < PreserveBootAddr)
                    {
                        PreserveFlag &= BootFlag;
                    }
                    else
                    {

                        PreserveFlag |= BIT0;
                    }   
                }

                if ((Configstatus == 0x00) && (((Parsing.SplitImg == 0x01 && SplitFlashOffset != 0) && (UpgradedBytes == ConfigRomAddr) && (Configstatus == 0x00)) || 
                ((UpgradedBytes == ConfigFirmAddr) ||((UpgradedBytes +EraseBlkSize) > ConfigFirmAddr))))
                {

                    PreserveFlag &= ConfigFlag;
                    Configstatus = 0x01;
                }

                #if defined (CONFIG_SPX_FEATURE_YAFUFLASH_SKIP_FLASHING_FREE_AREA)
                    if(SkipFreeArea == 0)
                    {
                        if ((Parsing.SplitImg == 0x01 && SplitFlashOffset != 0) && (UpgradedBytes == FreeSpaceRomAddr) && (FreeSpaceStatus == 0x00))
                        {
                           UpgradedBytes += FreeSpaceFirmSize;
                           seekpos += FreeSpaceFirmSize;
                           FreeSpaceStatus = 0x01;
                           continue;
                        }

                        if(((UpgradedBytes == FreeSpaceFirmAddr) ||((UpgradedBytes +EraseBlkSize) > FreeSpaceFirmAddr))&& FreeSpaceStatus == 0x00)
                        {
                           UpgradedBytes += FreeSpaceFirmSize;
                           seekpos += FreeSpaceFirmSize;
                           FreeSpaceStatus = 0x01;
                           continue;
                        }
                    }
                #endif
				
    /*
    Moved this section from main () to this function.
    Reason: As soon as boot block is flashed, uboot environment variables
    (including MAC addresses) have to be written right after.  This
    will reduce the size of the time window, and therefore reduce
    chances of other errors (USB,KCS errors etc) causing the BMC to
    lose its MAC address or other important uboot variables.
    */
                if((Boot == 0x00) && (EnvWritten == 0) && (UpgradedBytes > (BootFirmAddr + BootFirmAllocAddr)))
                {
                    if (vdbg) { printf ("\nSaving Env variables\n");
                    fflush (stdout); }
    
                    EnvWritten = 1;
                      if(ImgOpt == -1)
                      {
                          printf("Block by Block Mode setting Environment Variables\n");
                          if(SetBlkUBootVars(hSession,&env, EraseBlkSize) != 0)
                          {
                                printf("Error in SettingEnvUbootVariables \n");
                          }
                                
                      }
                      else
                      {
                            if(SettingEnvUbootVariables(hSession,bvnArr,&BootVarsCount) != 0)
                            {
                                printf("Error in SettingEnvUbootVariables \n");
                            }
                      }
                }
                                
                if((UpgradedBytes + EraseBlkSize ) > NewImageSize)
                    SizeToFlash = (NewImageSize - UpgradedBytes);
                else
                    SizeToFlash = EraseBlkSize;
    			
                if(Parsing.SplitImg == 0x01 && SplitFlashOffset != 0)
                {
                    tempUpBytes = UpgradedBytes;
                    UpgradedBytes += SplitFlashOffset;
                }

                if((retval=FlashFirmware(hSession,img,SizeToFlash,UpgradedBytes,seekpos)) !=0)
                {
                    if(retval==1)
                        return retval;
                    else
                        return -1;
                }
    			
                if(Parsing.SplitImg == 0x01 && SplitFlashOffset != 0)
                {
                    UpgradedBytes = tempUpBytes;
                }
                seekpos += SizeToFlash;
                UpgradedBytes +=  SizeToFlash;
    
            }
            if(Parsing.Silent == 0x01)
            {
                printf("\rUpgrading Firmware Image ...    Done\n");    
            }
            else
            {
                printf("\rUpgrading Firmware Image : %d%%... done\n",Percentage);
            }
        }
    }

        if((ImgOpt == -1) && (Parsing.ConfigPreserve == 1)) 
        {
                for(Count=0;Count<ModuleCount;Count++)
                {
                        if(spstrcasecmp((char *)FwModuleHdr[Count]->ModuleName,"conf") == 0)
                        {
                           if(ReadConf == 0)
                           {
                               ReadConf++;
                           }
                           else if(ReadBkupConf == 0)
                           {
                               ReadBkupConf++;
                                break;
                           }
                          
                        }
                }

             if(ReadConf == 1)
             {
                  WriteBackupConfnBkconf(hSession,1);
             }
              if(ReadBkupConf == 1)
              {
                   WriteBackupConfnBkconf(hSession,2);
              }
        }
    fflush(stdout); 
    return 0;
}

/*
 * @fn ModuleUpgrade
 * @brief Funtion Upgrade the Firmware by module or 
 *        Verifies the flash with module depends on the Flash Flag.
 * @param hSession - Current Session Pointer
 * @param img - Image file
 * @param Module - Module Number to upgrade in Firmware
 * @param FlashFlag - Flag to flash the image.
 * @return Returns 0 on success
*/
int ModuleUpgrade(IPMI20_SESSION_T *hSession,FILE *img,int Module, int FlashFlag,int CurNumModule,int MaxNumModule)
{
    INT8U Protect =0;
#if defined (__x86_64__) || defined (WIN64)
    unsigned int SizetoAlloc = 0;
    unsigned int WriteMemOff = 0;
    unsigned int MemAlloc = 0;
    unsigned int VMemOffset = 0;
    unsigned int FlashOffset = 0;
    unsigned int SizetoCpy = 0,MemoryOff = 0;
#else
    unsigned long SizetoAlloc    = 0;
    unsigned long WriteMemOff = 0;
    unsigned long VMemOffset = 0;
    unsigned long MemAlloc = 0;
    unsigned long FlashOffset   = 0;
    unsigned long SizetoCpy = 0,MemoryOff = 0;
#endif
    char *Buf = 0,ModuleName[15]={0};
    INT32U Offset=0,SizeToRead=0,Blknum = 0;
    INT32U WriteOff;    
    INT16U Datalen = 0; 
#if defined (__x86_64__) || defined (WIN64)
    unsigned int seekpos =0,ImagePos=0;
#else
    unsigned long seekpos = 0,ImagePos=0;       
#endif

    if(GetStatus(hSession) !=0)
    {
        printf("\nError in GetStatus ");
        return -1;
    }   

    Blknum = 0xffffffff;
    Protect  = 0x00;    
    if(protectFlash(hSession,Blknum,Protect) !=0)
    {
        printf("Error in Protecting Flash \n");
        return -1;
    }


    strcpy(ModuleName,FwModuleHdr[Module]->ModuleName);
    ModuleName[8]='\0';
    if((spstrcasecmp(ModuleName,"boot") == 0) ||
      (spstrcasecmp(ModuleName,"boot_img") == 0))
    {
        PreserveFlag &= BootFlag;
    }
    else if(spstrcasecmp(ModuleName,"conf") == 0)
    {
        PreserveFlag &= ConfigFlag;
        PreserveFlag |= BIT0;
    }
    else if(spstrcasecmp(ModuleName,"extlog") == 0)
    {
        PreserveFlag &= ExtlogFlag;
        PreserveFlag |= BIT0;
    }
    else
    {
        PreserveFlag |= BIT0;
    }
	
   /* This is added to Handle Signed Image Support in Module Based Firmware Upgrade */
   if((SignedImageSup == 1) && (CurNumModule != 0xFF) && (MaxNumModule != 0xFF))
   {
      /* Upload the Total Image for Testing Sign in Image */
      if(CurNumModule == 0)
      {
        SizetoAlloc = NewImageSize;
        if(RecoveryFlashMode == 0x00)
        {
            if(MemoryAllocation(hSession,SizetoAlloc) != 0 || AddofAllocMem == 0xffffffff )
            {
                fprintf(stderr,"Error in Memory Allocation %x\n", (unsigned int)(SizetoAlloc));
                return -1;
            }
            if( AddofAllocMem == 0xfffffffe )
            {
                printf("There is no Sufficient Memory So,Block by Block Mode is not Supported for signed image\n");
                return -1;
            }
            MemOffset = AddofAllocMem;
        }
          while(Offset < NewImageSize)
          {
             if(Offset+MAX_SIZE_TO_READ > NewImageSize)
             {
                 SizeToRead = NewImageSize - Offset;
             }
             else
             {
                 SizeToRead = MAX_SIZE_TO_READ;
             }

             Buf = bufArr;
             if(fseek (img, Offset, SEEK_SET) !=0)
             {
                printf("\nError in fseek \n");
             }
             if(fread(Buf,1,SizeToRead,img) != SizeToRead)
             {
                printf("\nError in fread \n");
             }

            if(WritetoMemory(hSession,AddofAllocMem+Offset,SizeToRead,Buf) != 0)
            {
                if(RecoveryFlashMode == 0x00)
                    MemoryDealloc(hSession,AddofAllocMem);
                return -1;
            }
            Offset += SizeToRead;
          }
      }

      Offset = FwModuleHdr[Module]->FmhLocation;
      SizetoCpy = FwModuleHdr[Module]->AllocatedSize;

    if((Offset % EraseBlkSize) != 0x00)
    {
        if(spstrcasecmp(ModuleName,"boot") == 0)
            Offset = 0;
        else
            Offset /= EraseBlkSize;
    }

      if(RecoveryFlashMode == 0x00)
      {
            if(FlashFlag == ENABLEFLASH)
            {
                if(FlashAndVerify(hSession, AddofAllocMem+Offset, Offset, SizetoCpy,PreserveFlag) != 0)
                {
                    MemoryDealloc(hSession, AddofAllocMem);
                }
            }
            else
            {
                if(VerifyFlash(hSession,AddofAllocMem+Offset,Offset,SizetoCpy) != 0)
                {
                    MemoryDealloc(hSession,AddofAllocMem);	
                    return -1;
                }
                if(oldVersion != 0x01)
                {
                    if(VerifyStatus(hSession)==-1)
                    {
                        printf("Error in Verify Firmware\n");
                        MemoryDealloc(hSession, AddofAllocMem);	
                        return -1;
                    }
                }
            }
            if(CurNumModule == MaxNumModule-1)
            {
                if(MemoryDealloc(hSession,AddofAllocMem) != 0)
                {   
                    return -1;
                }
           }
        }
   }
   else
   {
     Offset = seekpos= FwModuleHdr[Module]->FmhLocation;

    if((Offset % EraseBlkSize) != 0x00)
    Offset  /= EraseBlkSize;
    MemoryOff = seekpos = ImagePos = Offset;
    while(Offset < (MemoryOff + FwModuleHdr[Module]->AllocatedSize))
    {
        SizetoAlloc = EraseBlkSize;
        if(RecoveryFlashMode == 0x00)
        {
            if(MemoryAllocation(hSession,SizetoAlloc) != 0)
            {
                printf("Error in Memory Allocation \n");
                return -1;
            }
            VMemOffset = MemAlloc = AddofAllocMem;
        }

        WriteOff = Offset;
        while((WriteOff < ( Offset + EraseBlkSize)) && (WriteOff < (MemoryOff + FwModuleHdr[Module]->AllocatedSize)) )
        {
            if((WriteOff + MAX_SIZE_TO_READ) > (Offset + EraseBlkSize))
                SizeToRead = ((Offset + EraseBlkSize) - WriteOff);
            else
                SizeToRead = MAX_SIZE_TO_READ;

            Buf=bufArr;
            memset(Buf, 0, SizeToRead);
            if(fseek (img, ImagePos, SEEK_SET) !=0)
                printf("\nError in fseek \n");
            if(fread(Buf,1,SizeToRead,img) != SizeToRead)
                printf("\nError in fread \n");

            WriteMemOff = VMemOffset; 
            Datalen = (INT16U)SizeToRead;

            if(RecoveryFlashMode == 0x01)
            {
                WriteMemOff = Offset;
            }
            if(WritetoMemory(hSession,WriteMemOff,Datalen,Buf) != 0)
            {
                if(RecoveryFlashMode == 0x00)
                    MemoryDealloc(hSession,MemAlloc);
                return -1;
            }
            VMemOffset += SizeToRead;
            ImagePos += SizeToRead;
            WriteOff += SizeToRead;

        }

        if(Offset >= (MemoryOff + FwModuleHdr[Module]->AllocatedSize))
            break; 
        FlashOffset = seekpos;
        SizetoCpy = EraseBlkSize;

        if(RecoveryFlashMode == 0x00)
        {
            if(FlashFlag == ENABLEFLASH)
            {
                if(FlashAndVerify(hSession, AddofAllocMem, FlashOffset, SizetoCpy,PreserveFlag) != 0)
                {
                    MemoryDealloc(hSession, MemAlloc);
                    return -1;
                }
            }
            else
            {
                if(VerifyFlash(hSession,AddofAllocMem,FlashOffset,SizetoCpy) != 0)
                {
                    MemoryDealloc(hSession,MemAlloc);
                    return -1;
                }
                if(oldVersion != 0x01)
                {
                    if(VerifyStatus(hSession)==-1)
                    {
                        MemoryDealloc(hSession, MemAlloc);
                        return -1;
                    }
                }
            }
            if(MemoryDealloc(hSession,MemAlloc) != 0)
            {
                return -1;
            }
        } 

        Offset += EraseBlkSize;
        seekpos +=EraseBlkSize;
        ImagePos = seekpos;
    }
   }
    return 0;
}
/*
 * @fn FlashFullFirmwareImage
 * @brief
 * @param hSession - Current Session Pointer
 * @param img - Image file
 * @param Config - Prserve or non-preserver the Config Params
 * @param Boot - Prserve or non-preserver the Boot Params
 * @return Returns 0 on success
*/
int FlashFullFirmwareImage(IPMI20_SESSION_T *hSession,FILE *img,int Config,int Boot)
{
    int retval=0;
    if(oldVersion==0x01)
    {
        Parsing.Full = 0x02;
    }
    if((retval=FullFlashThreadFn(hSession,img,Config,Boot)) != 0)
    {
        if(retval==1)
        {
            return retval;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

/*
 * @fn FlashModule
 * @brief  Flsah the module given by user in command line
 * @param hSession - Current Session Pointer
 * @param img - Image file
 * @param Sizetocopy - size of the module to flash
  * @param CurrFMHLoc - FMHLocation of existing firmware
  * @param FMHLocation -FMH Location in current image
 * @return Returns 0 on success
*/
#if defined (__x86_64__) || defined (WIN64)
int FlashModule(IPMI20_SESSION_T *hSession,FILE *img,unsigned int SizetoCopy,INT32U CurrFMHLoc,INT32U FMHLocation,INT8U *ModName)
#else
int FlashModule(IPMI20_SESSION_T *hSession,FILE *img, unsigned long SizetoCopy,INT32U CurrFMHLoc,INT32U FMHLocation,INT8U *ModName)
#endif
{
    INT8U Protect =0;
#if defined (__x86_64__) || defined (WIN64)
    unsigned int SizetoAlloc = 0;
    unsigned int WriteMemOff = 0;
    unsigned int MemAlloc = 0;
    unsigned int VMemOffset = 0;
    unsigned int FlashOffset = 0;
    unsigned int SizetoCpy = 0,MemoryOff = 0;
    unsigned int TempOffset = 0;
#else
    unsigned long SizetoAlloc    = 0;
    unsigned long WriteMemOff = 0;
    unsigned long VMemOffset = 0;
    unsigned long MemAlloc = 0;
    unsigned long FlashOffset   = 0;
    unsigned long SizetoCpy = 0,MemoryOff = 0;
    unsigned int TempOffset = 0;
#endif
    char *Buf = 0;
    INT32U Offset=0,SizeToRead=0,Blknum = 0;
    INT32U WriteOff;
    INT16U Datalen = 0;
#if defined (__x86_64__) || defined (WIN64)
    unsigned int seekpos =0,ImagePos=0;
#else
    unsigned long seekpos = 0,ImagePos=0;
#endif

    if(GetStatus(hSession) !=0)
    {
        printf("\nError in GetStatus ");
        return -1;
    }
    Parsing.Full = 0x00;

    Blknum = 0xffffffff;
    Protect  = 0x00;
    if(protectFlash(hSession,Blknum,Protect) !=0)
    {
       printf("Error in Protecting Flash \n");
       return -1;
    }

    if((spstrcasecmp(ModName,"boot") == 0) ||
      (spstrcasecmp(ModName,"boot_img") == 0))
    {
        PreserveFlag &= BootFlag;
    }
    else if(spstrcasecmp(ModName,"conf") == 0)
    {
        PreserveFlag &= ConfigFlag;
        PreserveFlag |= BIT0;
    }
    else if(spstrcasecmp(ModName,"extlog") == 0)
    {
        PreserveFlag &= ExtlogFlag;
        PreserveFlag |= BIT0;
    }
    else
    {
        PreserveFlag |= BIT0;
    }

    if(Parsing.SplitImg == 0x01)
    {
       Offset = CurrFMHLoc;
       ImagePos = FMHLocation;
       MemoryOff = seekpos = Offset;
       if((Offset % EraseBlkSize) != 0x00)
       {
           Offset  /= EraseBlkSize;
           MemoryOff = seekpos = ImagePos = Offset;
       }
    }
    else
    {
       Offset = FMHLocation;
       if((Offset % EraseBlkSize) != 0x00)
           Offset  /= EraseBlkSize;
       MemoryOff = seekpos = ImagePos = Offset;
    }

            
    while(Offset < (MemoryOff + SizetoCopy))
    {
        SizetoAlloc = EraseBlkSize;
        if(RecoveryFlashMode == 0x00)
        {
            if(MemoryAllocation(hSession,SizetoAlloc) != 0)
            {
                printf("Error in Memory Allocation \n");
                return -1;
            }
            VMemOffset = MemAlloc = AddofAllocMem;
        }

        WriteOff = Offset;
        while((WriteOff < ( Offset + EraseBlkSize)) && (WriteOff < (MemoryOff + SizetoCopy)) )
        {
            if((WriteOff + MAX_SIZE_TO_READ) > (Offset + EraseBlkSize))
                SizeToRead = ((Offset + EraseBlkSize) - WriteOff);
            else
                SizeToRead = MAX_SIZE_TO_READ;

            Buf=bufArr;
            memset(Buf, 0, SizeToRead);
            if(fseek (img, ImagePos, SEEK_SET) !=0)
                printf("\nError in fseek \n");
            if(fread(Buf,1,SizeToRead,img) != SizeToRead)
                printf("\nError in fread \n");

            WriteMemOff = VMemOffset; 
            Datalen = (INT16U)SizeToRead;

            if(RecoveryFlashMode == 0x01)
            {
                WriteMemOff = Offset;
            }
            if(WritetoMemory(hSession,WriteMemOff,Datalen,Buf) != 0)
            {
                if(RecoveryFlashMode == 0x00)
                    MemoryDealloc(hSession,MemAlloc);
                return -1;
            }
            VMemOffset += SizeToRead;
            ImagePos += SizeToRead;
            WriteOff += SizeToRead;

        }

        if(Offset >= (MemoryOff + SizetoCopy))
            break; 
        FlashOffset = seekpos;
        SizetoCpy = EraseBlkSize;

        if(RecoveryFlashMode == 0x00)
        {
            if(FlashAndVerify(hSession, AddofAllocMem, FlashOffset, SizetoCpy,PreserveFlag) != 0)
            {
                MemoryDealloc(hSession, MemAlloc);
                return -1;
            }
            
            if(MemoryDealloc(hSession,MemAlloc) != 0)
            {
                return -1;
            }
        } 

        Offset += EraseBlkSize;
        seekpos +=EraseBlkSize;
        if(Parsing.SplitImg != 0x01)
            ImagePos = seekpos;
    }
    return 0;
}



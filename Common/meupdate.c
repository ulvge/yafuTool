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
* Filename: meupdate.c
*
* Author   : Austin <AustinLin@ami.com.tw>
*
******************************************************************/

/* Includes */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#ifdef WINDOWS
#include <conio.h>
#include <windows.h>
#include <winioctl.h>
#include <winsvc.h>
#else
#include <ctype.h>
#endif
#include "flashcmds.h"
#include "main.h"
#include "meupdate.h"
#ifndef MSDOS
#include "icc_what.h"
#include "IPMIDefs.h"
#include "libipmi_session.h"
#include "libipmi_errorcodes.h"
#include "IPMI_AMIDevice.h"
#include "libipmi_struct.h"
#include "libipmi_AMIOEM.h"
#include "coreTypes.h"
#else
#include "flashlib.h"
#endif

static FILE *ME_fp;
int DONMMEUpdate = 0;

#if defined(__x86_64__) || defined(WIN64)
unsigned int MENewImageSize = 0;
#else
unsigned long MENewImageSize = 0;
#endif

#if defined(__x86_64__) || defined(WIN64)
unsigned int MEAddofAllocMem = 0;
#else
unsigned long MEAddofAllocMem = 0;
#endif

#if defined(__x86_64__) || defined(WIN64)
unsigned int MEEraseBlkSize = 0, MEFlashSize = 0;
#else
unsigned long MEEraseBlkSize = 0, MEFlashSize = 0;
#endif

char bufArr_ME[MAX_SIZE_TO_READ_ME];
INT8U ME_Current_Status = 0x00;
INT8U ME_Comparison_Status = 0x00;

extern void LoadOpenIPMIDrivers();
extern UPDATE_INFO Parsing;

/*
 * @fn GetMEImageSize
 * @brief this function is used to get ME image size
 * @praram imgName ME image Name
 * @return Returns 0 on success
 */
int GetMEImageSize(char *imgName)
{
#if defined(__x86_64__) || defined(WIN64)
    unsigned int len = 0;
#else
    unsigned long len = 0;
#endif

    ME_fp = fopen(imgName, "rb");
    if (ME_fp == NULL)
    {
        printf("Image file (%s) could not be opened or not present.\n", imgName);
        exit(YAFU_FILE_OPEN_ERR);
    }
    else
    {
        if (fseek(ME_fp, 0, SEEK_END))
        {
            fclose(ME_fp);
            return -1;
        }
        else
        {
            len = ftell(ME_fp);
            MENewImageSize = len;
            if (len == -1)
            {
                printf("ftell returned error: %s\n", strerror(errno));
                fclose(ME_fp);
                return -1;
            }
            if (MENewImageSize == 0)
            {
                printf("WARNING!! No data in the Image File\nInvalid Image File!!\n");
                exit(YAFU_CC_IMAGE_SIZE_INVALID);
            }
        }
    }

    return 0;
}

/*
 * @fn print_MEUpdateInfo
 * @brief this function is used to print ME update Image topic
 */
void print_MEUpdateInfo()
{
    printf("\n");
    printf("-------------------------------------------------\n");
    printf("YAFUFlash - Updating ME Image\n");
    printf("-------------------------------------------------\n");
    printf("\n");
}

/*
 * @fn DoMEUpdate
 * @brief this function is used to upload and flash ME image
 * @praram Session parameter
 * @return Returns 0 on success
 */
int DoMEUpdate(IPMI20_SESSION_T *hSession)
{
    int count = 0, i = 0;
    int byteread = 0;
    int datalength = 0;
    int rembyte = 0;
    unsigned char *buffer;
    unsigned long SizetoAlloc = 0;
    unsigned long SizetoLoad = 0;
    unsigned long SizetoRead = 0;
    unsigned long WriteMemOff = 0;
    unsigned long seekoffset = 0;
    unsigned long offset = 0;
    unsigned long memoryoffset = 0;
    char FlashChoice[MAX_INPUT_LEN];

    DONMMEUpdate = 1;

    SizetoAlloc = MENewImageSize;
    if (MemoryAllocation(hSession, SizetoAlloc) != 0)
    {
        printf("Error in Memory Allocation  %x\n", (unsigned int)(SizetoAlloc));
        return -1;
    }

    memoryoffset = MEAddofAllocMem;
    SizetoLoad = MENewImageSize;
    count = (SizetoLoad / MAX_SIZE_TO_READ_ME);
    for (i = 0; i < count; i++)
    {
        SizetoRead = MAX_SIZE_TO_READ_ME;
        buffer = bufArr_ME;
        memset(buffer, 0, SizetoRead);

        if (fseek(ME_fp, seekoffset, SEEK_SET) != 0)
        {
            printf("Error in fseek\n");
        }

        byteread = fread(buffer, 1, SizetoRead, ME_fp);
        if (byteread != SizetoRead)
        {
            printf("Error in fread byteread=%x\n", byteread);
            return -1;
        }

        WriteMemOff = MEAddofAllocMem;
        datalength = byteread;

        if (WritetoMemory(hSession, WriteMemOff, datalength, buffer) != 0)
        {
            printf("Error in Uploading Firmware Image\n");
            return -1;
        }

        offset += SizetoRead;

        printf("Uploading Firmware Image : %d%%\r", (int)((offset * 100) / MENewImageSize));

        MEAddofAllocMem += SizetoRead;
        seekoffset += SizetoRead;
    }

    rembyte = (SizetoLoad % MAX_SIZE_TO_READ_ME);
    if (rembyte != 0)
    {
        if (fseek(ME_fp, seekoffset, SEEK_SET) != 0)
        {
            printf("Error in fseek\n");
        }

        byteread = fread(buffer, 1, rembyte, ME_fp);
        if (byteread != rembyte)
        {
            printf("Error in fread byteread=%x\n", byteread);
            return -1;
        }

        WriteMemOff = MEAddofAllocMem;
        datalength = byteread;

        if (WritetoMemory(hSession, WriteMemOff, datalength, buffer) != 0)
        {
            printf("Error in Uploading Firmware Image\n");
            return -1;
        }

        offset += byteread;
        printf("Upgrading Firmware Image : %d%%\r", (int)((offset * 100) / MENewImageSize));
    }

    printf("Uploading Firmware Image: 100%%... done\n");

    if (CompareMeVersion(hSession, memoryoffset, MENewImageSize) != 0)
    {
        printf("Error: Compare Me Version fail\n");
        return -1;
    }

    // status from CompareMeVersion()
    if (ME_Current_Status != NORMALME)
    {
        printf("Error: No Normal ME version\n");
        return -1;
    }

    // status from CompareMeVersion()
    if (ME_Comparison_Status == INVALID_ME_IMG)
    {
        printf("Invalid ME version\n");
        return -1;
    }

    printf("Type(Y/y) continute to Upgrade or (N/n) exit\n");
    printf("Enter your Option :");
    scanf(" %[^\n]%*c", FlashChoice);

    if (strcmp(FlashChoice, "y") != 0 && strcmp(FlashChoice, "Y") != 0)
    {
        if (DeactivateFlshMode(hSession) != 0)
        {
            printf("Error in Deactivating the flash Mode\n");
            return -1;
        }
        Close_Session(hSession);
        exit(0);
    }

    if (EraseAndFlash(hSession, memoryoffset, 0, MENewImageSize) != 0)
    {
        printf("Error in Erase and Flash...\n");
        return -1;
    }

    if (ECFStatus(hSession) != 0)
    {
        printf("Error while getting Flash Progress Status\n");
        return -1;
    }

    return 0;
}

/*
 * @fn StartDoMEUpdate
 * @brief this function is used to Start ME update
 * @praram Session parameter
 * @return Returns 0 on success
 */
int StartDoMEUpdate(IPMI20_SESSION_T *hSession)
{
    print_MEUpdateInfo();

    if (SwitchFlashDevice(hSession, &MEEraseBlkSize, &MEFlashSize) < 0)
    {
        printf("Error in identifying the Flash information\n");
#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif
        return -1;
    }

    if (GetMEImageSize(Parsing.FileName) != 0)
    {
        printf("Error in reading the file\n");
#ifdef ICC_OS_LINUX
        LoadOpenIPMIDrivers();
#endif
        exit(-1);
    }

    if (ActivateFlashMode(hSession) != 0)
    {
        printf("Error in activate flash mode\n");
        fclose(ME_fp);
        return -1;
    }

    if (DoMEUpdate(hSession) != 0)
    {
        printf("ME update error\n");
    }
    else
        printf("ME update success\n");

    fclose(ME_fp);

    if (DeactivateFlshMode(hSession) != 0)
    {
        printf("\nError in Deactivate Flash mode\n");
        return -1;
    }

    return 0;
}

/*************************************************************************
RexDebug  - Generic Debugger 

Copyright (c) 2000-2005, Samvinesh Christopher
Written by Samvinesh Christopher.  Not derived from licensed software.

Permission is granted to anyone to use this software for any purpose 
on any computer system, and to redistribute it freely,subject to the 
following restrictions:

1. The author is not responsible for the consequences of use of this 
   software, no matter how awful, even if they arise from defects in 
   it.

2. The origin of this software must not be misrepresented, either by 
   explicit claim or by omission.

3. Altered versions must be plainly marked as such, and must not  be 
   misrepresented as being the original software.

4. This notice may not be removed or altered.

5. Commercial use of this software is allowed freely subject to prior
   notificaiton to the author.
**************************************************************************/
#ifndef REX_DBG_HASH_H
#define REX_DBG_HASH_H
#include "config.h"
#include "types.h"

UINT32 KRHash  (char* str, UINT32 len);
UINT32 ELFHash (char* str, UINT32 len);
UINT32 PJWHash (char* str, UINT32 len);
UINT32 SDBMHash(char* str, UINT32 len);
UINT32 DJB2Hash(char* str, UINT32 len);

typedef struct hash_entry
{	
	char *str;
	char *value;
	UINT8 type;
	struct hash_entry *next;
} HASH_ENTRY;

typedef struct
{
	HASH_ENTRY **HashTable;
	UINT32 HashSize;
	HASH_ENTRY *ListHead;
} HASH_HANDLE;

/* Basic Functions to Handle Hash Tables */
HASH_HANDLE *InitHashTable(UINT32 count);
BOOL ReleaseHashTable(HASH_HANDLE *handle);
BOOL AddStrToHash(HASH_HANDLE *handle, char *str, char *value, UINT8 type);
BOOL DelStrFromHash(HASH_HANDLE *handle,char *str);

/* Functions to Access Information from Hash Tables*/
BOOL FindStrInHash(HASH_HANDLE *handle,char *str, char **value, UINT8* type);
int  GetNumberOfHashEntries(HASH_HANDLE *handle,int *novalues);
char *GetNonValueHashEntry(HASH_HANDLE *handle,int Index);
char *GetHashEntry(HASH_HANDLE *handle,int Index);


/* Debugging Functions */
void DumpHashTable(HASH_HANDLE *handle);
void PrintHashDistribution(HASH_HANDLE *handle);

/* Extended Function for special operations */
void FreeHashList(HASH_HANDLE *handle);
HASH_ENTRY *ConvertHashToList(HASH_HANDLE *handle);

#endif  

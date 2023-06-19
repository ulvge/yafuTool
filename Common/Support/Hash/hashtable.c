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
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "types.h"
#include "console.h"
#include "hash.h"

/* Define what hash method we are going to use */
#define HashFunction(str,len) 	DJB2Hash(str,len)


static 
HASH_ENTRY *
AddEntryToList(HASH_ENTRY *List, HASH_ENTRY *entry)
{
	HASH_ENTRY *ListHead;
	HASH_ENTRY *ptr,*prev;
	
	ListHead=List;

	/* The list of strings are arranged in ascending order.
	   Find a proper spot for our string */
	ptr = List;
	prev = NULL;
	while (ptr != NULL)
	{
		if (strcmp(ptr->str,entry->str) ==0)
		{	
			ConsolePrintf("WARNING: Duplicate Entry %s. Cannot happen\n",
																entry->str);
			return ListHead;	
		}
		if (strcmp(ptr->str,entry->str) > 0)
			break;
		prev=ptr;
		ptr= ptr->next;			
	}
	/* Change links */	
	entry->next = ptr;
	if (prev != NULL)
		/* Prev is Not null, point prev's next to us */
		prev->next = entry;
	else
		/* Prev is NULL (i.e) Change HashTable to point to us */
		ListHead = entry;
	return ListHead;
}



static
HASH_ENTRY *
AddHashToList(HASH_ENTRY *List,HASH_ENTRY *Hash)
{
	HASH_ENTRY *SaveHash;
	/* First entry, return as it is */
	if (List == NULL)
		return Hash;
	while (Hash != NULL)
	{
		/* Hash->next will be modified by AddEntryToList in 
		   certain scenarious. So move to next before calling 
		   the function */
		SaveHash = Hash;
		Hash = Hash->next;
		List =AddEntryToList(List,SaveHash);
	}	
	return List;	
}



HASH_ENTRY *
ConvertHashToList(HASH_HANDLE * handle)
{
	HASH_ENTRY *List=NULL;
	int i;
	
	for (i=0;i<handle->HashSize;i++)
	{
		if (handle->HashTable[i] == NULL)		
			continue;
		List = AddHashToList(List,handle->HashTable[i]);	
		handle->HashTable[i] = NULL;
	}

	handle->ListHead = List;
	return List;

}

void
FreeHashList(HASH_HANDLE *handle)
{
	HASH_ENTRY *Node;
	HASH_ENTRY *Head;
	
	Head = handle->ListHead;	

	while (Head != NULL)
	{
		/* Save Node to be deleted */
		Node = Head;

		/* Point Head to next */
		Head= Head->next;
		
		/* Free String Memory */
		if (Node->str)	
			free(Node->str);	/* We used strdup to create this. So free it */
		if (Node->value);	
			free(Node->value);	/* We used strdup to create this. So free it */

		/* Free Entry Memory */
		free(Node);
	}
	return ;	
}



HASH_HANDLE *
InitHashTable(UINT32 count)
{
	int i;
	HASH_HANDLE *handle;

	handle = (HASH_HANDLE *)malloc(sizeof(HASH_HANDLE));
	if (handle == NULL)
	{
		ConsolePrintf("ERROR: Memory allocation Failed for hash handle\n");
		return NULL;
	}
	handle->ListHead = NULL;
	handle->HashSize = count;

	handle->HashTable= (HASH_ENTRY **)malloc(sizeof(HASH_ENTRY *) * count);
	if (handle->HashTable == NULL)
	{
		ConsolePrintf("ERROR: Memory allocation Failed for hash table\n");
		free(handle);
		return NULL;
	}
	
	for (i=0;i<handle->HashSize;i++)
			handle->HashTable[i] = NULL;
	return handle;
}

BOOL
ReleaseHashTable(HASH_HANDLE *handle)
{
	/* Covert to a list of entries */
	ConvertHashToList(handle);

	/* Free the entries */
	FreeHashList(handle);

	/* Free the table */
	free(handle->HashTable);

	/* free the handle */
	free(handle);

	return TRUE;
}


BOOL
AddStrToHash(HASH_HANDLE *handle, char *str,char *value, UINT8 type)
{
	UINT32 hash;
	HASH_ENTRY *entry,*ptr,*prev;

	/* Check for NULL string */
	if (str == NULL)
		return FALSE;

	/* Check for empty string */
	if (strlen(str) == 0)
		return FALSE;

	/* Compute the hash of the string */
	hash = HashFunction(str,strlen(str));
	
	/* Convert hash to index to table */
	hash %= handle->HashSize;

	/* Create a new entry */
	entry = (HASH_ENTRY *)malloc(sizeof(HASH_ENTRY));
	if (entry == NULL)
	{
		ConsolePrintf("ERROR: Memory allocation Failed for adding %s to hash table\n",str);
		return FALSE;
	}
	entry->str  = strdup(str);
	if (value)
		entry->value = strdup(value);
	else
		entry->value = NULL;
	entry->type = type;
	entry->next = NULL;


	/* If no previous entries add to the table */
	if (handle->HashTable[hash] == NULL)
	{
		handle->HashTable[hash] = entry;
		return TRUE;	
	}

	/* The list of strings are arranged in ascending order.
	   Find a proper spot for our string */
	ptr = handle->HashTable[hash];
	prev = NULL;
	while (ptr != NULL)
	{
		if (strcmp(ptr->str,str) ==0)
		{	
			ConsolePrintf("WARNING: Duplicate Entry %s Cannot Add\n",str);
			return FALSE;
		}
		if (strcmp(ptr->str,str) > 0)
			break;
		prev=ptr;
		ptr= ptr->next;			
	}

	/* Change links */	
	entry->next = ptr;
	if (prev != NULL)
		/* Prev is Not null, point prev's next to us */
		prev->next = entry;
	else
		/* Prev is NULL (i.e) Change HashTable to point to us */
		handle->HashTable[hash] = entry;

	return TRUE;
}

BOOL
FindStrInHash(HASH_HANDLE *handle,char *str, char **value, UINT8 *type)
{
	UINT32 hash;
	HASH_ENTRY *ptr;

	*value = NULL;
	*type = 0;

	/* Check for NULL string */
	if (str == NULL)
		return FALSE;

	/* Check for empty string */
	if (strlen(str) == 0)
		return FALSE;

	/* Compute the hash of the string */
	hash = HashFunction(str,strlen(str));
	
	/* Convert hash to index to table */
	hash %= handle->HashSize;

	/* The list of strings are arranged in ascending order.Find our str */
	ptr = handle->HashTable[hash];
	
	
	while (ptr != NULL)
	{
		if (strcmp(ptr->str,str) ==0)
			break;
		if (strcmp(ptr->str,str) > 0)
			return FALSE;
		ptr= ptr->next;			
	}
	if (ptr == NULL)
		return FALSE;

	/* Return the value stored in this */
	*value = ptr->value;
	*type = ptr->type;
	return TRUE;
}


BOOL
DelStrFromHash(HASH_HANDLE *handle,char *str)
{
	UINT32 hash;
	HASH_ENTRY *ptr,*prev;

	/* Check for NULL string */
	if (str == NULL)
		return FALSE;

	/* Check for empty string */
	if (strlen(str) == 0)
		return FALSE;

	/* Compute the hash of the string */
	hash = HashFunction(str,strlen(str));
	
	/* Convert hash to index to table */
	hash %= handle->HashSize;

	/* The list of strings are arranged in ascending order.Find our str */
	ptr = handle->HashTable[hash];
	prev = NULL;
	while (ptr != NULL)
	{
		if (strcmp(ptr->str,str) ==0)
			break;
		if (strcmp(ptr->str,str) > 0)
		{
			ConsolePrintf("WARNING: Delete String %s is not in HashTable\n",str);
			return FALSE;
		}
		prev=ptr;
		ptr= ptr->next;			
	}

	if (ptr == NULL)
	{
		ConsolePrintf("WARNING: Delete String %s is not in HashTable\n",str);
		return FALSE;
	}

	/* Change links */	
	if (prev == NULL)
		handle->HashTable[hash] = ptr->next;
	else
		prev->next =  ptr->next;

	/* Free String Memory */
	if (ptr->str)	
		free(ptr->str);		/* We used strdup to create this. So free it */
	if (ptr->value)	
		free(ptr->value);	/* We used strdup to create this. So free it */

	/* Free Hash Entry Memory */
	free(ptr);

	return TRUE;
}

void
PrintHashDistribution(HASH_HANDLE *handle)
{
	int count;
	HASH_ENTRY *ptr;
	int i;
	for (i=0;i<handle->HashSize;i++)
	{
		ptr = handle->HashTable[i];
		count = 0;
		while (ptr!=NULL)
		{
			count++;
			ptr=ptr->next;
		}
		ConsolePrintf("%04X = %04X ;",i,count);
	}
	return;
}

void
DumpHashTable(HASH_HANDLE *handle)
{
	HASH_ENTRY *ptr;
	int i;
	for (i=0;i<handle->HashSize;i++)
	{
		ptr = handle->HashTable[i];
		while (ptr!=NULL)
		{
			if (ptr->value)
				ConsolePrintf("%s=%d:%s\n",ptr->str,ptr->type,ptr->value);
			else	
				ConsolePrintf("%s\n",ptr->str);
			ptr=ptr->next;
		}
	}
	return;
}


/* Returns total number of entries and the number of entries with no value */
int
GetNumberOfHashEntries(HASH_HANDLE *handle,int *novalues)
{
	HASH_ENTRY *ptr;
	int i;
	int entries;
	
	*novalues= 0;
	entries = 0;
	for (i=0;i<handle->HashSize;i++)
	{
		ptr = handle->HashTable[i];
		while (ptr!=NULL)
		{
			if (!ptr->value)
				(*novalues)++;

			/* Increment entries */
			entries++;
			ptr=ptr->next;
		}
	}

	return entries;
}


char *
GetHashEntry(HASH_HANDLE *handle,int Index)
{
	HASH_ENTRY *ptr;
	int i;
	int entries;
	
	entries = 0;
	for (i=0;i<handle->HashSize;i++)
	{
		ptr = handle->HashTable[i];
		while (ptr!=NULL)
		{
			entries++;
			if (entries == Index)
				return ptr->str;
			ptr=ptr->next;
		}
	}
	return NULL;
}

char *
GetNonValueHashEntry(HASH_HANDLE *handle,int Index)
{
	HASH_ENTRY *ptr;
	int i;
	int novalues;

	novalues = 0;
	for (i=0;i<handle->HashSize;i++)
	{
		ptr = handle->HashTable[i];
		while (ptr!=NULL)
		{
			if (!ptr->value)
			{
				if (novalues == Index)
					return ptr->str;
				novalues++;
			}
			ptr=ptr->next;
		}
	}
	return NULL;
}



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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "types.h"
#include "rexstr.h"
#include "console.h"
#include "hash.h"
#include "iniparse.h"

#define MAX_LINE_LEN   1024
#define HASH_BUCKETS   128	



static
char*
ExtractValue(char *line, int type)
{
	BOOL Quot = FALSE;
	char *Saveline = line;
	

	/* Extract till a delimiter if found . # or ; not enclosed in "" or EOL */
	while (*line)
	{
		/* if " is found, Set Quot to be start or end of " */
		if (*line == '"')
		{
			Quot = (Quot==TRUE)?FALSE:TRUE;
			line ++;
			continue;
		}
	
		/* If the Quot is found, all the characters are part of the string */
		if (Quot == TRUE)	
		{
			line++;
			continue;
		}

		/* Here comes if Quot is FALSE. Check for Comment delimiters */
		if ((*line == '#') || (*line == ';'))
			break;
		
		/* Normal character : Not a # or ; */	
		line++;				
	}

	*line = 0;
	Saveline = RexSkipTrailingWhiteSpaces(Saveline);
	return Saveline;
}

static
char *
GetIniTypeStr(UINT8 Type)
{
	switch (Type)
	{
		case INI_TYPE_STR:
			return INI_TYPESTR_STR;

		case INI_TYPE_NUM:
			return INI_TYPESTR_NUM;

		case INI_TYPE_MSTR:
			return INI_TYPESTR_MSTR;

		case INI_TYPE_MNUM:
			return INI_TYPESTR_MNUM;
	}

	ConsolePrintf("WARNING:Unknown Value type [%d]. Treating it as INI_STR\n",Type);
	return INI_TYPESTR_STR; 	/* Default Value */
}


static
UINT8
GetIniType(char *str)
{
	if (strcmp(str,INI_TYPESTR_STR) == 0)
		return INI_TYPE_STR;	
	if (strcmp(str,INI_TYPESTR_NUM) == 0)
		return INI_TYPE_NUM;	
	if (strcmp(str,INI_TYPESTR_MSTR) == 0)
		return INI_TYPE_MSTR;	
	if (strcmp(str,INI_TYPESTR_MNUM) == 0)
		return INI_TYPE_MNUM;	
		
	ConsolePrintf("WARNING:Unknown Value type [%s]. Treating it as INI_STR\n",str);
	return INI_TYPE_STR;
}


static
void
ProcessIniFile(FILE *fd, HASH_HANDLE *handle)
{
	char LineBuffer[MAX_LINE_LEN];
	char section[MAX_LINE_LEN]="";
	char seckey[MAX_LINE_LEN];
	char *line,*delim;
	char *key;
	char *value;
	char *typestr;
	UINT8 type;
	int  ret;

	while (!feof(fd))
	{
		/* ReInitialize all variables except section */
		line = LineBuffer;
		key = value = typestr = delim = NULL;
		type = INI_TYPE_STR;

		/* Read a line from file */
		if (NULL == fgets(line,MAX_LINE_LEN,fd))
			break;

		/* Skip trailing \n and/or \r characters */
		delim = strchr(line,'\n');
		if (delim != NULL)  *delim=0;
		delim = strchr(line,'\r');
		if (delim != NULL)  *delim=0;

		/* Skip leading white spaces */
		line = RexSkipWhiteSpaces(line);
		
		/* Check if it is a comment line or a empty line */
		if ((*line == '#') || (*line == ';') || (*line == 0))
			continue;

		/* Check if it is a section - a string enclosed in [] at the start
	       of line and [ must be the first non white space char of line */
		ret=sscanf(line,"[%[^]]",section);
		if (ret > 0)
		{
			/* Add to Hash Table */	
			AddStrToHash(handle,section,NULL,0);
			continue;
		}

		/* Ignore any other lines if no section is still found */
		if (strcmp(section,"") == 0)
		{
			ConsolePrintf("WARNING:Ignoring lines without section\n");
			continue;
		}

		/* Get pointer to = in the line */	
		delim = strchr(line,'=');

		/* Ignore Invalid lines. lines without = */
		if (delim == NULL)
		{
			ConsolePrintf("WARNING:Ignoring lines without '=' \n");
			continue;
		}
			
		/* Extract the key and terminate properly*/
		key = line;
		*delim = 0;

		/* Get a proper key . Leading spaces are already removed */
		key = RexSkipTrailingWhiteSpaces(key);
		if (key == NULL)
		{
			ConsolePrintf("WARNING:Ignoring lines without key\n");
			continue;
		}
		
		/* Move line pointer after =  and remove leading white spaces */
		line = delim+1;
		line =RexSkipWhiteSpaces(line);
		
		/* Now we have to extract the value from here. Check if has TYPE: */
		delim = strchr(line,':');
		if (delim != NULL)
		{
			typestr=line;
			*delim = 0;
			typestr=RexSkipTrailingWhiteSpaces(typestr);
			type = GetIniType(typestr);

			/* Move line pointer after :  and remove leading white spaces */
			line = delim+1;
			line = RexSkipWhiteSpaces(line);
		}

		/* TYPE: is removed. Rest is pure value . Extract it */			
		value = ExtractValue(line,type);		
		if (value == NULL)
		{
			ConsolePrintf("WARNING:Ignoring lines without value\n");
			continue;
		}
		if (strcmp(value,"")==0)
		{
			ConsolePrintf("WARNING:Ignoring lines without value\n");
			continue;
		}
	
		/* Add to Hash Table */	
//		ConsolePrintf("Found Section [%s] Key [%s] Value [%s] Type [%d]\n",
//						section,key,value,type);
		sprintf(seckey,"%s:%s",section,key);
		AddStrToHash(handle,seckey,value,type);
	}

	return;
}

static
BOOL
ReadIniInfo(char *IniFile, HASH_HANDLE *handle)
{

	FILE *fd;
	/* Open the IniFile */
	fd = fopen(IniFile,"rb");
	if (fd == NULL)
	{
		//ConsolePrintf("ERROR: Unable to open INI File %s\n",IniFile);
		return FALSE;
	}
	
	/* Process the Ini File */
	ProcessIniFile(fd,handle);

	/* Close the IniFile */
	fclose(fd);

	return TRUE;
}

	

HANDLE
InitIniTable(char *IniFile)
{
	HASH_HANDLE *handle;
	UINT8 temptype;
	char *tempvalue;

	/* Create Hash Table */	
	handle = InitHashTable(HASH_BUCKETS);
	if (handle == NULL)
		return NULL;

	/* Read the INI File and fill up hash tables */
	if (ReadIniInfo(IniFile,handle) == FALSE)
	{
		ReleaseHashTable(handle);
		return NULL;
	}
	
	/* Add a / section if not present already */
	if (ReadIniEntry((HANDLE)handle,"/",NULL,&tempvalue,&temptype) !=TRUE)
		AddStrToHash(handle,"/",NULL,0);

	return (HANDLE)handle;
}

BOOL
FreeIniTable(HANDLE handle)
{
	return ReleaseHashTable((HASH_HANDLE *) handle);	
}

void
DumpIniTable(HANDLE handle)
{
	DumpHashTable((HASH_HANDLE *)handle);
	return;
}

BOOL
ReadIniEntry(HANDLE handle,char *sec, char *key , char **value, UINT8 *type)
{
	char seckey[MAX_LINE_LEN];
	if (key !=NULL)
		sprintf(seckey,"%s:%s",sec,key);
	else
		sprintf(seckey,"%s",sec);
	return FindStrInHash(handle,seckey,value,type);

}



BOOL 
AddIniEntry(HANDLE handle, char *section, char *key , char *value, UINT8 type)
{
	char seckey[MAX_LINE_LEN];
	UINT8 temptype;
	char *tempvalue;

	/* Before Adding key/value, check if the section present 
	   if not add a section */
	if (key !=NULL)
	{
		if (ReadIniEntry((HASH_HANDLE *)handle,section,NULL,
										&tempvalue,&temptype) !=TRUE)
		{
			if (AddStrToHash((HASH_HANDLE *)handle,section,NULL,0) != TRUE)
			{
				ConsolePrintf("ERROR:Unable to creation section %s\n",section);
				return FALSE;
			}
		}
	}


	if (key !=NULL)
	{
		sprintf(seckey,"%s:%s",section,key);
	}	
	else
		sprintf(seckey,"%s",section);
	return AddStrToHash((HASH_HANDLE *)handle,seckey,value,type);
}

BOOL 
DelIniEntry(HANDLE handle, char *section, char *key)
{
	char seckey[MAX_LINE_LEN];
	if (key == NULL)
	{
		/*TODO: If section to be deleted. first delete all the keys 
				and then delete the section
		*/
		ConsolePrintf("Cannot Delete Sections from INI File\n");
		return FALSE;
	}
	sprintf(seckey,"%s:%s",section,key);
	return DelStrFromHash((HASH_HANDLE*)handle,seckey);
}

BOOL 
WriteIniFile(HANDLE handle, char *FileName)
{
	HASH_ENTRY *List;
	FILE *fd;
	char *delim,*key;

	/* Open the IniFile */
	fd = fopen(FileName,"wb");
	if (fd == NULL)
	{
		ConsolePrintf("ERROR: Unable to open INI File %s\n",FileName);
		return FALSE;
	}
	/* Create a list From Hash */
	List = ConvertHashToList((HASH_HANDLE *)handle);
	
	while (List != NULL)
	{
		/* Section name is one with no : in str */
		delim = strrchr(List->str,':');
		if (delim == NULL)	
		{
			fprintf(fd,"[%s]\n",List->str);
		}
		else
		{
			key = delim+1;
			*delim=0;
			fprintf(fd,"\t%s\t\t= %s: %s\n",key,GetIniTypeStr(List->type),List->value); 
		}
		List = List->next;
	}
	
	
	/* Close the File*/
	fclose(fd);

	/* Free the List from Hash */
	FreeHashList(handle);

	/* Read the new information back to Hash*/
	return ReadIniInfo(FileName,handle);
}


int
GetIniSectionCount(HANDLE handle)
{
	int sections; 		
	/* Get the number of non values entries -> Sections don't have any values */
	GetNumberOfHashEntries((HASH_HANDLE*)handle,&sections);
	return sections;
}

int
GetIniEntryCount(HANDLE handle)
{
	int sections; 		
	return GetNumberOfHashEntries((HASH_HANDLE*)handle,&sections);
}

char *
GetIniSectionName(HANDLE handle,int Index)
{
	return GetNonValueHashEntry((HASH_HANDLE *)handle,Index);		
}


char *
GetIniEntryName(HANDLE handle,int Index)
{
	return GetHashEntry((HASH_HANDLE *)handle,Index);		
}

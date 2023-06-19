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
#ifndef REX_DBG_INIPARSE_H
#define REX_DBG_INIPARSE_H
#include <stdio.h>
#include "config.h"
#include "types.h"

/************************************************************************* 
	INI File Syntax:
	
   	# This is a comment 
   	; This is a comment 
   	[Section]	
		Key1 = Value1						; This is a comment
		Key2 = Value2	
   	[Section/SubSection]	
		Key3 = Value3						# This is a comment
		Key4 = "# This is not a comment"
	
	and so on..	

  Comment Syntax:
	A comment normally starts with # or ; at the start of the line or 
    after value. Any # or ; found in key or section is not treated as 
    comment. Also # or ;  "" enclosed string in value are not treated 
   	as comment.

  Section Syntax:	
	Can consists of any printable character (includes # and ;) except ] 
	A / in the section string splits the section into subsections

  Key Syntax:	
	Can consists of any printable character (includes # and ;) except =
	= will be treated as a delimiter between key and value

  Value Syntax:
	Value is in the form of TYPE:STRING 
		where TYPE can be either INI_STR (STRING is a ASCIIZ string), 
		INI_NUM (STRING is a numeric value), INI_MSTR (STRING is a 
		series of ASCIIZ strings) or INI_MNUM (STRING is a series of 
		numeric values). 

		If TYPE: is not specified the STRING will be treated as INI_STR.

		For INI_MSTR,white spaces will be treated as delimiters between 
		the strings. So if white spaces are part of these strings, enclose 
		them in ""(double quotes). Also if any of the strings  need to 
		have # or ; it is required to enclose them in "" or else it will 
		be treated as start of comment and the rest of line will be 
		ignored. All leading and trailing white spaces are ignored (unless
		these white spaces are enclosed in quoted)
			
		INI_STR can be either specified in quotes or without quotes. When
		specified without quoted a # or ; or EOL is treated as delimiter.
 		White Spaces within the string are treated as part of string, 
		except the leading and trailing white spaces (unless these white
		spaces are enclosed	in quotes)
	
		INI_NUM and INI_MNUM numeric values can be represented in decimal
		values, or binary (suffixed with  b or B) or octal (prefixed with 
		0) or hexadecimal (prefixed with 0x or 0X)
		
**************************************************************************/		
#define INI_TYPE_STR		0x01
#define INI_TYPE_NUM		0x02
#define INI_TYPE_MULTIPLE	0x80
#define INI_TYPE_MSTR		(INI_TYPE_MULTIPLE | INI_TYPE_STR)
#define INI_TYPE_MNUM		(INI_TYPE_MULTIPLE | INI_TYPE_NUM)


#define INI_TYPESTR_STR		"INI_STR"
#define INI_TYPESTR_NUM		"INI_NUM"
#define INI_TYPESTR_MSTR	"INI_MSTR"
#define INI_TYPESTR_MNUM	"INI_MNUM"

/*Basic INI File Funtions */
HANDLE InitIniTable(char *IniFile);
BOOL FreeIniTable(HANDLE handle);
BOOL AddIniEntry(HANDLE handle, char *sec, char *key, char *value, UINT8 type);
BOOL DelIniEntry(HANDLE handle, char *sec, char *key);

/* Special Functions */
void DumpIniTable(HANDLE handle);
BOOL WriteIniFile(HANDLE handle, char *FileName);

/* INI Access Functions */
BOOL ReadIniEntry(HANDLE handle,char *sec, char *key, char **value,UINT8 *type);
int  GetIniSectionCount(HANDLE handle);
int	 GetIniEntryCount(HANDLE handle);
char *GetIniSectionName(HANDLE handle,int Index);
char *GetIniEntryName(HANDLE handle,int Index);

#endif  

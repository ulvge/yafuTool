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
#include "config.h"
#include "console.h"
#include "rexstr.h"


/* 
  Forward the String pointer to the first non which space char 
  Returns NULL if empty (all white spaces) string 
*/
char *
RexSkipWhiteSpaces(char *Str)
{
	while (*Str)
	{
		if ((*Str != REX_KEY_SPACE) && (*Str != REX_KEY_TAB))
			break;
		Str++;
	}
	return Str;
}

/*
	Trim the trailing white spaces of a string 
	Returns NULL if empty(all white spaces) string
*/
char *
RexSkipTrailingWhiteSpaces(char *Str)
{
	char *BackStr = Str+(strlen(Str)-1);	/* Points to last char */

	while (BackStr>=Str)
	{
		if ((*BackStr != REX_KEY_SPACE) && (*BackStr != REX_KEY_TAB))
		{
			BackStr++;
			break;
		}
		BackStr--;
	}
	if (BackStr < Str)
		return NULL;
	*BackStr = 0;	
	return Str;
}

char *
RexCopyTillWhiteSpace(char *Src,char **Dest)
{
	BOOL Quot = FALSE;

	/* Any String enclosed in quotes is treted as single argument iresepecitve of spaces */
	if (*Src == '"')
	{
		Quot = TRUE;
		Src++;
	}

	*Dest = Src;	/* Set Dest to Src */

	while (*Src)
	{
		if (Quot == FALSE)
		{
			if ((*Src == REX_KEY_SPACE) || (*Src == REX_KEY_TAB))
			{
				*Src = 0;		/* Terminate Destination String */
				return Src+1;   /* Return New Src */
			}
		}
		else
		{
			if (*Src == '"')
			{
				Quot=FALSE;
				*Src = 0;		/* Terminate Destination String */
				return Src+1;   /* Return New Src */
			}

		}
		Src++;
	}

	/* Control Comes here if *Src is already 0 . In this case Dest will be same as Input Src */
	return Src;		/* Return NULL */
}


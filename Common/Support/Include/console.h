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
#ifndef REX_DBG_CONSOLE_H
#define REX_DBG_CONSOLE_H

#include "config.h"
#include "types.h"

/* Normal non-display ascii Keys returned by ConsoleGetChar*/
#define REX_KEY_BELL		'\a'
#define REX_KEY_TAB			'\t'
#define REX_KEY_BACKSPACE	'\b'
#define REX_KEY_RETURN		'\r'
#define REX_KEY_NEWLINE		'\n'
#define REX_KEY_SPACE		' '
#define	REX_KEY_ESCAPE		27 
#define REX_KEY_ASCII_DEL	127
#define REX_KEY_ESC_SEQ		'['		/* Used in Terminal Escape Sequence */


/* Special non-ascii Keys returned by ConsoleGetChar*/
#define REX_KEY_UP			0xFF00
#define REX_KEY_DOWN		0xFF01
#define REX_KEY_LEFT		0xFF02
#define REX_KEY_RIGHT		0xFF03

#define REX_KEY_PGUP		0xFF10
#define REX_KEY_PGDN		0xFF11

#define REX_KEY_DEL			0xFF20
#define REX_KEY_INS			0xFF21

#define REX_KEY_HOME		0xFF30
#define REX_KEY_END			0xFF31

#define REX_KEY_F1			0xFF80
#define REX_KEY_F2			0xFF81
#define REX_KEY_F3			0xFF82
#define REX_KEY_F4			0xFF83

/* Check the following values are correct for WIN32/DOS */
#define REX_KEY_F5			0xFF84
#define REX_KEY_F6			0xFF85
#define REX_KEY_F7			0xFF86
#define REX_KEY_F8			0xFF87
#define REX_KEY_F9			0xFF88
#define REX_KEY_F10			0xFF89
#define REX_KEY_F11			0xFF8A
#define REX_KEY_F12			0xFF8B

void	OpenConsole(BOOL raw);
void 	CloseConsole(void);
void	ConsolePutChar(UINT16 ch);
//int 	ConsolePrintf(char *fmt, ...);
#define ConsolePrintf printf
void	ConsoleBell(void);
UINT16	ConsoleGetChar(void);		
void	ConsoleClear(void);
UINT16  ConsoleCheckKey(void);	
UINT8	ConsoleIsKeyAvail(void);
/* Note: Differences between ConsoleCheckKey and ConsoleIsKeyAvail is the 
		 former returns the key if available where the later returns 1 if 
         key is available
*/

#endif  /* REX_DBG_CONSOLE_H*/

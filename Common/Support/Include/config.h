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
#ifndef REX_DBG_CONFIG_H
#define REX_DBG_CONFIG_H


/* Define this to enable command history support */
#define REX_DBG_CMD_HISTORY	

/* Configuration Parameters */
#define LINE_LEN	72				/* Maximum Line Width of console	*/
/* Note: LINE_LEN should be set so a proper value such that that the 
         command line input does not wrap to the next line. 			*/

#define TAB_SPACE	4				/* Tab Spacing						*/
#define MAX_HIST	20				/* Maximum Command History			*/
#define BYTES_PER_LINE 16			/* Number of Bytes to display in a line */
#define MAX_PATH_LEN 1024			/* Target Path Length				*/

/* Plugin Related */
#ifndef WIN32
#define PLUGIN_PATH	 "/usr/local/lib"   /* Plugin Directory for Unix Systems */
#else
/* For windows the plugin should be located in the windows system directory */
#endif

#endif

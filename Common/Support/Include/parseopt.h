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
#ifndef REX_DBG_PARSEOPT_H
#define REX_DBG_PARSEOPT_H

#include "config.h"
#include "types.h"


/* flags of ParseOptions */
#define PARSE_LONG_DOUBLE   0x01		/* --option	*/
#define PARSE_LONG_SINGLE   0x02		/* -option 	*/
#define PARSE_SHORT_DOUBLE  0x04		/* --o		*/
#define PARSE_SHORT_SINGLE  0x08		/* -o		*/

#define PARSE_LONG_MASK		(PARSE_LONG_DOUBLE  | PARSE_LONG_SINGLE)
#define PARSE_SHORT_MASK	(PARSE_SHORT_DOUBLE | PARSE_SHORT_SINGLE)

#define PARSE_DOUBLE_MASK	(PARSE_LONG_DOUBLE | PARSE_SHORT_DOUBLE)
#define PARSE_SINGLE_MASK	(PARSE_LONG_SINGLE | PARSE_SHORT_SINGLE)



/* optargs of ParseOptions is a array of this structure terminated 
   with longopt == NULL and shortopt=0 */
typedef struct 
{
	char *longopt;
	char shortopt;
	int opttype;
} OPTIONS;

/*opttype of OPTIONS */
#define  NO_ARG		0	/* No argument value follows the option 	*/
#define  OPT_ARG	1	/* No or one argument follows the option 	*/
#define  MAND_ARG	2	/* A mandatarty argument follows the option */
#define  NO_ARG_REP 3	/* No argument value, but can be specified rep */

/* value of parseOptions is a array of this structure terminated with NULL 
   for option and value. option will be a short option if availble or a  
   long option if short is not available */
typedef struct 
{
	char *option;
	char *value;
	int count;
} OPTIONS_RESULT;

/* Error codes of Parse Options */
#define PARSE_UNKNOWN_OPTION  	(-1)	/* Unsupported Option in cmdline	*/
#define	PARSE_INVALID_OPTION 	(-2)	/* Incorrect ParseOptions in cmdline*/	
#define PARSE_MISSING_ARG	  	(-3)	/* Option missing its mand argument */	
#define PARSE_REP_OPTION	  	(-4)	/* Repeating without NO_ARG_REP 	*/
#define	PARSE_DUPLICATE_OPTIONS (-101)	/* Duplicate OPTIONS parameter		*/
#define PARSE_RESULT_MEMORY		(-102)	/* Insufficient Option Result Space */
										/* This happens if the space provided											   to store the result is less than the
										   max possible options */

int ParseOptions(int argc,char **argv, int flags,OPTIONS *options,
					OPTIONS_RESULT *values, int rescount);
/* Returns a positive value indicating the number of args remaining  
   after parsing.argv is adjusted to the remaining arguments.A negative 
   value returned means error */

#endif

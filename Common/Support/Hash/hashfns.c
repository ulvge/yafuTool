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
#include "config.h"
#include "types.h"
#include "hash.h"


/* ------------------------- KR Hash Function ------------------------- */
/*						hash(i) = hash(i-1) * 31 +str(i)				*/
/*    This appeared in K&R (Ist Edition) and hence became popular 		*/
/*    because of its simplicity(!). But it does the worst hashing       */
/* ---------------------------------------------------------------------*/
UINT32 
KRHash(char* str, UINT32 len)
{
   UINT32  hash = 0;
   UINT32  seed = 31; /* 31 131 1313 13131 131313 .....*/
   UINT32  i    = 0;

   for(i = 0; i < len; str++, i++)
      hash = (hash*seed)+(*str);

   return hash;
}

/* -------------------------UNIX ELF Hash Function ---------------------- */
/*						hash(i) = hash(i-1)* 16 +str(i)					  */
/*     Published UNIX ELF Hash algorithm used for Unix Object Files       */
/* ---------------------------------------------------------------------- */
UINT32  
ELFHash(char* str, UINT32  len)
{
   UINT32  hash 	= 0;
   UINT32  overflow = 0;
   UINT32  i    	= 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = (hash << 4) + (*str);
      overflow = (hash & (UINT32)0xF0000000);
	  if (overflow)
      {
         hash ^= (overflow >> 24);
         hash &= (~overflow);
      }
   }

   return hash;
}

/*--------------------  Peter J. Weinberger Hash Function ---------------*/
/* 				  Almost similiar to Unix ELF Hashing method 			 */
/* There are too many derivations and misprints of this method. Hence no */
/* 		one knows the actual implementation of this function 			 */
/*-----------------------------------------------------------------------*/
UINT32 
PJWHash(char* str, UINT32 len)
{
   UINT32 hash             = 0;
   UINT32 overflow         = 0;
   UINT32 i                = 0;

   	for(i = 0; i < len; str++, i++)
   	{
    	hash = (hash << 4) + (*str);		/* 1/8 Bits of UINT32 */

		/* Check if overflow to High 4 Bits of UINT32*/
      	overflow = (hash & (UINT32)0xF0000000);
	  	if (overflow)
      	{
         	hash ^= (overflow >> 28);	/* Many versions misprint it as 24 */ 			
			hash ^= overflow;			/* Some versions hash &= (~0xF0000000) */
      	}
   	}
	
	/*hash  = hash % 211;*/			/* Few versions does this also */
	return hash;
}


/* ------------------------- SDBM Hash Function ------------------------- */
/*					 hash(i) = hash(i-1) * 65599 + str(i) 				  */	
/*			 Used in Berkeley DB (Sleepy cat) Implementations 			  */
/* -----------------------------------------------------------------------*/
UINT32  
SDBMHash(char* str, UINT32 len)
{
   UINT32  hash = 0;
   UINT32  i    = 0;

   for(i = 0; i < len; str++, i++)
      hash = (*str) + (hash << 6) + (hash << 16) - hash;

   return hash;
}


/* ------------------------- DJB2 Hash Function ------------------------ */
/*					 hash(i) = hash(i-1) * 33 + str(i) 				     */	
/*	           This is one of the best string hashing method  			 */
/*                   Both speedwise and distribution wise			 	 */
/*                           Dan J. Bernstein							 */ 
/* ----------------------------------------------------------------------*/
UINT32  
DJB2Hash(char* str, UINT32  len)
{
   UINT32  hash = 5381;
   UINT32  i    = 0;

   for(i = 0; i < len; str++, i++)
      hash = ((hash << 5) + hash) + (*str);

   return hash;
}


/*-----------------------------------------------------------------------*/


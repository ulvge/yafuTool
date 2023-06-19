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
#ifndef REX_DBG_CONVERT_H
#define REX_DBG_CONVERT_H

#include "config.h"
#include "types.h"

BOOL Unsigned2Str(UINT32 Value, char *Asc, UINT16 AscLen, BOOL Qual, UINT16 Radix);
BOOL Signed2Str(SINT32 Value, char *Asc, UINT16 AscLen, BOOL Qual, UINT16 Radix);

BOOL Str2Unsigned(char *Str,UINT32 *Value);
BOOL Str2Signed(char *Str, SINT32 *Value);

/*
#define Value2HexStr(Value,Asc,AscLen,Qual) Unsigned2Str(Value,Asc,AscLen,Qual,16)
#define Value2OctStr(Value,Asc,AscLen,Qual) Unsigned2Str(Value,Asc,AscLen,Qual,8)
#define Value2BinStr(Value,Asc,AscLen,Qual) Unsigned2Str(Value,Asc,AscLen,Qual,2)
#define Value2DecStr(Value,Asc,AscLen,Qual) Unsigned2Str(Value,Asc,AscLen,Qual,10)
*/



#endif  /* REX_DBG_CONVERT_H*/

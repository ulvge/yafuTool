/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2002-2003, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/****************************************************************
  $Header: $

  $Revision: $

  $Date: $
 *****************************************************************/
/*****************************************************************
 *
 * debug.h
 * Debug message handler functions
 *
 *  Author: Basavaraj Astekar <basavaraja@ami.com>
 *
 *****************************************************************/
#ifndef DEBUG_H
#define DEBUG_H
#include <stdio.h>
#include <stdlib.h>
#include "err_codes.h"

#define DBG_MSG 

#ifdef DBG_MSG

/* Debug levels */
#define DBG_ALL     0
#define DBG_LVL1    (DBG_ALL + 1)
#define DBG_LVL2    (DBG_LVL1 + 1)
#define DBG_LVL3    (DBG_LVL2 + 1)
#define DBG_LVL4    (DBG_LVL3 + 1)
#define DBG_LVL_MAX DBG_LVL4
#define DBG_LVL DBG_LVL2

#define DBG_INIT_MSG()                           printf ("Debug messages enabled, debug level - %d\n", DBG_LVL);
#define DBG_PRINT(lvl, fmt)                      (lvl > DBG_LVL) ? printf ("") : printf ("DBG-%1d: "fmt, lvl)
#define DBG_PRINT1(lvl, fmt, arg1)               (lvl > DBG_LVL) ? printf ("") : printf ("DBG-%1d: "fmt, lvl, arg1)
#define DBG_PRINT2(lvl, fmt, arg1, arg2)         (lvl > DBG_LVL) ? printf ("") : printf ("DBG-%1d: "fmt, lvl, arg1, arg2)
#define DBG_PRINT3(lvl, fmt, arg1, arg2, arg3)   (lvl > DBG_LVL) ? printf ("") : printf ("DBG-%1d: "fmt, lvl, arg1, arg2, arg3)

#else

#define DBG_INIT_MSG()
#define DBG_PRINT(lvl, fmt)
#define DBG_PRINT1(lvl, fmt, arg1)
#define DBG_PRINT2(lvl, fmt, arg1, arg2)
#define DBG_PRINT3(lvl, fmt, arg1, arg2, arg3)

#endif

#endif /* DBEUG_H */

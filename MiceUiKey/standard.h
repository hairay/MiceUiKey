/******************************************************************************
*         Copyright (c) 2002 Oak Technology, Inc. All rights reserved.
*
* Author:            Mark V. Dobrosielski
*
* Description:       Header file containing some standard constant and type
*                    definitions.
*
* Revision History:
* Date      Author   Description
* ----      ------   -----------
* 7/21/98   MMP      Added YES & NO #defines.
* 5/24/99   Dobro    Updated header notes.
* 8/31/99   Dobro    Initial release (based on PMIC100).
* 8/12/02   Dobro    Initial Quatro release, with updated copyright line. 
******************************************************************************/
#ifndef __STANDARD
#define __STANDARD

/******************************************************************************
                       constants/enums/typedefinitions
******************************************************************************/
typedef unsigned long   Uint32;
typedef unsigned int    Uint;
typedef signed long     Sint32;
typedef unsigned short  Uint16;
typedef signed short    Sint16;
typedef unsigned char   Uint8;
typedef signed char     Sint8;
typedef void*           ObjID;
typedef long long		Sint64;
typedef unsigned long long 	Uint64;

// nucleus define that we may wish to use even if we aren't using nucleus
#ifndef UNSIGNED
typedef unsigned long UNSIGNED;
#endif

typedef void ISR (void);

typedef int Bool ;
#define FALSE 0
#define TRUE 1
typedef Bool             Boolean;
typedef Bool             BOOL;
//void LcdCenter(unsigned char row,char *s);
//typedef enum
//{
//   API_OK = 0,
//   API_FAIL
//} AvApiRet;

#define BIT8(n)   ((Uint8) (1 << n))
#define BIT16(n)  ((Uint16) (1 << n))
#define BIT32(n)  ((Uint32) (1 << n))

#define MAX(a,b)	((a) > (b) ? (a) : (b))
#define MIN(a,b)	((a) < (b) ? (a) : (b))

#define LOCAL     static
#define EXTERN    extern
#define FOREVER   while(TRUE)


#ifndef NULL
#define NULL (void *)0
#endif
/******************************************************************************
                                structures
******************************************************************************/

/******************************************************************************
                               prototypes
******************************************************************************/

/******************************************************************************
                                externs
******************************************************************************/

#endif



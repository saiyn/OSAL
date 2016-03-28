#ifndef _SYS_HEAD_H_
#define _SYS_HEAD_H_

#include "stdlib.h"
#include "stdbool.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>


typedef   unsigned int     uint32;
typedef   unsigned short   uint16;
typedef   unsigned char    uint8;

typedef   unsigned int     UINT32;
typedef   unsigned short   UINT16;
typedef   unsigned char    UINT8;


typedef   unsigned long    uint64_t;
typedef   unsigned int     uint32_t;
typedef   int              int32_t;
typedef   unsigned short   uint16_t;
typedef   unsigned char    uint8_t;

typedef   unsigned int     size_t;

typedef unsigned           int uint_fast8_t;
typedef unsigned           int uint_fast16_t;
typedef unsigned           int uint_fast32_t;
typedef unsigned       __int64 uint_fast64_t;

typedef unsigned long      DWORD;
typedef unsigned short     WORD;
typedef unsigned char      BYTE;

typedef union{
	WORD val;
	BYTE v[2];
	struct{
		BYTE LB;
		BYTE HB;
	}byte;
	struct{
		 BYTE b0:1;
		 BYTE b1:1;
		 BYTE b2:1;
		 BYTE b3:1;
		 BYTE b4:1;
		 BYTE b5:1;
		 BYTE b6:1;
		 BYTE b7:1;
		 BYTE b8:1;
		 BYTE b9:1;
		 BYTE b10:1;
		 BYTE b11:1;
		 BYTE b12:1;
		 BYTE b13:1;
		 BYTE b14:1;
		 BYTE b15:1;
	}bits;
}WORD_VAL;


typedef union{
	DWORD val;
	WORD  w[2];
	BYTE  v[4];
	struct{
		WORD LW;
		WORD HW;
	}word;
	struct{
		BYTE LB;
		BYTE HB;
		BYTE UB;
		BYTE MB;
	}byte;
	struct{
		WORD_VAL low;
		WORD_VAL high;
	}wordUnion;
	struct{
	   BYTE b0:1;
		 BYTE b1:1;
		 BYTE b2:1;
		 BYTE b3:1;
		 BYTE b4:1;
		 BYTE b5:1;
		 BYTE b6:1;
		 BYTE b7:1;
		 BYTE b8:1;
		 BYTE b9:1;
		 BYTE b10:1;
		 BYTE b11:1;
		 BYTE b12:1;
		 BYTE b13:1;
		 BYTE b14:1;
		 BYTE b15:1;
		 BYTE b16:1;
		 BYTE b17:1;
		 BYTE b18:1;
		 BYTE b19:1;
		 BYTE b20:1;
		 BYTE b21:1;
		 BYTE b22:1;
		 BYTE b23:1;
		 BYTE b24:1;
		 BYTE b25:1;
		 BYTE b26:1;
		 BYTE b27:1;
		 BYTE b28:1;
		 BYTE b29:1;
		 BYTE b30:1;
		 BYTE b31:1;
	}bits;
	
}DWORD_VAL;



typedef enum
{
	   OFF = 0,
	   ON
}swicth_state_t;

#ifndef NULL
#define NULL    ((void*)0)    
#endif

#define NOP()			__asm volatile (\
							" nop \n" \
						)

#if 1
extern void s_printf(char *fmt, ...);
#define SYS_TRACE s_printf
#else
#define SYS_TRACE(...)
#endif


#if 1
#define OSAL_ASSERT(x) \
if(!(x)){\
	SYS_TRACE("(%s) has assert failed at %s.\n", #x, __FUNCTION__);\
  while(1);\
}
#else

#define OSAL_ASSERT(...)

#endif


#include "sysctl.h"
#include "systick.h"
#include "pin_map.h"
#include "gpio.h"
#include "uart.h"
#include "adc.h"
#include "ssi.h"
#include "timer.h"
#include "hw_gpio.h"
#include "pwm.h"
#include "hw_memmap.h"
#include "hw_types.h"
//#include "hw_ints.h"
#include "udma.h"
#include "hw_ssi.h"
#include "interrupt.h"
#include "tm4c1290nczad.h"
#include "fpu.h"

extern uint32 gSysClock;

extern uint32 gTickCount;


#include "sys_database.h"
#include "sys_config.h"
#include "bsp.h"


#endif




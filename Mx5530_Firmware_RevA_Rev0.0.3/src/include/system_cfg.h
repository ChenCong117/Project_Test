#ifndef _SYSTEM_CFG_H
#define _SYSTEM_CFG_H

/**
 * system clock define
 */

#define SYS_CLK_MHZ					27 		// default clock freq...

/**
 * firmware version:
 Major: 

 Middle:

 Minor: 

_Debug: 
	Support modify paramters without re-download firmware(only need reset mcu).
 */
#define MAJOR			0  		// 4-bit: 0 - 15  (0 is debug version)
#define MIDLE			0		// 4-bit: 0 - 15
#define MINOR			3		// 8-bit: 1 - 255
//
// Version released to:	0.1.2		   
//
// construct firmware version:
#define SYS_FW_VER				((MAJOR<<12)|(MIDLE<<8)|(MINOR))

/**
 * interrupt number define
 */									
#define INT_NUM_0_EX0				0
#define INT_NUM_1_TIMER0			1																   
#define INT_NUM_2_EX1				2
#define INT_NUM_3_TIMER1			3
#define INT_NUM_4_UART				4
#define INT_NUM_5_TOTAL				5

#endif
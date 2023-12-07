#ifndef _ERR_DEF_H
#define _ERR_DEF_H

/**
 * Major error code indicate the register address actually, we use 22 registers
 * to trace module error... 
 * Minor error code to be defined in module itself...
 */
// inclusive modules:
#define ERR_MAJOR_SCHED			0
#define ERR_MAJOR_EVENT			1
#define ERR_MAJOR_INTERRUPT		2
#define ERR_MAJOR_SYSTEM		3
#define ERR_MAJOR_DEBUG			4
// functional modules:
#define ERR_MAJOR_DPRX			5
#define ERR_MAJOR_HDMITX		6
// assist modules:
#define ERR_MAJOR_EDID			7
// drivers:
#define ERR_MAJOR_IIC_MASTER	8
#define ERR_MAJOR_TIMER			9
#define ERR_MAJOR_GPIO			10
#define ERR_MAJOR_UART			11
// number gate:
#define ERR_MAJOR_NUMBER		12


#endif
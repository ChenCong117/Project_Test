#ifndef _REG_DEF_H
#define _REG_DEF_H

#include "compile_cfg.h" 
/**
 * using general register define
 */
#include <reg51.h>

// 0x0000 - 0x00FF: Reserved

// 0x0100 - 0x01FF: reset/clock_gate
#define RSTCLK_REG_BASE			0x0100
#define regmap01(n)				XBYTE[0x0100 + (n)]	

// 0x0200 - 0x02FF: Interrupt control
#define INTCTRL_REG_BASE		0x0200
#define regmap02(n)				XBYTE[0x0200 + (n)]	

// 0x0300 - 0x03FF: MISC
#define MISC_REG_BASE			0x0300
#define regmap03(n)				XBYTE[0x0300 + (n)]	

// 0x0400 - 0x04FF: auto-loading
#define AUTOLOAD_REG_BASE		0x0400
#define regmap04(n)				XBYTE[0x0400 + (n)]

// 0x0500 - 0x05FF: IIC_HW master
#define IICHW_REG_BASE			0x0500
#define regmap05(n)				XBYTE[0x0500 + (n)]

// 0x0600 - 0x06FF: IIC_SW master

// 0x0700 - 0x07FF: IIC_MTP module

// 0x0800 - 0x08FF: global bus
#define GLBBUS_REG_BASE			0x0800
#define regmap08(n)				XBYTE[0x0800 + (n)]	

// 0x0900 - 0x09FF: analog control
#define ANALOG_REG_BASE			0x0900	
#define regmap09(n)				XBYTE[0x0900 + (n)]

// 0x0A00 - 0x0AFF: iic clock domain 
#define IICCLK_REG_BASE			0x0A00
#define regmap0A(n)				XBYTE[0x0A00 + (n)]
	
// 0x0B00 - 0x0EFF: USB Type-C module (4 pages)
#define USBPD_REG_BASE			0x0B00
#define regmap0B(n)				XBYTE[0x0B00 + (n)]
#define regmap0C(n)				XBYTE[0x0C00 + (n)]
#define regmap0D(n)				XBYTE[0x0D00 + (n)]
#define regmap0E(n)				XBYTE[0x0E00 + (n)]	 

// 0x0F00 - 0x0FFF: FCP module
#define FCPSCP_REG_BASE			0x0F00
#define regmap0F(n)				XBYTE[0x0F00 + (n)]

// 0x1000 - 0x10FF: Analog Write
#define ANAWR_REG_BASE			0x1000
#define regmap10(n)				XBYTE[0x1000 + (n)]

// 0x1100 - 0x11FF: Analog Read
#define ANARD_REG_BASE			0x1100
#define regmap11(n)				XBYTE[0x1100 + (n)]

// 0x1200 - 0x12FF: OTP Memory
#define OTPMEM_REG_BASE			0x1200
#define regmap12(n)				XBYTE[0x1200 + (n)]

// 0x1300 - 0x3FFF: Gap - reserved

// 0x4000 - 0x4BFF: general xram memory
#define GENMEM_REG_BASE 		0x4000
#define GENMEM_REG_LENGTH		0x0800

// 0x4C00 - 0x4EFF: Gap - reserved

// 0xFF00 - 0xFFFF: iic slave download firmware
#define IIC_LOADFW_REG_BASE		0xFF00
#define regmapFF(n)				XBYTE[0xFF00 + (n)]	







#endif
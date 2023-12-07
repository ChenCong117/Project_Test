#ifndef _SW_IIC_H
#define _SW_IIC_H

#include "includes.h"

#if (DRV_SW_IIC_OPEN == YES)

// software iic isr, to be called from general isr...
void iic_isr(void);

// init iic to default setting: target = local, speed = 100K
void iic_init(void);

/**
 * guide for how to use above interface:
 *
 * read: 
 * 		1) iic_prepare(target, speed);
 * 		2) iic_start_send(device_address);
 *		3) iic_step_send(offset) // when offset is 2 bytes, repeat this step: one for high byte, another for low byte
 *		4) iic_restart_send(device_address);
 *		5) read_data = iic_step_read(); // repeat this step until to last byte
 *		6) read_data = iic_final_read(); 
 * write:
 * 		1) iic_prepare(target, speed);
 * 		2) iic_start_send(device_address);
 *		3) iic_step_send(offset) // when offset is 2 bytes, repeat this step: one for high byte, another for low byte
 *		4) iic_step_send(); // repeat this step until to last byte
 *		6) iic_final_send(); 		
 */

// prepare for read/write operation: must be called at beginning...
// target: only accept below values:
#define IIC_TGT_LOCAL		0
#define IIC_TGT_HDMIDDC		1
// speed: only accept below values:
#define IIC_SPEED_100K		0
#define IIC_SPEED_400K		1
void iic_prepare(uint8 target, uint8 speed);

// puclic interface:
void iic_start_send(uint8 byte);
void iic_restart_send(uint8 byte);

// for read operation:
uint8 iic_step_read(void);
uint8 iic_final_read(void);

// for write operation:
void iic_step_send(uint8 byte);
void iic_final_send(uint8 byte);

// here, we provide whole interface for read/write:
// note: iic_prepare() is still to be called first...
#define WHOLE_VERSION_SUPPORT		NO
#if (WHOLE_VERSION_SUPPORT == YES)
void iic_whole_read(uint8 address, uint8 offset, uint8 *values, uint8 length);
void iic_whole_read2(uint8 address, uint16 offset, uint8 *values, uint8 length);
void iic_whole_send(uint8 address, uint8 offset, uint8 *values, uint8 length);
void iic_whole_send2(uint8 address, uint16 offset, uint8 *values, uint8 length);
#endif

// test function:
#define IIC_TEST_EN		NO
#if (IIC_TEST_EN == YES)
void iic_test(void);
#endif

#endif  // (DRV_SW_IIC_OPEN == YES)

#endif	
#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include "includes.h"

/**
 * interrupt bit define: '0' or '1' following module name indicate that
 * the corresponding bit belong to interrupt 0 or interrupt 1 respectively...
 */

// MCU_INT0_STA & MCU_INT0_STA_MASK bits:
#define EX0_CEC_INT0_ACT			(1 << 7)
#define EX0_USB2_INT0_ACT			(1 << 6)
#define EX0_AUDIO_INT0_ACT			(1 << 5)
#define EX0_HMTX_INT0_ACT			(1 << 4)
#define EX0_DPRX_INT2_ACT			(1 << 3)
#define EX0_DPRX_INT1_ACT			(1 << 2)
#define EX0_DPRX_INT0_ACT			(1 << 1)
#define EX0_HOSTIF_INT0_ACT			(1 << 0)

// MCU_INT1_STA & MCU_INT1_STA_MASK bits:
#define EX1_HWACC_INT0_ACT			(1 << 7)
#define EX1_RSA_INT0_ACT			(1 << 6)
#define EX1_CEC_INT1_ACT			(1 << 5)
#define EX1_DPRX_INT3_ACT			(1 << 4)
#define EX1_USB_TYPEC_ACT			(1 << 3)
#define EX1_HMTX_INT2_ACT			(1 << 2)
#define EX1_HMTX_INT1_ACT			(1 << 1)
#define EX1_HOSTIF_INT1_ACT			(1 << 0) 

// HOSTIF_INT0_STA & HOSTIF_INT0_STA_MASK bits:
#define HIF_INT0_RR_TIMEOUT			(1 << 6)  	// RR_TIMEOUT_IRQ
#define HIF_INT0_RR					(1 << 5)	// RR_IRQ
#define HIF_INT0_FLASH_OP_END		(1 << 4)
#define HIF_INT0_IICHW_FINISH		(1 << 3)
#define HIF_INT0_IICHW_WB_FINISH	(1 << 2)
#define HIF_INT0_IICHW_RD_FINISH	(1 << 1)
#define HIF_INT0_IICSW_OP_END		(1 << 0)

// HOSTIF_INT1_STA & HOSTIF_INT1_STA_MASK bits:
#define HIF_INT1_IIC_SLV			(1 << 1)
#define HIF_INT1_DDC_REG			(1 << 0)

/**
 * init interrupt...
 */
void interrupt_init(void);


#endif
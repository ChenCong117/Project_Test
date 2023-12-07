#ifndef _COMPILE_CFG_H
#define _COMPILE_CFG_H

/**
 * refer to <<CT367Èí¼þÉè¼Æ³õ¸åv0.3.pdf>...
 *
 * note: 
 * 		we would use #if (XXX == YES) not #ifdef XXX
 *		so...													
 *		when module to be included,  #define XXX 	YES
 *		when module NOT be included, #define XXX	NO
 */

#define YES		1

#define NO		0

/** 
 * always inclusive modules, unless you really not need... for example, 
 * you just want to test one special module...
 */
#define IMOD_SCHED_OPEN			1		// scheduler

#define IMOD_EVENT_OPEN			1		// event processor

#define IMOD_INTERRUPT_OPEN		1		// interrupt processor

#define IMOD_SYSTEM_OPEN		1		// system misc...
  

/**
 * Function modules...
 */

#define FMOD_USB_PD_OPEN		1		// USB Type-C Power Delivery 

/**
 * Assist modules...
 */
#define AMOD_SW_DELAY_OPEN		1		// software Delay

/**
 * Drivers...
 */
#define DRV_SW_IIC_OPEN			0		// SW_IIC, not exist...

#define DRV_TIMER_OPEN			1		// Timer

#define DRV_POWER_OPEN			1		// POWER


/**									
 * for test
 */

#define TST_ENABLE		 		0		// Total gate...




#endif
#ifndef _TIMER_H
#define _TIMER_H


#include "includes.h"

#if (DRV_TIMER_OPEN == YES)

// system time ticks (1 tick = 1 ms)
extern uint32 data system_ticks; 

/**
 * to be system clock source
 */
void timer_init(void);

/** 
 * For polling function...
 */
typedef enum {
	// connection: 
	TP_USBPD_CONNECTION = 0,
	// gate:
	TP_TYPE_MAX
} TMR_POLL_TYPE;  
void timer_polling_enable(uint8 enable, TMR_POLL_TYPE type);

#define TIMER_TEST_EN		NO
#if (TIMER_TEST_EN == YES)
void timer_test(void);
#endif

	
#endif // (DRV_TIMER_OPEN == YES)


#endif /* __TIMER_H__ */


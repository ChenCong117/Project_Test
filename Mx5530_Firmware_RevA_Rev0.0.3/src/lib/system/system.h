#ifndef _SYSTEM_H
#define _SYSTEM_H

#include "includes.h"

#if (IMOD_SYSTEM_OPEN == YES)

// analog settings:
void system_init(void);

// system clock switch:
// target: 
//			0 - switch to 27MHz
//			1 - switch to 108MHz
void system_clock_switch(uint8 target);
extern uint8 data cur_system_clock;


#endif

#endif /* __SCALER_H__ */

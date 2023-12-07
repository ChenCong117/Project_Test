
#include "includes.h"

//scheduler:
#if (IMOD_SCHED_OPEN == YES)
#include "sched.h"
#endif

//function modules:
#if (FMOD_USB_PD_OPEN == YES)
#include "usb_pd/usbpd/usb_pd.h"
#endif

//event process:
#if (IMOD_EVENT_OPEN == YES)
#include "event.h"
#endif

//assist modules:
#if (IMOD_INTERRUPT_OPEN == YES)
#include "interrupt/interrupt.h"
#endif
#if (IMOD_SYSTEM_OPEN == YES)
#include "system/system.h"
#endif
#if (AMOD_SW_DELAY_OPEN == YES)
#include "sw_delay/sw_delay.h"
#endif

//init drivers:
#if (DRV_SW_IIC_OPEN == YES)
#include "sw_iic/sw_iic.h"
#endif
#if (DRV_TIMER_OPEN == YES) 
#include "timer/timer.h"
#endif
#if (DRV_TIMER_OPEN == YES)
#include "timer/timer.h"
#endif
#if (DRV_POWER_OPEN == YES)
#include "power/power.h"
#endif

//for test:
#if (TST_ENABLE	== YES)
extern void test_main(void);
#endif

static uint8 xdata MEM_INIT_FLAG	_at_	(GENMEM_REG_BASE + 0x00);	// reserved
static uint8 xdata MCU_OVER_FLAG	_at_ 	(GENMEM_REG_BASE + 0x01); 	// indicator that firmware run over...
static uint8 xdata FW_VERSION1		_at_ 	(GENMEM_REG_BASE + 0x02);	// firmware version
static uint8 xdata FW_VERSION2		_at_	(GENMEM_REG_BASE + 0x03); 	// reserved

// Entry...
int main()
{ 
	//version init...
	FW_VERSION1 = (SYS_FW_VER >> 8) & 0xFF;
	FW_VERSION2 = (SYS_FW_VER >> 0) & 0xFF;

	// event process module eaarlier as some init routine will register event maybe...
	#if (IMOD_EVENT_OPEN == YES)
	event_initialize();
	#endif

	//open interrupt first, as test_main will use interrupt sometimes...
	#if (IMOD_INTERRUPT_OPEN == YES)
	interrupt_init();  
	#endif

	//system init as it affect system status:
	#if (IMOD_SYSTEM_OPEN == YES)
	system_init();
	#endif

	//system clock open as real task is beginning...
	#if (DRV_TIMER_OPEN == YES)
	timer_init();
	#endif

	//As it's test, we could move this to where we want....
	#if (TST_ENABLE == YES)
	test_main(); 
	#endif

	//we have to init something first...

	//system init: from low-layer to top-layer...
	//drivers...
	#if (DRV_SW_IIC_OPEN == YES)
	iic_init();
	#endif
	#if (DRV_POWER_OPEN == YES)
	power_init(); 
	#endif
	//libraries...
	//function modules...
	#if (FMOD_USB_PD_OPEN == YES)
	usb_pd_init();
	#endif

	//any necessary operation here...

	//use scheduler to enter main loop...
	#if (IMOD_SCHED_OPEN == YES)
	scheduler();
	#endif

	//spin forever...
	MCU_OVER_FLAG = 0;	// for avoiding simulation warning...
	while(1)
	{
		MCU_OVER_FLAG++;
	}
	
	return 0;
}

/*
void init_memory(void)
{
	// check memory has been initialized:
	if(MEM_INIT_FLAG == 0)
		return;

	// init memory values to zero:
	memset(&XBYTE[GENMEM_REG_BASE], 0, 0x20);
}
*/



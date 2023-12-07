/**
 * timer driver code.
 * note: timer0 to be the system timer, timer1 to be used by uart, so the timer is only 
 *  	 mean timer0, eg, timer_isr() is isr for timer0, there's nothing about timer1 here...
 */
#include "timer.h"
#include "event.h"
#include "system/system.h"
#include "usb_pd/usbpd/usb_pd.h"

#if (DRV_TIMER_OPEN == YES)

// 1ms period: 32-bit value will not overflow...
uint32 data system_ticks; 

/**
 * time = (0x10000 - init_value) * machine_cycle
 * machine_cycle = clock_cycle * 2 
 * clock_cycle = 1 / clock_freq
 * time = (0x10000 - init_value) * ((1 / clock_freq) * 2)
 * 0x10000 - init_value = time / ((1 / clock_freq) * 2)
 * init_value = 0x10000 - time / (2 / clock_freq)
 * init_value = 0x10000 - time * clock_freq / 2
 */
//time = 1ms:
#define RELOAD_VALUE			(0x10000 - (SYS_CLK_MHZ * 500))	

// reload values(1ms) = 0x10000 - FREQ_OSC / 2 = 65536 - 27000 / 2 = 52036 = 0xCB44
//#define RELOAD_VAL_L	(0x44 + 0x00)  // maybe need some adjust...
//#define RELOAD_VAL_H	(0xCB)
// reload values(1ms) = 0x10000 - 50000 / 2 = 65536 - 25000 = 40536 = 0x9E58
//#define RELOAD_VAL_L	(0x58 + 0x00) 
//#define RELOAD_VAL_H	(0x9E)

// timer polling machinism: 
static uint16 probe_status; 
static uint32 xdata start_ticks[TP_TYPE_MAX]; 
void timer_polling_enable(uint8 enable, TMR_POLL_TYPE type)
{
	if(enable)
	{
		probe_status |=  ((uint16)1 << (uint8)type); 
		start_ticks[(uint8)type] = system_ticks - 1; 
	}
	else
	{
		probe_status &= ~((uint16)1 << (uint8)type); 		
	}	
}

static uint8 xdata timer_running_flag		_at_		(GENMEM_REG_BASE + 0x70); 
void timer_isr(void) interrupt INT_NUM_1_TIMER0
{
	// reload values:
	TR0 = 0;
	// we have to check load values:
	{
		TL0 = (RELOAD_VALUE >> 0) & 0xFF;
		TH0 = (RELOAD_VALUE >> 8) & 0xFF; 
	}
	TR0 = 1;

	// ticks:
	system_ticks++;

	// just for debug...
	timer_running_flag += 1; 

	// test timer:

	// probe connection status......which should be interrupt???
	#if 1
	if(probe_status & (1 << TP_USBPD_CONNECTION))
	{
		if((system_ticks & 0xF) == 0) // 15ms -  every 512ms...
		{
			// check if disconnected...
			usb_pd_check_connect(1); 
		}
	}
	#endif
}


void timer_init(void)
{
	// software init:
	system_ticks = 0;	
	probe_status = 0;

	// put timer0 into 16-bit no prescale (method - 1)
	TMOD = (TMOD & 0xF0) | 0x01;  
	
	// set peroid:
	TL0 = (RELOAD_VALUE >> 0) & 0xFF;
	TH0 = (RELOAD_VALUE >> 8) & 0xFF;

	// start timer at last:
	TR0 = 1;     
}

//////////////////////////////// test function //////////////////////////////////
#if (TIMER_TEST_EN == YES)
void timer_test(void)
{
	while(1)
	{
	}
}
#endif



#endif /* DRV_TIMER_OPEN */
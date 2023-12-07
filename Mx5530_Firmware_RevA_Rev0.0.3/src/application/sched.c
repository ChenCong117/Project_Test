
#include "sched.h"
#include "event.h"

#if (IMOD_SCHED_OPEN == YES)

//function modules:
#if (FMOD_USB_PD_OPEN == YES)
#include "usb_pd/usbpd/usb_pd.h"
#endif

/**
 * define some varaibles to trace program running...
 */
uint8 xdata g_evt_des_not_enough		_at_	(GENMEM_REG_BASE + 0x0004);
static uint8 xdata f_running_flag		_at_	(GENMEM_REG_BASE + 0x0005);
static uint8 xdata f_last_evt_type		_at_	(GENMEM_REG_BASE + 0x0006);
static uint8 xdata f_last_evt_param		_at_	(GENMEM_REG_BASE + 0x0007);

//trace event number for each module: 
static uint8 xdata usbpd_event_num		_at_	(GENMEM_REG_BASE + 0x0008); 

void scheduler(void)
{
	struct event_des evt_des;
	
	usbpd_event_num = 0;
		
	//loop forever...
	while(1)
	{
		//trace if running...
		f_running_flag++;

		//give chance to call back...
		#if (CB_FUNC_ENABLE == YES)
		traverse_call_back_list();
		#endif

		//check event...
		if(!event_pending(&evt_des))
			continue;
		
		//trace event...
		f_last_evt_type = evt_des.type;
		f_last_evt_param = evt_des.param;

		//event pending! process it:
		if(evt_des.type >= EVT_UN_DEF)
			continue;

		// usb pd:
		#if (FMOD_USB_PD_OPEN == YES)
		else if(evt_des.type == EVT_USB_PD)
		{
			usbpd_event_num++;
			usb_pd_process(evt_des.param);
		}
		#endif

		else
		{
			//do nothing...
		}
	}
}


/**************************** end *****************************/
#endif



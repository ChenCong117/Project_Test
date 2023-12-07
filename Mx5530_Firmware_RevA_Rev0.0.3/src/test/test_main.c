
#include "includes.h"

#if (TST_ENABLE == YES)

#include "timer/timer.h"
#include "usb_pd/usbpd/usb_pd.h"

void test_main()
{
	usb_pd_test();
	
	//spin forever...
	while(1)
	{
		XBYTE[GENMEM_REG_BASE + 0x01]++;
	}
}

#endif //#if (TEST_ENABLE == YES)
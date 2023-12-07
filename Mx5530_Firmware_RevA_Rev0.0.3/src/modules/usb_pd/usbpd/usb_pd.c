
#include "usb_pd.h"
#include "sw_delay/sw_delay.h"
#include "event.h"
#include "timer/timer.h"

#if (FMOD_USB_PD_OPEN == YES)
#include "usbpd_protocol.h"
#include "usbpd_policy.h"

// control if to reply goodcrc in isr...
#define REPLY_GOODCRC_IN_ISR			0	  // set to 0 in default...

// timer status:
static uint8 xdata usbpd_timer_bits[3];

// event status:
static uint16 xdata usbpd_event_bits; 

// public debug resgiers:
uint8 xdata g_usbpd_trace[16]	_at_	(GENMEM_REG_BASE + 0x50); 
 
// check attach status:
#define CC_VALID_Rd		1
#define CC_VALID_Ra		2
static void _check_cc_src_connection(uint8 bisr);  

// if using hardware to detect cc voltage for connection detection, 
// we have to set this to update the debounce time, which in ms...
#define DET_PLUG_MS			50   // 100 - 200 in spec, but for improv compatibility...
#define DET_UNPLUG_MS		5	 // 0 - 20 in spec
#define SET_SRC_DET_DEBOUNCE(ms) do{;}while(0)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////	

// attention: this routine will be called from system isr, so be careful...
void usb_pd_isr1(void)
{
	uint8 data bmc_rx_status = regmap0B(0x01);
	uint8 data bmc_tx_status = regmap0B(0x02);
	uint8 data timer0_status = regmap0B(0x03);
	uint8 data timer1_status = regmap0B(0x04);
	uint8 data timer2_status = regmap0B(0x05);
	uint8 data cc_det_status = regmap0B(0x0C);

	////////////////////// bmc rx interrupt //////////////////////////
	if(bmc_rx_status & BIT_RX_RCV_MESSAGE)
	{
		// if policy is not ready, we no process the message...
		if(current_connect_status & 0x80)
		{
			// judge if message valid:
			if((regmap0B(0x21) & 0xC0) == 0x40)
			{
				// message received:
				// update system status...
				regmap10(0x0D) |= (1 << 0); // PD_WORK = 1
				// check if GoodCRC message recieved:
				if(!usbpd_check_goodcrc())
				{
					// we should start timer for 195us counter...but we set to 190us...
					usbpd_us_timer_start(0, 190);
					// reply goodcrc here...??? or in usbpd_message_reception()...
					#if (REPLY_GOODCRC_IN_ISR == 1)
					if(usbpd_reply_goodcrc()) 
					#endif
					{
						// register event for messages except GoodCRC...
						event_add_tail(1, EVT_USB_PD, PBIT_USBPD_PRLMSG_RECEIVED);
					}  
				}
			}
		}
	}
	if(bmc_rx_status & BIT_RX_RCV_HARDRST)
	{
		// protocol process:
		usbpd_hardreset_received(); 
		// if not connected, ignore it...
		if(current_connect_status & 0x80)
		{
			// register event:
			usb_pd_add_event(1, USBPD_EVT_HARDRST_RECEIVED); 
		} 
	}

	////////////////////////// bmc tx interrupt //////////////////////////////
	if(bmc_tx_status & BIT_TX_SEND_MESSAGE_DONE)
	{
		sig_send_message_done = 1; 
	}
	if(bmc_tx_status & BIT_TX_SEND_GOODCRC_DONE)
	{
		sig_send_goodcrc_done = 1;
	}
	if(bmc_tx_status & BIT_TX_SEND_HARDRST_DONE)
	{
		sig_send_hardrst_done = 1;
	}
	if(bmc_tx_status & BIT_TX_MESSAGE_DISCARDED)
	{
		sig_message_discarded = 1;
	}
	if(bmc_tx_status & BIT_TX_GOODCRC_DISCARDED)
	{
		sig_goodcrc_discarded = 1;
	}

	/////////////////////////// cc voltage change //////////////////////////////
	if(cc_det_status & (BIT_CC1_VOL_CHG_CCDB | BIT_CC2_VOL_CHG_CCDB))
	{
		// cc voltage change:
		_check_cc_src_connection(1); 
	}

	/////////////////////////// pe timer time out //////////////////////////////
	// pre-process:	just set flag and not register event...
	// post-process:
	if(timer0_status || timer1_status || timer2_status)
	{
		usbpd_timer_bits[0] = timer0_status;
		usbpd_timer_bits[1] = timer1_status;
		usbpd_timer_bits[2] = timer2_status;
		// register event:
		event_add_tail(1, EVT_USB_PD, PBIT_USBPD_PETMR_PENDING);
	}
	
	// clear interupt:
	regmap0B(0x01) = bmc_rx_status;
	regmap0B(0x02) = bmc_tx_status;	
	regmap0B(0x03) = timer0_status; 
	regmap0B(0x04) = timer1_status;
	regmap0B(0x05) = timer2_status; 
	regmap0B(0x0C) = cc_det_status;		
}

////////////////////////////////////// init routine /////////////////////////////////////////

static void _hardware_init(void)
{
	// add hardware init here...
}

void usb_pd_init(void)
{
	// for debug trace:
	memset(g_usbpd_trace, 0, 16); 

	// clock gate: enable at first, then hardware start work...
	regmap0D(0x01) = 0x00;
	// detect clock gate:
	regmap0D(0x60) = 0x00; 
	regmap0D(0x61) |= (1 << 0); // CC_DET_EN = 1

	// clear previous interrupt status:
	regmap0B(0x01) = 0xFF;
	regmap0B(0x02) = 0xFF;
	regmap0B(0x03) = 0xFF; 
	regmap0B(0x04) = 0xFF;
	regmap0B(0x05) = 0xFF; 
	regmap0B(0x06) = 0xFF;
	regmap0B(0x0D) = 0xFF;

	// interrupt initialize..
	// bmc_rx:
	regmap0D(0x3F) = (BIT_RX_RCV_MESSAGE			|
					  BIT_RX_RCV_HARDRST			);
	// bmc_tx: 
	regmap0D(0x40) = (BIT_TX_SEND_MESSAGE_DONE 		| 
					  BIT_TX_SEND_GOODCRC_DONE		|
					  BIT_TX_SEND_HARDRST_DONE  	|
					  BIT_TX_MESSAGE_DISCARDED		|
					  BIT_TX_GOODCRC_DISCARDED 		);
	// timers: 
	regmap0D(0x41) = 0xFF;
	regmap0D(0x42) = 0xFF;
	regmap0D(0x43) = 0xFF;
	regmap0D(0x44) = 0x00; 
	// vbus detect: 
	regmap0D(0x6D) = 0x00;
	// cc detect: 
	regmap0D(0x6E) = (BIT_CC1_VOL_CHG_CCDB |
					  BIT_CC2_VOL_CHG_CCDB );

	// hardware init...
	_hardware_init(); 

	// software init: 
	usbpd_timer_bits[0] = 0;
	usbpd_timer_bits[1] = 0;
	usbpd_timer_bits[2] = 0;
	usbpd_timer_bits[3] = 0;
	usbpd_event_bits = 0; 

	// protocol init:
	usbpd_protocol_init();

	// policy init routine:	 // when we're source, we will send source capalibity message at start up...
	usbpd_policy_init();		 

	// for fpga test only: we should wait really attached...
	/*
	while((regmap11(0x03) & (1 << 2)) == 0)
	{
	}
	*/

	// check if attached at start up:
	_check_cc_src_connection(0); 

	// start check connect, for test... 
	//timer_polling_enable(1, TP_USBPD_CONNECTION); 
}

//process routine:
void usb_pd_process(uint8 param)
{
	// receive protocol message:
	if(param & PBIT_USBPD_PRLMSG_RECEIVED)
	{
		// do message reception (except goodcrc):
		#if (REPLY_GOODCRC_IN_ISR == 0)
		if(usbpd_reply_goodcrc())
		#endif 
		{
			usbpd_message_reception(); 
		}  
	}
	// message penging: need policy engine to process...
	if(param & PBIT_USBPD_PEMSG_PENDING)
	{
		// process general message in pending...	
		usbpd_message_process();
	}
	// timer timeout pending...
	if(param & PBIT_USBPD_PETMR_PENDING)
	{
		uint8 data i;
		for(i=0; i<3; ++i)
		{
			if(usbpd_timer_bits[i])
			{
				usbpd_timer_process(i, usbpd_timer_bits[i]);
				// clear previous status:
				usbpd_timer_bits[i] = 0; 
			}
		}
	}
	// event pending: need policy to process... 
	if(param & PBIT_USBPD_PEEVT_PENDING)
	{
		// event process:
		usbpd_event_process(usbpd_event_bits); 
		// don't forget clear with interrupt disabled:
		INT_DISABLE(); 
		usbpd_event_bits = 0; 
		INT_ENABLE(); 
	}
}

// policy event register:
void usb_pd_add_event(uint8 bisr, uint16 evt_type)
{
	usbpd_event_bits |= evt_type;
	event_add_tail(bisr, EVT_USB_PD, PBIT_USBPD_PEEVT_PENDING); 
}

////////////////////////////////////////////////////////////////////////////////////

// check detach by cc:
void _check_cc_src_connection(uint8 bisr)
{
	uint8 v;
  	uint8 cc0_valid;
	uint8 cc1_valid;

	// first check if disconnected...
	/*
	if((regmap11(0x03) & (1 << 2)) == 0)
	{
		// disconnected...
		if(current_connect_status & 0x80)
		{
			// update flag:
			current_connect_status &= 0x7F; 
			// times:
			g_usbpd_trace[3]++;
			// update de-bounce time:
			SET_SRC_DET_DEBOUNCE(DET_PLUG_MS); 
			// notify:
			usb_pd_add_event(bisr, USBPD_EVT_PORT_DISCONNECTED);
		}
		// exit anyway...
		return; 
	}
	*/	

	// now, it's confirmed connedted...
	// start check cc voltage:
	v = regmap0B(0x61);  
	switch((v >> 0) & 0x03)
	{
	case 1:
		cc0_valid = CC_VALID_Rd; 
		break;
	case 2: 
		cc0_valid = CC_VALID_Ra; 
		break; 
	default:
		cc0_valid = 0;
		break;
	}
	// 2. check cc2:
	switch((v >> 2) & 0x03)
	{
	case 1:
		cc1_valid = CC_VALID_Rd;
		break;
	case 2: 
		cc1_valid = CC_VALID_Ra; 
		break; 
	default:
		cc1_valid = 0;
		break;
	}

	// either connected:
	if((cc0_valid || cc1_valid) && (cc0_valid != cc1_valid))
	{
		// attached:
		if((current_connect_status & 0x80) == 0)
		{
			// flag:
			current_connect_status |= 0x80;
			// update system status...
			//regmap10(0x0D) |= (1 << 0); // PD_WORK = 1
			// times:
			g_usbpd_trace[2]++;
			// update de-bounce time:
			SET_SRC_DET_DEBOUNCE(DET_UNPLUG_MS); 
			// notify: 
			usb_pd_add_event(bisr, USBPD_EVT_PORT_CONNECTED); 
			// configure which is cc:
			current_connect_status &= 0xF0;
			if(cc0_valid)
			{
				// set correct cc: 
				regmap0D(0x65) &= ~(1 << 0);
				// flag: important!!!
				current_connect_status |= 0x01;
			}
			else
			{
				// set correct cc:
				regmap0D(0x65) |=  (1 << 0);
				// flag: importart!!!
				current_connect_status |= 0x02; 
			}
		}
	}
	else
	{
		///* we don't need process detach - 2018.11.27
		// detached:
		if(current_connect_status & 0x80)
		{
			// update flag:
			current_connect_status &= 0x7F; 
			// update system status...
			regmap10(0x0D) &= ~(1 << 0); // PD_WORK = 0
			// times:
			g_usbpd_trace[3]++;
			// update de-bounce time:
			SET_SRC_DET_DEBOUNCE(DET_PLUG_MS); 
			// notify:
			usb_pd_add_event(bisr, USBPD_EVT_PORT_DISCONNECTED);
		}
		//*/
	}
}

// disconnect process, which called from isr...just for test...
void usb_pd_check_connect(uint8 bisr)
{
	// wrap function: 
	_check_cc_src_connection(bisr); 
}

////////////////////////////////////////////////////////////////////////////////////

#if (USB_PD_TEST_EN == YES)
// test software algorithm...
/* 
static uint32 xdata test_value 		_at_		(GENMEM_REG_BASE + 0x001C);
static _test_algorithm(void)
{
	test_value = 0;
	test_value = 1UL << 16;
	test_value = 1UL << 24;
	SET_V32_BITS(test_value, 0, 0, 1);
	SET_V32_BITS(test_value, 7, 7, 1);
	SET_V32_BITS(test_value, 31, 24, 0x55);
	SET_V32_BITS(test_value, 23, 16, 0xFF);
	SET_V32_BITS(test_value, 15, 8, 0x55);
	SET_V32_BITS(test_value, 30, 0, 0);	
	test_value = 0x55555555;
}
*/
// test register read/write...												 
///*
static uint8 xdata test_buf[8]		_at_		(GENMEM_REG_BASE + 0x0010); 
static _test_register(void)
{
	// test Page0B0C: read only...
	test_buf[0] = regmap0B(0x01); 
	test_buf[1] = regmap0C(0x01); 
	// test Page0D0E0F: write/read...
	regmap0D(0x01) = 0x11;
	test_buf[2] = regmap0D(0x01);
	regmap0E(0x01) = 0x22; 
	test_buf[3] = regmap0E(0x01);
	regmap0F(0x01) = 0x33;
	test_buf[4] = regmap0F(0x01);		
}
//*/
void usb_pd_test(void)
{
 	// _test_algorithm(); 
	_test_register(); 	
}
#endif 



#endif

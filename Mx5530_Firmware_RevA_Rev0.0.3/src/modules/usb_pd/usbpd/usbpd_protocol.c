
#include "usbpd_protocol.h"
#include "usbpd_policy.h"
#include "timer/timer.h"
#include "sw_delay/sw_delay.h"
#include "event.h"

#if (FMOD_USB_PD_OPEN == YES)

//pointers for mbcrx:
#define  BMCRX_MSG_DATA_BASE     (USBPD_REG_BASE + 0x0101)	// PAGE-0C
#define  PRX_MSGHEAD             ((volatile union  pd_msg_head 	xdata *)(BMCRX_MSG_DATA_BASE+ 0))
#define  PRX_MSG                 ((volatile struct pd_message 	xdata *)(BMCRX_MSG_DATA_BASE+ 0))
#define  PRX_VDMHEAD             ((volatile union  pd_vdm_head	xdata *)(BMCRX_MSG_DATA_BASE+ 2))
//pointers for mbctx:
#define  BMCTX_MSG_DATA_BASE     (USBPD_REG_BASE + 0x0301)	// PAGE-0E
#define  PTX_MSGHEAD             ((volatile union  pd_msg_head 	xdata *)(BMCTX_MSG_DATA_BASE+ 0))
#define  PTX_MSG                 ((volatile struct pd_message 	xdata *)(BMCTX_MSG_DATA_BASE+ 0))
#define  PTX_VDMHEAD             ((volatile union  pd_vdm_head	xdata *)(BMCTX_MSG_DATA_BASE+ 2))

// ronly one message buffer... it's easy to extended to message queue in fact...
static volatile struct pd_message xdata pd_current_rxmsg; 
struct pd_message xdata * pd_rxmsg_ptr; 

// only one message buffer... it's easy to extended to message queue in fact...
static volatile struct pd_message xdata pd_current_txmsg;
struct pd_message xdata * pd_txmsg_ptr;

// PD3.0 extend message buffer for receive: 
static volatile struct pd_ext_message xdata pd_current_ext_rxmsg; 
struct pd_ext_message xdata * pd_ext_rxmsg_ptr; 

// PD3.0 extend message buffer for tranmit: 
static volatile struct pd_ext_message xdata pd_current_ext_txmsg;
struct pd_ext_message xdata * pd_ext_txmsg_ptr;

// message id management:
static uint8 xdata message_id_counter;
// message id stored:
static uint8 xdata message_id_stored;
// messageID in received goodcrc:
static uint8 xdata goodcrc_message_id; 

// bmc rx interrupt signals:
bit sig_goodcrc_received; 
							
// bmc tx interrupt signals:
bit sig_send_message_done;   
bit sig_send_goodcrc_done;
bit sig_send_hardrst_done;
bit sig_message_discarded;
bit sig_goodcrc_discarded;

// macro define:
// trigger PHY to send message...
#define TRIGGER_SEND_MESSAGE()		do{regmap0D(0x09) |= 0x01; regmap0D(0x09) &= 0xFE;}while(0)	
// trigger PHY to send hardreset...
#define TRIGGER_SEND_HARDRST()		do{regmap0D(0x08) |= 0x01; regmap0D(0x08) &= 0xFE;}while(0)
// inform policy engine that hardreset received or has been sent...
#define HARDRST_DONE_PE()		
// check if bus idle...we have to check this before send out message...
#define CCBUS_IS_IDLE()				(((regmap0B(0x27) & 0x05) == 0x05) ? 1 : 0)
// set message type to default...
#define RELEASE_MSG_TYPE()			do{usbpd_set_msg_type(MSG_TYPE_SOP);}while(0)

// trace hardreset...
static uint8 xdata hardreset_rcv_times			_at_	(GENMEM_REG_BASE + 0x0C);  
static uint8 xdata hardreset_snd_times			_at_	(GENMEM_REG_BASE + 0x0D);

// trace if message discarded... 
static uint8 xdata trace_txretry_counter		_at_	(GENMEM_REG_BASE + 0x0E);
static uint8 xdata trace_discard_counter		_at_	(GENMEM_REG_BASE + 0x0F); 

/////////////////////////////////////// general external routine ////////////////////////////////////////

//init routines:
void usbpd_protocol_init(void)
{
	// rx message init:
	memset((void*)&pd_current_rxmsg, 0, sizeof(struct pd_message)); 
	pd_rxmsg_ptr = &pd_current_rxmsg; 
						   
	// tx message init:
	memset((void*)&pd_current_txmsg, 0, sizeof(struct pd_message)); 
	pd_txmsg_ptr = &pd_current_txmsg; 

	// PD3.0 extern rx message init: 
	memset((void*)&pd_current_ext_rxmsg, 0, sizeof(struct pd_ext_message));
	pd_ext_rxmsg_ptr = &pd_current_ext_rxmsg; 
	
	// PD3.0 extern tx message init:
	memset((void*)&pd_current_ext_txmsg, 0, sizeof(struct pd_ext_message));
	pd_ext_txmsg_ptr = &pd_current_ext_txmsg; 

	// hw registers:
	regmap0D(0x09) &= 0xFE; // message trigger register init...
	regmap0D(0x08) &= 0xFE;	// hard reset trigger register init...
	//regmap2D(0x3F) &= 0xFE; // hardreset done pe...
	usbpd_set_msg_type(MSG_TYPE_SOP); // set sop as default message type...

	// sw variables:
	message_id_counter = 0;
	message_id_stored = 0xFF;
	goodcrc_message_id = 0xFF; 
	// trace:
	trace_discard_counter = 0;
	trace_txretry_counter = 0; 
	hardreset_rcv_times = 0;
	hardreset_snd_times = 0;

	// bmc rx interrupt signals:
	sig_goodcrc_received = 0; 
							
	// bmc tx interrupt signals:
	sig_send_message_done = 0;   
	sig_send_goodcrc_done = 0;
	sig_send_hardrst_done = 0;
	sig_message_discarded = 0;
	sig_goodcrc_discarded = 0;
}

void usbpd_protocol_reset(void)
{
	// reset MessageIDCounter
	message_id_counter = 0; 
	// clear MessageID value:
	message_id_stored = 0xFF;	
}


//////////////////////////////// process received message ///////////////////////////////////

bool usbpd_reply_goodcrc(void)
{
	// 1. construct and send GoodCRC (PRL_Rx_Send_GoodCRC):
	PTX_MSGHEAD->bits._extended = 0; // don't forget this!!!
	PTX_MSGHEAD->bits._num_data_objects = 0;
	PTX_MSGHEAD->bits._message_id = PRX_MSGHEAD->bits._message_id; // same with recieved message... 
	PTX_MSGHEAD->bits._port_power_role = current_power_role; // defined in usbpd_policy.c...
	PTX_MSGHEAD->bits._spec_revision = SPEC_REV20; // we always set to 2.0 in goodcrc...
	PTX_MSGHEAD->bits._port_data_role = current_data_role;
	PTX_MSGHEAD->bits._message_type = CTRL_MSG_GOODCRC; 

	// 2. we should send goodcrc within 195us, or do nothing...
	/*
	{
	uint8 i;
	#define TIME_N	  19 	
	for(i=0; i<TIME_N; ++i)
	{
		// check if channel idle:
		if((regmap0B(0x27) & 0x05) == 0x05)
			break;
		// 19 * 10 = 190us...
		sw_delay_10us(); 
	} 
	if(i >= TIME_N) 
	{
		// time out! nothing happend...
		return;
	} 
	}
	*/
	///*
	// using hdmitx timer to calculate 195us...
	while(1)
	{
		// check if timeout:
		/*
		if(usbpd_us_timer_is_timeout(0))
		{
			//timeout! which mean channel not idle within 195us...	
			return false;
		} 
		*/

		// check if channel idle:
		if(CCBUS_IS_IDLE())
		{
			// channel become idle...
			break;
		}	
	}
	//*/
	// trigger:
	TRIGGER_SEND_MESSAGE();

	return true; 
}

void usbpd_message_reception(void)
{
	//uint8 data i; 

	// 1. check if it's soft reset message (PRL_RX_Layer_Reset_for_Receive):
	if(PRX_MSGHEAD->bits._num_data_objects == 0)
	{
		if(PRX_MSGHEAD->bits._message_type == CTRL_MSG_SOFT_RESET)
		{
			// reset messageID counter and stored messageID:
			usbpd_protocol_reset(); 
			// tell transmission to PRL_Tx_PHY_Layer_Reset state:
			// do nothing???		
		}
	}

	/*
	// 1.5 when BIST message arrived, we can not send GoodCRC:
	if(PRX_MSGHEAD->bits._num_data_objects && PRX_MSGHEAD->bits._message_type == DATA_MSG_BIST)
	{
		goto RX_SUCCESS;
	}
	*/

	// 2. construct and send GoodCRC (PRL_Rx_Send_GoodCRC):
	//usbpd_reply_goodcrc(); 
	
	// 3. wait for goodcrc send complete:
	for(;;)
	{
		// if completed:
		if(sig_send_goodcrc_done)
		{
			// reset signal:
			sig_send_goodcrc_done = 0;
			break;
		}
		// if discarded:
		if(sig_goodcrc_discarded)
		{
			// trace if discarded...
			if((trace_discard_counter & 0x0F) == 0x0F)
				trace_discard_counter &= 0xF0;
			else
				trace_discard_counter += 0x01; 
			break;
		}
		// add some delay:
		sw_delay_10us(); 	
	}
	// if goodcrc discarded, nothing happened...
	if(sig_goodcrc_discarded)
	{
		// reset signal:
		sig_goodcrc_discarded = 0;
		return; 
	}

	// 4. check if messageID repeated:
	if(PRX_MSGHEAD->bits._message_id == message_id_stored)
	{
		// it's a repeat message, do nothing...
		return; 
	}

	// 5. it's a valid message:
	message_id_stored = PRX_MSGHEAD->bits._message_id;
	goto RX_SUCCESS; // disable warning...

RX_SUCCESS:

	// 6. prepare message for policy engine...
	memcpy((void*)pd_rxmsg_ptr, (void*)PRX_MSG, sizeof(struct pd_message));
	// notify policy engine: 
	event_add_tail(0, EVT_USB_PD, PBIT_USBPD_PEMSG_PENDING); 
}

// check if receive goodcrc message:
bool usbpd_check_goodcrc(void)
{
	if((PRX_MSGHEAD->bits._num_data_objects == 0) && (PRX_MSGHEAD->bits._message_type == CTRL_MSG_GOODCRC))
	{
		// record message id:
		goodcrc_message_id = PRX_MSGHEAD->bits._message_id;
		// update signal:
		sig_goodcrc_received = 1;
		return true;
	}
	return false;
}

/////////////////////////////////////// process sending message ///////////////////////////////////////////

// protocol layer message transmission:
bool usbpd_message_transmission(void)
{
	uint8 retry_counter = 0;  // retry counter reset...
	uint8 retry_gate = (current_pd_version < SPEC_REV30) ? 4 : 3; 

	// 1. check if it's soft reset message (PRL_Tx_Layer_Reset_for_Transmit):
	if(pd_txmsg_ptr->_msg_head.bits._num_data_objects == 0)
	{
		if(pd_txmsg_ptr->_msg_head.bits._message_type == CTRL_MSG_SOFT_RESET)
		{
			// reset MessageIDCounter
			usbpd_protocol_reset();
			// tell reception to PRL_Rx_Wait_for_PHY_Message:
			// do nothing???		
		}
	}

	// 2. should not be GoodCRC Message, insert our MessageIDCounter (PRL_Tx_Construct_Message):
	pd_txmsg_ptr->_msg_head.bits._message_id = message_id_counter; 
	// copy to register map: 
	memcpy((void*)PTX_MSG, (void*)pd_txmsg_ptr, sizeof(struct pd_message)); 

TX_RETRY: 	
	// 3. check retry times whatever channel idle or not...
	retry_counter += 1;
	if(retry_counter <= retry_gate) // nRetryCounter = 3 (in PD3.0, it's 2) - so 4 times...
	{
		// trace retry:
		if(retry_counter > 1)
			trace_txretry_counter += 1; 
		// trigger PHY to send message:
		TRIGGER_SEND_MESSAGE(); 
	}
	else
	{
		// Failed! (PRL_Tx_Transmission_Error):
		message_id_counter += 1;
		message_id_counter &= 7;
		// recover message type:
		RELEASE_MSG_TYPE(); 		 
		return false;  
	}
	
	// 4. wait for message send completed:
	for(;;)
	{
		// if message send over...
		if(sig_send_message_done)
		{
			sig_send_message_done = 0;
			break;
		}
		// if message discarded...
		if(sig_message_discarded)
		{
			// trace discarded...
			if((trace_discard_counter & 0xF0) == 0xF0)
				trace_discard_counter &= 0x0F; 
			else
				trace_discard_counter += 0x10; 
			// if message discarded, retry...
			break; 
		}	
	}
	// if discarded due to channel busy, try again...
	if(sig_message_discarded)
	{
		// reset flag:
		sig_message_discarded = 0;
		goto TX_RETRY; 
	}	 

	// 5. wait for GoodCRC response: tReceive = 0.9~1.1ms
	///*
	{
		uint8 data i;
		for(i=0; i<11; ++i)
		{
			// if goodcrc received:
			if(sig_goodcrc_received)
			{
				// reset signal:
				sig_goodcrc_received = 0;
				break;
			}
			// delay 100us:
			sw_delay_100us(); 
		}
		// timeout (PRL_Tx_Check_RetryCounter):
		if(i >= 11)
		{		 
			goto TX_RETRY; 
		}	
	}
	//*/
	/*
	usbpd_us_timer_start(0, 1000); // tReceive = 0.9~1.1ms
	for(;;)
	{
		// if goodcrc received:
		if(sig_goodcrc_received)
		{
			// reset signal:
			sig_goodcrc_received = 0;
			break;
		}
		// if time out, try again:
		if(usbpd_us_timer_is_timeout(0))
		{
			goto TX_RETRY; 
		}
	}
   	*/

	// messageID mismatch:
	if(message_id_counter != goodcrc_message_id) 
	{
	   	goto TX_RETRY; 
	}

	// here, mean messageID match!
	message_id_counter += 1; 
	message_id_counter &= 7;  
	
	// not fix message type: 
	RELEASE_MSG_TYPE(); 

	return true; 
}

///////////////////////////////////////// hard/cable reset process //////////////////////////////////////

// send hard(cable) reset...
void usbpd_send_hardreset(uint8 is_cable)
{
	// disable warning:
	is_cable = is_cable; 

	// tracce:
	hardreset_snd_times += 1; 
													
	// 1. PRL_HR_Reset_Layer:
	// reset MessageID Counter: 
	message_id_counter = 0; 
	// PRL_Tx_Wait_For_Message_Request & PRL_Rx_Wait_for_PHY_Message:
	// do nothing...

	// Request from policy layer...
	TRIGGER_SEND_HARDRST();
	// wait for complete: 4-5ms...
	{
		uint8 data i;
		for(i=0; i<5; ++i)
		{				
			if(sig_send_hardrst_done)
			{
				// reset flag:
				sig_send_hardrst_done = 0;
				break;  	
			}
			// wait for 1ms...
			sw_delay_1ms(); 
		}
	}
  	// whatever timeout or not, inform PE...
	HARDRST_DONE_PE(); 	
}

// hard(cable) reset received...will ba called from isr...
void usbpd_hardreset_received(void)
{
	// tracce:
	hardreset_rcv_times += 1; 
	// inform PE hardreset received...
	HARDRST_DONE_PE(); 
}






#endif





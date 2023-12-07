
#include "usbpd_policy.h"
#include "usbpd_protocol.h"
#include "usb_pd.h"
#include "sw_delay/sw_delay.h"
#include "power/power.h"

#if (FMOD_USB_PD_OPEN == YES)

// Qualcomm VID:
#define QUALCOMM_VID					0x05C6

// PDP Rating Predefinitions: Index = 0 ~ 8 to select target PDP RATING...
#define PDP_RATING_SEL			0
// when max voltage = 12V:
#define PDO_12V_SEL				3
#define PDO_12V_NUM				4
static uint32 code PDO_12V_VAL[PDO_12V_SEL][PDO_12V_NUM] = 
{
	// 1. 18W: 5V@3A, 9V@2A, 12V@1.5A, 9V_Prog@2A
	{0x0801912C, 0x0802D0C8, 0x0803C096, 0xC8DC2128}, 
	// 2. 24W: 5V@3A, 9V@2.5A, 12V@2A, 9V_Prog@2.5A
	{0x0801912C, 0x0802D0FA, 0x0803C0C8, 0xC8DC2132},
	// 3. 30W: 5V@3A, 9V@3A, 12V@2A, 9V_Prog@3A
	{0x0801912C, 0x0802D12C, 0x0803C0C8, 0xC8DC213C}	
};
// when max voltage = 20V:
#define PDO_20V_SEL				6
#define PDO_20V_NUM				6
static uint32 code PDO_20V_VAL[PDO_20V_SEL][PDO_20V_NUM] =  
{
	// 1. 27W: 5V@3A, 9V@3A, 12V@2.5A, 15V@1.8A, 20V@1.35A, 9V_Prog@3A
	{0x0801912C, 0x0802D12C, 0x0803C0FA, 0x0804B0B4, 0x08064087, 0xC8DC213C},
	// 2. 30W: 5V@3A, 9V@2.5A, 12V@2A, 15V@1.5A, 20V@1.2A, 9V_Prog@2.5A
	{0x0801912C, 0x0802D0FA, 0x0803C0C8, 0x0804B096, 0x08064078, 0xC8DC2132},
	// 3. 36W: 5V@3A, 9V@3A, 12V@2.5A, 15V@2A, 20V@1.5A, 15V_Prog@2A 
	{0x0801912C, 0x0802D12C, 0x0803C0FA, 0x0804B0C8, 0x08064096, 0xC9402128},
	// 4. 40W: 5V@3A, 9V@3A, 12V@3A, 15V@2.4A, 20V@1.8A, 15V_Prog@2.4A 
	{0x0801912C, 0x0802D12C, 0x0803C12C, 0x0804B0F0, 0x080640B4, 0xC9402130},
	// 5. 45W: 5V@3A, 9V@3A, 12V@3A, 15V@3A, 20V@2.25A, 20V_Prog@2.25A
	{0x0801912C, 0x0802D12C, 0x0803C12C, 0x0804B12C, 0x080640E1, 0xC990212D},
	// 6. 60W: 5V@3A, 9V@3A, 12V@3A, 15V@3A, 20V@3A, 20V_Prog@3A
	{0x0801912C, 0x0802D12C, 0x0803C12C, 0x0804B12C, 0x0806412C, 0xC990213C}
};
// flag that if support 5A power capability...
static uint8 xdata need_support_5A;  
// pdo number:
static uint8 xdata cur_pdo_num; 

// PE timers...
// timer index-0:
#define TIMER_NO_RESPONSE				(1 << 0)
#define TIMER_HARDRST_COMPLETE			(1 << 1)
#define TIMER_CRC_RECEIVE				(1 << 2)
#define TIMER_PS_TRANSITION				(1 << 3)
#define TIMER_SENDER_RESPONSE			(1 << 4)
#define TIMER_SINK_REQUEST				(1 << 5)
#define TIMER_SINK_WAIT_CAP				(1 << 6)
#define TIMER_SOURCE_CAPABILITY			(1 << 7)
// timer index-1:
#define TIMER_PS_HARD_RESET				(1 << 0)
#define TIMER_VCONN_ON					(1 << 1)
#define TIMER_VCONN_OFF					(1 << 2)
#define TIMER_BIST_CONT_MODE			(1 << 3)
#define TIMER_VDM_RESPONSE				(1 << 4)
#define TIMER_DISCOVER_IDENTITY			(1 << 5)
#define TIMER_SINK_TX					(1 << 6)
#define TIMER_CHUNK_NOT_SUPPORT			(1 << 7)
// timer index-2: 
#define TIMER_CHUNK_SENDER_REQ			(1 << 0)
#define TIMER_CHUNK_SENDER_RESP			(1 << 1)
#define TIMER_SOURCE_PPS_COMM			(1 << 2)
// timer index-3:							
#define TIMER_MS_GENERAL0				(1 << 0)
#define TIMER_MS_GENERAL1				(1 << 1)
#define TIMER_MS_GENERAL2				(1 << 2)
#define TIMER_US_GENERAL0				(1 << 3)
#define TIMER_US_GENERAL1				(1 << 4)
#define TIMER_US_GENERAL2				(1 << 5)

// timer control:
#define PE_TIMER_ENABLE(idx, bits)		do{regmap0D(0x10 + (idx)) =  (bits);}while(0)
#define PE_TIMER_DISABLE(idx, bits)		do{regmap0D(0x10 + (idx)) = ~(bits);}while(0)
#define PE_TIMER_START(idx, bits)		do{regmap0D(0x14 + (idx)) = (bits); _nop_(); regmap0D(0x14 + (idx)) = 0;}while(0)
#define PE_TIMER_STOP(idx, bits)		do{regmap0D(0x18 + (idx)) = (bits); _nop_(); regmap0D(0x18 + (idx)) = 0;}while(0)

// message process assist routine:
static void _process_ctrl_message(void);
static void _process_data_message(void); 
static void _process_data_vdm_message(void);
static void _process_structured_vdm_message(void);
static void _process_unstructured_vdm_message(void);  
static void _do_physical_bist(void); 
static bool _send_ctrl_message(uint8 msg_type);
static void _send_hardreset_message(void);  
static bool _send_softreset_message(void);
static bool _send_general_message(void); 
static void _process_extd_message(void); // PD3.0
static bool _parse_extd_message(void);
static bool _send_chunk_req_message(void); 
static bool _send_chunk_resp_message(void);
static bool _send_extend_message(void);

// state transition...
#define PE_STATE_INVALID					0x60
static uint8 new_port_state; 
// source port state machine: 
#define PE_SRC_HARD_RESET_RECEIVED			200
#define PE_SRC_TRANSITION_TO_DEFAULT		201 
#define PE_SRC_HARD_RESET					202
#define PE_SRC_STARTUP						203
#define PE_SRC_DISCOVERY					204
#define PE_SRC_SEND_CAPABILITIES			205
#define PE_SRC_NEGOTIATE_CAPABILITY			206
#define PE_SRC_TRANSITION_SUPPLY			207
#define PE_SRC_READY						208
#define PE_SRC_CAPABILITY_RESPONSE			209
#define PE_SRC_WAIT_NEW_CAPABILITIES		210
#define PE_SRC_GIVE_SOURCE_CAP				211
#define PE_SRC_GET_SINK_CAP					212
static void _pe_src_port_states(uint8 state); 

// source port routines:
static uint8 xdata snk_req_evaluate_result; // 0: cannot met - 1: can met - other: can met later
static void _src_build_source_capabilities(void); 
static void _src_evaluate_sink_request(void);   
static uint8 xdata src_caps_counter;
// we response data swap...
static bool _dfp2ufp_response_data_swap(uint8 step);
// discover cable id:
static bool _do_cable_discover_identify(void);

// PD3.0 new messages:
// prepare message routines...
static void _build_source_caps_extend_message(void); 
static void _build_status_message(void); 
static void _build_pps_status_message(void);
// evaluate message routines...
static bool _src_evaluate_sink_alert(void); 
static void _src_evaluate_sink_status(void); 
static void _src_evaluate_sink_caps_extend(void); 

// extend message index: 
static uint16 xdata cur_ext_rxmsg_index; 
static uint16 xdata cur_ext_txmsg_index; 

// varialbles:
static uint32 data data_object_t; 
// bist counter...
static uint8 xdata physical_bist_counter;  
// power role... 
bit current_power_role; 
// data role... 
bit current_data_role; 
// vconn role... 
bit current_vconn_role; 
// current version...
uint8 xdata current_pd_version;
// contract status... 
static bit current_contract_status;	// 0: not constructed - 1: constructed 
// port state: 
#define PS_STATUS_NONE			10
static uint8 xdata current_port_state		_at_	(GENMEM_REG_BASE + 0x1C);	
//[7]: connect status-> 1=connected, 0=disconnected
//[6:4]: Reserved
//[3:0]: cc detect status -> 1: cc0=cc, 2:cc1=cc, other=no detected
uint8 xdata current_connect_status			_at_	(GENMEM_REG_BASE + 0x1D); 
//_at_	(GENMEM_REG_BASE + 0x001E);	// used in usbpd_protocol.c...
//_at_	(GENMEM_REG_BASE + 0x001F);	// used in usbpd_protocol.c...

// trace:
static uint8 xdata snk_timeout_bits			_at_	(GENMEM_REG_BASE + 0x0A);
static uint8 xdata src_timeout_bits 		_at_	(GENMEM_REG_BASE + 0x0B);

// power information:
static struct pd_message xdata g_our_capabilities; // our capability...
static uint16 xdata src_negotiated_voltage; // in mV unit...
static uint16 xdata src_negotiated_current; // in mA unit...
static uint16 xdata current_real_voltage; // in mV...
static uint16 xdata current_real_current; // in mA...
static bit current_is_pps_request; 
// power operation: 
static void _build_our_capabilities(uint8 pdp_sel);
static void _switch_out_voltage(uint16 target_voltage); // called after  _switch_out_current()!!!
static void _switch_out_current(uint16 target_current);	// called before _switch_out_voltage()!!!
static uint8 extract_apdo_voltage(uint8 max_100mv); 

// control vbus: 
#define VBUS_CTRL_ENABLE(n)		do{if(n)regmap10(0x0C)|=(1<<1);else regmap10(0x0C)&=~(1<<1);}while(0)
// charge control: 
#define CHARGE_ENABLE(n)		do{if(n)regmap10(0x0C)|=(1<<0);else regmap10(0x0C)&=~(1<<0);}while(0)

//////////////////////////////////////////// external routines ////////////////////////////////////////////////

// init routine:
void usbpd_policy_init(void)
{
	// enable all timers:
	PE_TIMER_ENABLE(0, 0xFF); 
	PE_TIMER_ENABLE(1, 0xFF);
	PE_TIMER_ENABLE(2, 0xFF);
	PE_TIMER_ENABLE(3, 0xFF); 

	// hardware init:
	VBUS_CTRL_ENABLE(1); // FW can control VBUS...

	// reset:
	usbpd_policy_reset();
	 
	// variables:
	current_port_state = PE_STATE_INVALID; 
	snk_timeout_bits = 0;
	src_timeout_bits = 0; 

	// important status:
	current_power_role = PORT_POWER_ROLE_SOURCE; 
	current_data_role = PORT_DATA_ROLE_DFP; 
	current_vconn_role = 1; 
	current_pd_version = SPEC_REV30; 
	current_connect_status = 0x00;

	// init our capabilities:
	_build_our_capabilities(PDP_RATING_SEL); 

	// set start up state: (just for debug) 
	//_pe_snk_port_states(PE_SNK_TRANSITION_TO_DEFAULT); // assume we're sink...
	//_pe_src_port_states(PE_SRC_TRANSITION_TO_DEFAULT); // assume we're source...
}

// reset routine:
void usbpd_policy_reset(void)
{
	// stop timer:
	PE_TIMER_STOP(0, 0xFF);
	PE_TIMER_STOP(1, 0xFF);
	PE_TIMER_STOP(2, 0xFF);
	PE_TIMER_STOP(3, 0xFF); 

	// variables: 
	physical_bist_counter = 0; 
	// extend message index:
	cur_ext_rxmsg_index = 0;
	cur_ext_txmsg_index = 0;
	// current version:
	current_pd_version = SPEC_REV30; 
	// new state:
	new_port_state = PE_STATE_INVALID; 
	// support 5A:
	need_support_5A = 0; 
	
	// power information: 
	src_negotiated_voltage = 0;
	src_negotiated_current = 0;
	current_is_pps_request = 0; 
}

// event process:
void usbpd_event_process(uint16 event_bits)
{
	// hardreset received:
	if(event_bits & USBPD_EVT_HARDRST_RECEIVED)
	{
		// move to PE_SRC_Hard_Reset_Received state:
		_pe_src_port_states(PE_SRC_HARD_RESET_RECEIVED); 
	}
	// source capabilities changed (when in source role)...
	if(event_bits & USBPD_EVT_SRC_CAPS_CHANGED)
	{
		// move to PE_SRC_Send_Capabilities state:
		_pe_src_port_states(PE_SRC_SEND_CAPABILITIES); 
	}
	// sink request changed (when in sink role)...
	if(event_bits & USBPD_EVT_SNK_REQ_CHANGED)
	{

	}
	// we're source, sink attached:
	if(event_bits & USBPD_EVT_PORT_CONNECTED)
	{
		// in state machine:
		_pe_src_port_states(PE_SRC_TRANSITION_TO_DEFAULT);
	}
	// we're source, sink detached:
	if(event_bits & USBPD_EVT_PORT_DISCONNECTED)
	{
		// reset protocol:
		usbpd_protocol_reset();
		// reset policy: 
		usbpd_policy_reset();
	}
	if(event_bits & USBPD_EVT_STATE_TRANSITION)
	{
		// source state:
		_pe_src_port_states(new_port_state);
		// reset:
		new_port_state = PE_STATE_INVALID;
	}
}

// entry for process message: which pointed by pd_rxmsg_ptr... 
void usbpd_message_process(void)
{
	// some misc process:
	PE_TIMER_STOP(0, TIMER_SINK_REQUEST); 	// when we're source...

	// we should be in connect status:
	if((current_connect_status & 0x80) == 0)
	{
		// if not connected, do nothing...
		return; 
	}

	// check if in physical bist:
	if(physical_bist_counter)
	{
		// we're in bist mode, need hard reset:
		return; 
	}
	
	// 2. if general message, first check message type:  
	if(pd_rxmsg_ptr->_msg_head.bits._num_data_objects == 0) 
	{
		// process control message:
		_process_ctrl_message(); 
	}
	else    
	{
		// here we should check if it's extended message...
		if(pd_rxmsg_ptr->_msg_head.bits._extended)
		{			 
			// if the extend message is completed:
			if(_parse_extd_message())
			{
				// process extended message - PD3.0:
				_process_extd_message(); 
			}	
		}
		else
		{
			// process data message:
			_process_data_message(); 
		} 
	}
}

// process pe timers:
void usbpd_timer_process(uint8 index, uint8 bits)
{
	if(index == 0)
	{
		if(bits & TIMER_SENDER_RESPONSE)
		{				
			// trace:
			src_timeout_bits = bits;
			// enter PE_SRC_HardReset:
			_pe_src_port_states(PE_SRC_HARD_RESET); 
		}	
		if(bits & TIMER_SOURCE_CAPABILITY)
		{
			// goto PE_SRC_Send_Capabilities state:	
			_pe_src_port_states(PE_SRC_SEND_CAPABILITIES); 
		}	
	}
	else if(index == 1)
	{
		if(bits & TIMER_PS_HARD_RESET)
		{
			// reset voltage to vSafe0v:
			CHARGE_ENABLE(0);  
			// tSrcRecover = 0.66 - 1s - 2017.10.26
			{
				uint8 i;
				for(i=0; i<8; ++i) 
				{
					sw_delay_100ms(); 
				}
			}
			// move to default state:
			_pe_src_port_states(PE_SRC_TRANSITION_TO_DEFAULT); 
		}
		if(bits & TIMER_VCONN_ON)
		{
			// send hard reset:
			src_timeout_bits = bits; 
			_pe_src_port_states(PE_SRC_HARD_RESET);
		}
	}
	else if(index == 2)
	{
		if(bits & TIMER_SOURCE_PPS_COMM)
		{
			// send hard reset:
			src_timeout_bits = bits; 
			_pe_src_port_states(PE_SRC_HARD_RESET);	
		}
	}	
}

//////////////////////////// internal message process routines  ///////////////////////////////////

// control mesage parse:
void _process_ctrl_message(void)
{
	switch(pd_rxmsg_ptr->_msg_head.bits._message_type)
	{
		// GoodCRC: will not happened...
		case CTRL_MSG_GOODCRC:	
		break;

		//////////////////////////////////////// GotoMin //////////////////////////////////////////
		case CTRL_MSG_GOTOMIN:	
		break;

	   	//////////////////////////////////////// Accept /////////////////////////////////////////// 
		case CTRL_MSG_ACCEPT:
		break;

		//////////////////////////////////////// Reject /////////////////////////////////////////// 
		case CTRL_MSG_REJECT:
		break;

		///////////////////////////////////////// Ping ////////////////////////////////////////////
	    case CTRL_MSG_PING:	
		// nothing to do...	   
		break;
		
		//////////////////////////////////////// PS_RDY ///////////////////////////////////////////
		case CTRL_MSG_PS_RDY:
		break;

		///////////////////////////////////// Get_Source_Cap //////////////////////////////////////
		case CTRL_MSG_GET_SOURCE_CAP:  
		// we're source, send our capabilities...
		_pe_src_port_states(PE_SRC_GIVE_SOURCE_CAP);
		break;		

		////////////////////////////////////// Get_Sink_Cap ///////////////////////////////////////
		case CTRL_MSG_GET_SINK_CAP:
		// we're source, just reject it...
		_send_ctrl_message((current_pd_version >= SPEC_REV30) ? CTRL_MSG_NOT_SUPPORT : CTRL_MSG_REJECT); 
		break;

		//////////////////////////////////////// DR_Swap ////////////////////////////////////////// 
		case CTRL_MSG_DR_SWAP:
		// for support qc4, we have to...
		if(current_data_role == PORT_DATA_ROLE_DFP)
		{
			// we do data role swap only when we're dfp...
			if(_dfp2ufp_response_data_swap(1))
			{
				// move to 2nd step directly...
				_dfp2ufp_response_data_swap(2);  
			}
		}
		else 
		{  
			// we're ufp and didnt' want to become DFP...
			_send_ctrl_message((current_pd_version >= SPEC_REV30) ? CTRL_MSG_NOT_SUPPORT : CTRL_MSG_REJECT);
		}	
		// not support...
		//_send_ctrl_message((current_pd_version >= SPEC_REV30) ? CTRL_MSG_NOT_SUPPORT : CTRL_MSG_REJECT);
		break;	

		//////////////////////////////////////// PR_Swap //////////////////////////////////////////
		case CTRL_MSG_PR_SWAP:
		// not support...	
		_send_ctrl_message((current_pd_version >= SPEC_REV30) ? CTRL_MSG_NOT_SUPPORT : CTRL_MSG_REJECT); 
		break;

		/////////////////////////////////////// VCONN_Swap ////////////////////////////////////////  
		case CTRL_MSG_VCONN_SWAP:
		// we don't support VCONN swap whatever we're sink or source...	  
		_send_ctrl_message((current_pd_version >= SPEC_REV30) ? CTRL_MSG_NOT_SUPPORT : CTRL_MSG_REJECT);	
		break;

		///////////////////////////////////////// Wait ////////////////////////////////////////////  
		case CTRL_MSG_WAIT:			   
		break;

		///////////////////////////////////// Soft_Reset ////////////////////////////////////////// 
		case CTRL_MSG_SOFT_RESET:
		if(!_send_ctrl_message(CTRL_MSG_ACCEPT))
		{
			// goto Hard reset state:
			_pe_src_port_states(PE_SRC_HARD_RESET); 
		}
		else
		{
			// goto PE_SRC_Send_Capabilities state:	
			_pe_src_port_states(PE_SRC_STARTUP); 
		}
		break;	

		//////////////////////////////////// PD3.0 Added Message //////////////////////////////////
		 
		////////////////////////////////////// Not_Supported ////////////////////////////////////// 
		case CTRL_MSG_NOT_SUPPORT:
		// do nothing...
		break; 

		////////////////////////////////// Get_Source_Cap_Extended //////////////////////////////// 
		case CTRL_MSG_GET_SOURCE_CAP_EXT:
		// build our source caps extend and send...
		_build_source_caps_extend_message();
		_send_extend_message(); 
		//_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break; 

		////////////////////////////////////// Get_Status ///////////////////////////////////////// 
		case CTRL_MSG_GET_STATUS:
		// build message and send...
		_build_status_message(); 
		_send_extend_message(); 
		//_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break; 

		/////////////////////////////////////// FR_Swap /////////////////////////////////////////// 
		case CTRL_MSG_FR_SWAP:
		// not support in default..
		_send_ctrl_message((current_pd_version >= SPEC_REV30) ? CTRL_MSG_NOT_SUPPORT : CTRL_MSG_REJECT); 
		break; 

		//////////////////////////////////// Get_PPS_Status /////////////////////////////////////// 
		case CTRL_MSG_GET_PPS_STATUS:
		// build pps status message:
		_build_pps_status_message(); 
		_send_extend_message(); 
		//_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break; 

		/////////////////////////////////// Get_Country_Codes ///////////////////////////////////// 
		case CTRL_MSG_GET_COUNTRY_CODES:
		// not support in default: 
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break; 

		////////////////////////////////// Get_Sink_Cap_Extended ////////////////////////////////// 
		case CTRL_MSG_GET_SINK_CAP_EXT:
		// not support:
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break;

		/////////////////////////////////// Unrecognized Message //////////////////////////////////
		default: 
		// do nothing if we dind't know it...
		if(current_pd_version == SPEC_REV30) 
		{
			// for pd3.0 only...
			_send_ctrl_message(CTRL_MSG_NOT_SUPPORT);
		}
		break; 
	}			
}

// data message process:
void _process_data_message(void)
{
	switch(pd_rxmsg_ptr->_msg_head.bits._message_type)
	{
		////////////////////////////////// Source_Capabilities ////////////////////////////////////
		case DATA_MSG_SOURCE_CAPS: 
		break;

		/////////////////////////////////////// Request /////////////////////////////////////////// 
		case DATA_MSG_REQUEST:
		// negotiate requeset:
		_pe_src_port_states(PE_SRC_NEGOTIATE_CAPABILITY);
		// check engotiate result:
		if(snk_req_evaluate_result)
		{
			// can be met:
			_pe_src_port_states(PE_SRC_TRANSITION_SUPPLY); 
		}  
		else
		{
			// cannot be met or can be met later...
			_pe_src_port_states(PE_SRC_CAPABILITY_RESPONSE); 
			// check if we're in explicit contract already:
			if(!current_contract_status)
			{
				// goto PE_SRC_Wait_New_Capabilities state...
				_pe_src_port_states(PE_SRC_WAIT_NEW_CAPABILITIES);
			}
			else
			{
				// we cann't meet, but current 
				_pe_src_port_states(PE_SRC_READY);  
			}
		}										 
		break;

		///////////////////////////////////////// BIST //////////////////////////////////////////// 
		case DATA_MSG_BIST:	
		{
			// only for cts test...
			_do_physical_bist();
		}			
		break;

		/////////////////////////////////// Sink_Capabilities /////////////////////////////////////
		case DATA_MSG_SINK_CAPS:
		// we didn't send Get_Sink_Capabilities, so we will not receive this message...	 
		break;

		///////////////////////////////////// Battery Status //////////////////////////////////////
		case DATA_MSG_BATTERY_STATUS:
		// do nothing as it won't happen...
		break; 

		///////////////////////////////////////// Alert ///////////////////////////////////////////
		case DATA_MSG_ALERT:
		// when recevie alert, we have to send get status to see what's going on...
		if(!_src_evaluate_sink_alert()); 
		{
			// process non-battery change only...
			_send_ctrl_message(CTRL_MSG_GET_STATUS); 
		}
		break; 

		//////////////////////////////////// Get_Country_Info /////////////////////////////////////
		case DATA_MSG_GET_COUNTRY_INFO:
		// not support in default...
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break; 

		///////////////////////////////////// Vendor_Defined //////////////////////////////////////
		case DATA_MSG_VENDOR_DEFINED:  
	    {
			// vdm message, including alt-mode...
			_process_data_vdm_message();
		}
		break;

		/////////////////////////////////// Unrecognized Message //////////////////////////////////
		default: 
		// we didn't support it:
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break;
	}				
}  

void _process_extd_message(void)
{
	switch(pd_rxmsg_ptr->_msg_head.bits._message_type)
	{
		///////////////////////////// Source_Capabilities_Extended ////////////////////////////////
		case EXTD_MSG_SOURCE_CAPS_EXT: 
		// won't receive this message, haha...
		break;

		/////////////////////////////////////// Status ////////////////////////////////////////////
		case EXTD_MSG_STATUS:
		// check parter's status after sending get_status message...
		break; 

		////////////////////////////////// Get_Battery_Cap ////////////////////////////////////////
		case EXTD_MSG_GET_BATTERY_CAP:	   
		// not support in default...
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break;
		
		///////////////////////////////// Get_Battery_Status //////////////////////////////////////
		case EXTD_MSG_GET_BATTERY_STATUS:
		// not suppport in default...
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break;

		//////////////////////////////// Battery_Capabilities /////////////////////////////////////
		case EXTD_MSG_BATTERY_CAPS:	
		// won't receive...
		break;

		//////////////////////////////// Get_Manufacture_Info /////////////////////////////////////
		case EXTD_MSG_GET_MANU_INFO:
		// not support for a while...
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break;

		////////////////////////////////// Manufacturer_Info //////////////////////////////////////
		case EXTD_MSG_MANU_INFO:
		// won't receive...
		break;

		////////////////////////////////// Security_Request ///////////////////////////////////////
		case EXTD_MSG_SECURITY_REQ:
		// not support it totoally...
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break;

		////////////////////////////////// Security_Response //////////////////////////////////////
		case EXTD_MSG_SECURITY_RESP:
		// won't receive...
		break;

		/////////////////////////////// Firmware_Update_Request ///////////////////////////////////
		case EXTD_MSG_FW_UPDATE_REQ:
		// not support...
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break;

		////////////////////////////// Firmware_Update_Response ///////////////////////////////////
		case EXTD_MSG_FW_UPDATE_RESP:
		// won't receive...
		break;

		///////////////////////////////////// PPS_Status //////////////////////////////////////////
		case EXTD_MSG_PPS_STATUS:
		// won't receive...
		break; 

		//////////////////////////////////// Country_Info /////////////////////////////////////////
		case EXTD_MSG_COUNTRY_INFO:
		// won't receive...
		break;

		//////////////////////////////////// Country_Codes ////////////////////////////////////////
		case EXTD_MSG_COUNTRY_CODES:	 
		// won't receive...
		break; 

		///////////////////////////// Sink_Capabilities_Extended //////////////////////////////////
		case EXTD_MSG_SINK_CAPS_EXT: 
		// won't receive...
		break;

		/////////////////////////////////// Unrecognized Message //////////////////////////////////
		default: 
		// we didn't support it:
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		break;
	}	
}

// structured vendor_defined message process:
void _process_data_vdm_message(void)
{
	uint8 num_data_obj;
	uint8 response_type = TYPE_RESP_ACK;
	struct pd_vdm_message xdata * rx_vdm_msg = (struct pd_vdm_message xdata *)pd_rxmsg_ptr; 
	struct pd_vdm_message xdata * tx_vdm_msg = (struct pd_vdm_message xdata *)pd_txmsg_ptr;

	// pre-constructure message:
	// 1. message header:
	tx_vdm_msg->_msg_head.bits._extended = 0;
	tx_vdm_msg->_msg_head.bits._message_type = DATA_MSG_VENDOR_DEFINED;
	num_data_obj = 1; 
	// 2. vdm header: 
	tx_vdm_msg->_vdm_head._value = rx_vdm_msg->_vdm_head._value;

	// here, we have to process both structured and unstructured(qc4) message!!!
	if(rx_vdm_msg->_vdm_head.bits._vdm_type)
	{
		// structured vmd message:
		#if (SUPPORT_CABLE_DISCOVERY == 1)
		switch(rx_vdm_msg->_vdm_head.bits._command)
		{
		// we only process discover identity... 
		case VDM_CMD_DISCOVER_IDENTITY:
			data_object_t = V32_ENDIAN_SWAP(rx_vdm_msg->_data_objects[3]); // get product types vdo
			// B10-9: voltage: we don't care as voltage selected by sink...
			// B6-5: current:
			if(((data_object_t >> 5) & 3) == 0x02) // 0x02 -> 5A 
			{
				need_support_5A = 1; // update status..
			}
		    break;
		default:
			return;
		}
		// final check response type:
		tx_vdm_msg->_vdm_head.bits._command_type = response_type; 
		#endif 
	}
	/*
	else
	{
		// unstructured vmd message:
		uint8 val; 
		if(rx_vdm_msg->_vdm_head.bits_qc._command0 != 0x03)
			return; 
		// cmd0 must be 0x03...
		switch(rx_vdm_msg->_vdm_head.bits_qc._command1)
		{
		case VDM_CMD_QC_INQUIRE_CHG_TEMP: 
			val = power_read_temperature(0); // internal temperature
			data_object_t = (val > 20) ? (val - 20) : 0; 
			break;
	   	case VDM_CMD_QC_INQUIRE_CON_TEMP:
			val = power_read_temperature(1); // external temperature
			data_object_t = (val > 20) ? (val - 20) : 0;
			break;
		case VDM_CMD_QC_INQUIRE_CON_VOL:
			data_object_t = power_read_voltage(); // voltage
			break; 
		case VDM_CMD_QC_INQUIRE_CHG_TYPE:
			data_object_t = 0x00000004; // 0x02: QC2, 0x03: QC3, 0x04: QC4
			break; 
		case VDM_CMD_QC_INQUIRE_CHG_VER:
			data_object_t = 0x00000030; // Rev3.0
			break; 
		default:
			return; 			
		}
		// vdm header construction...
		tx_vdm_msg->_vdm_head.bits_qc._command0 = VDM_RESP_QC_ACK;
		tx_vdm_msg->_data_objects[0] = V32_ENDIAN_SWAP(data_object_t); 
		num_data_obj += 1; 
	}
	*/

	// construct data object number finally...important!!!
	tx_vdm_msg->_msg_head.bits._num_data_objects = num_data_obj; 

	// 3. vdm data objects...is ready!!!
	if(current_data_role == PORT_DATA_ROLE_UFP)
	{
		// OK! send it out only when we're UFP...
		if(!_send_general_message())
			return;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void _send_hardreset_message(void)
{
	// send hardreset message:
	usbpd_send_hardreset(0); 
	// Start PSHardResetTimer:
	PE_TIMER_START(1, TIMER_PS_HARD_RESET); 
}

// control message...
bool _send_softreset_message(void)
{
	pd_txmsg_ptr->_msg_head.bits._message_type = CTRL_MSG_SOFT_RESET;
	pd_txmsg_ptr->_msg_head.bits._num_data_objects = 0;	
	pd_txmsg_ptr->_msg_head.bits._port_power_role = current_power_role; 
	pd_txmsg_ptr->_msg_head.bits._port_data_role = current_data_role;
	pd_txmsg_ptr->_msg_head.bits._spec_revision = current_pd_version; 
	pd_txmsg_ptr->_msg_head.bits._extended = 0; 
	if(!usbpd_message_transmission())
	{
		// if softreset failed, send hardreset:
		_send_hardreset_message(); 
		// we should reset something...alternate mode at least...
		usbpd_policy_reset(); 
		// failed...
		return false; 
	}
	else
	{
		// stop all timers:
		PE_TIMER_STOP(0, 0xFF);
		PE_TIMER_STOP(1, 0xFF);
		PE_TIMER_STOP(2, 0xFF); 
		// if sucess, we need wait accept in tRecevierResponse...
		PE_TIMER_START(0, TIMER_SENDER_RESPONSE);
		// success???
		return true; 
	}		
}

bool _send_general_message(void) 
{
	// we should update power role and data role here...double confirm!!! - 2017.10.18
	pd_txmsg_ptr->_msg_head.bits._port_power_role = current_power_role; 
	pd_txmsg_ptr->_msg_head.bits._port_data_role = current_data_role;
	pd_txmsg_ptr->_msg_head.bits._spec_revision = current_pd_version; 

  	// message payload is ready actually...
	if(!usbpd_message_transmission())
	{
		// send softreset:
		_send_softreset_message();
		// always failed...
		return false;
	}
	return true; 
}

bool _send_ctrl_message(uint8 msg_type)
{
	pd_txmsg_ptr->_msg_head.bits._message_type = msg_type;
	pd_txmsg_ptr->_msg_head.bits._num_data_objects = 0;
	pd_txmsg_ptr->_msg_head.bits._extended = 0; 
	return _send_general_message();			
} 

#if (SUPPORT_MULTI_CHUNK_MSG == 1)
bool _send_chunk_req_message(void)
{
   	struct pd_chk_message xdata * tx_chk_msg = (struct pd_chk_message xdata *)pd_txmsg_ptr; 
	struct pd_chk_message xdata * rx_chk_msg = (struct pd_chk_message xdata *)pd_rxmsg_ptr;
	// message header: 
	tx_chk_msg->_msg_head.bits._extended = rx_chk_msg->_msg_head.bits._extended; 
	tx_chk_msg->_msg_head.bits._message_type = rx_chk_msg->_msg_head.bits._message_type; 
	tx_chk_msg->_msg_head.bits._num_data_objects = 1;	
	// extend header:
	tx_chk_msg->_ext_head.bits._chunked = 1; 
	tx_chk_msg->_ext_head.bits._chunk_num = rx_chk_msg->_ext_head.bits._chunk_num + 1; 
	tx_chk_msg->_ext_head.bits._req_chunk  = 1;
	tx_chk_msg->_ext_head.bits._data_size0 = 0;
	tx_chk_msg->_ext_head.bits._data_size1 = 0; 
	// data objects:
	memset((void*)tx_chk_msg->_data_bytes, 0, CHK_MSG_MAX); 
	// send message:
	return _send_general_message();
}
#endif

#if (SUPPORT_MULTI_CHUNK_MSG == 1)
bool _send_chunk_resp_message(void)
{
	uint16 len; 
	uint16 total_size;  
   	struct pd_chk_message xdata * tx_chk_msg = (struct pd_chk_message xdata *)pd_txmsg_ptr; 
	struct pd_chk_message xdata * rx_chk_msg = (struct pd_chk_message xdata *)pd_rxmsg_ptr;

	// total size:
	total_size = pd_ext_txmsg_ptr->_ext_head.bits._data_size0; 
	total_size = (total_size << 8) + pd_ext_txmsg_ptr->_ext_head.bits._data_size1;

	// check left bytes:
	len = total_size - cur_ext_txmsg_index; 
	if(len > CHK_MSG_MAX)
		len = CHK_MSG_MAX; 

	// message header:
	tx_chk_msg->_msg_head.bits._extended = rx_chk_msg->_msg_head.bits._extended; 
	tx_chk_msg->_msg_head.bits._message_type = rx_chk_msg->_msg_head.bits._message_type; 
	tx_chk_msg->_msg_head.bits._num_data_objects = (uint8)((len + 2 + 3) >> 2);	
	// extend header:
	tx_chk_msg->_ext_head.bits._chunked = 1; 
	tx_chk_msg->_ext_head.bits._chunk_num = rx_chk_msg->_ext_head.bits._chunk_num; 
	tx_chk_msg->_ext_head.bits._req_chunk  = 0;
	tx_chk_msg->_ext_head.bits._data_size0 = pd_ext_txmsg_ptr->_ext_head.bits._data_size0;
	tx_chk_msg->_ext_head.bits._data_size1 = pd_ext_txmsg_ptr->_ext_head.bits._data_size1;
	// data:
	memset((void*)tx_chk_msg->_data_bytes, 0, CHK_MSG_MAX); 
	memcpy((void*)tx_chk_msg->_data_bytes, (void*)&pd_ext_txmsg_ptr->_data_bytes[cur_ext_txmsg_index], len); 
	cur_ext_txmsg_index += len; 
	// reset index if all completed!
	if(cur_ext_txmsg_index >= total_size)
		cur_ext_txmsg_index = 0; 
	// send it:
	return _send_general_message();	
}
#endif

bool _parse_extd_message(void)
{
	struct pd_chk_message xdata * rx_chk_msg = (struct pd_chk_message xdata *)pd_rxmsg_ptr;

	if(rx_chk_msg->_ext_head.bits._chunked == 0)
	{
		// we don't support unchunked message whatever how long it is...
		_send_ctrl_message(CTRL_MSG_NOT_SUPPORT); 
		return false; 
	}

	#if (SUPPORT_MULTI_CHUNK_MSG == 1)
	{
	uint16 len, total_size;  
	// first we check if it's chunk reqest...
	if(rx_chk_msg->_ext_head.bits._req_chunk)
	{
		// just a request, we repsonse it!
		_send_chunk_resp_message(); 
		// not need do anything...
		return false; 
	}
	// it's normal message, ohh...I mean it's not chunk request... 
	// check total data size:
	total_size = rx_chk_msg->_ext_head.bits._data_size0; 
	total_size = (total_size << 8) + rx_chk_msg->_ext_head.bits._data_size1; 
	if(total_size <= CHK_MSG_MAX)
	{
		// not need request/response mechanism:
		memcpy((void*)pd_ext_rxmsg_ptr, (void*)rx_chk_msg, sizeof(struct pd_chk_message)); 
		return true; 
	}
	else
	{  	
		// check if the startup...
		if(rx_chk_msg->_ext_head.bits._chunk_num == 0)
		{
			// record message:
			memcpy((void*)pd_ext_rxmsg_ptr, (void*)rx_chk_msg, sizeof(struct pd_chk_message));
			cur_ext_rxmsg_index = CHK_MSG_MAX; 
			// we have to send chunk_request message here...
			_send_chunk_req_message(); 
			// not complete yet...
			return false; 
		}
		else 
		{
			// in middle transfer...
			// 1. check data length of this message:
			len = rx_chk_msg->_msg_head.bits._num_data_objects * 4 - 2; // 4-bytes boundary...
			// 2. record data: 
			memcpy((void*)&pd_ext_rxmsg_ptr->_data_bytes[cur_ext_rxmsg_index], (void*)rx_chk_msg->_data_bytes, len);
			cur_ext_rxmsg_index += len; 
			// 3. check if completed...
			if(cur_ext_rxmsg_index < total_size)
			{
				// still not enough, send chunk request...
				_send_chunk_req_message(); 
				// not complete yet...
				return false; 	
			}
			else
			{
				// reset index if all completed!
				cur_ext_rxmsg_index = 0; 
				// ok, it's enough...
				return true; 
			}
		}
	}
	}
	#else // #if (SUPPPORT_MULTI_CHUNK_MSG == 1) 
	{
		// not need request/response mechanism, copy directly...
		memcpy((void*)pd_ext_rxmsg_ptr, (void*)rx_chk_msg, sizeof(struct pd_chk_message)); 
		return true; 
	}
	#endif // #if (SUPPPORT_MULTI_CHUNK_MSG == 1)
}

bool _send_extend_message(void)
{
	uint16 len; 
	uint16 total_size; 
	struct pd_chk_message xdata * tx_chk_msg = (struct pd_chk_message xdata *)pd_txmsg_ptr; 
	struct pd_chk_message xdata * rx_chk_msg = (struct pd_chk_message xdata *)pd_rxmsg_ptr;
 
	// check if it can be transfer in one chunk message:
	total_size = pd_ext_txmsg_ptr->_ext_head.bits._data_size0; 
	total_size = (total_size << 8) + pd_ext_txmsg_ptr->_ext_head.bits._data_size1;
	len = (total_size > CHK_MSG_MAX) ? CHK_MSG_MAX : total_size; 

	// 1. message header:
	tx_chk_msg->_msg_head.bits._extended = 1; 
	tx_chk_msg->_msg_head.bits._message_type = pd_ext_txmsg_ptr->_msg_head.bits._message_type; 
	tx_chk_msg->_msg_head.bits._num_data_objects = (uint8)((len + 2 + 3) >> 2); // 4-bytes boundary...
	// 2. extend header:
	tx_chk_msg->_ext_head.bits._chunked = 1;  // double confirm...
	tx_chk_msg->_ext_head.bits._chunk_num = 0; 
	tx_chk_msg->_ext_head.bits._req_chunk = 0;
	tx_chk_msg->_ext_head.bits._data_size0 = pd_ext_txmsg_ptr->_ext_head.bits._data_size0;
	tx_chk_msg->_ext_head.bits._data_size1 = pd_ext_txmsg_ptr->_ext_head.bits._data_size1;
	// 3. data objects:
	memset((void*)tx_chk_msg->_data_bytes, 0, CHK_MSG_MAX); 
	memcpy((void*)tx_chk_msg->_data_bytes, (void*)pd_ext_txmsg_ptr->_data_bytes, len); 
	cur_ext_txmsg_index = len; 
	// ok, send it:
	return _send_general_message(); 
}

void _do_physical_bist(void)
{
	uint8 mode = pd_rxmsg_ptr->_data_objects[0] & 0xF0; // bit[31:24] - not do endian swap...
	switch(mode)
	{
	case 0x50: // we only process BIST Carrier Mode 2 
		// enable bist mode...
		//regmap2D(0x12) |= 0x80; 
		// delay 30-60ms
		sw_delay_10ms();
		sw_delay_10ms();
		sw_delay_10ms();
		sw_delay_10ms();
		// disable bist mode...
		//regmap2D(0x12) &= 0x7F;
		break;
	}
	physical_bist_counter += 1;
	if(physical_bist_counter == 0)
	{   
		// keep it not be zero...
		physical_bist_counter = 1;  
	}
}

////////////////////////////////////// source port state machine ////////////////////////////////////////////

void _pe_src_port_states(uint8 state)
{
	uint8 old_state = current_port_state; // record previous state...

	// record current state:
	current_port_state = state;

	switch(state)
	{
	case PE_SRC_HARD_RESET:
		{
		// send hardreset signal:
		usbpd_send_hardreset(0); 
		}
		// no break!!!
	case PE_SRC_HARD_RESET_RECEIVED:
		{
		// as no break above: 
		current_port_state = PE_SRC_HARD_RESET_RECEIVED;
		// Start PSHardResetTimer:
		PE_TIMER_START(1, TIMER_PS_HARD_RESET); 	
		}
		break; 
	case PE_SRC_TRANSITION_TO_DEFAULT:
		{
		// Reset protocol layer, which should in StartUp stage...
		usbpd_protocol_reset(); 
		// reset policy layer:
		usbpd_policy_reset(); 
	
		// flag that contract not valid:
		current_contract_status = 0; 
	
		// request device policy manager to request power sink transition to default:
		// reset local hardware	
		// if type-C set port data role to UFP and turn off VCONN:
		// set output to vSafe5v:  
		_switch_out_current(300); 
		_switch_out_voltage(100); // default is 5V@3A...
		// enable output: 
		CHARGE_ENABLE(1); 
		}
		// no break!!! 
	case PE_SRC_STARTUP: 
		{
		// as no break above: 
		current_port_state = PE_SRC_STARTUP;
		// Reset CapsCounter:
		src_caps_counter = 0; 
		// here, we need discover cable type to see if it support 5A...
		#if (SUPPORT_CABLE_DISCOVERY == 1)
		if(_do_cable_discover_identify())
		{
			// if respond, start timer to wait message...
			PE_TIMER_START(0, TIMER_SOURCE_CAPABILITY);
			break;
		} 
		#endif 
		}
		// no break!!! 
	case PE_SRC_SEND_CAPABILITIES: 
		{
		// as no break above:
		current_port_state = PE_SRC_SEND_CAPABILITIES; 
		// build source capabilities message:
		_src_build_source_capabilities();
		// increase capscounter:
		src_caps_counter += 1; 
		// send capability:
		if(usbpd_message_transmission())
		{
			// success!	- Start SenderResponseTimer:
			PE_TIMER_START(0, TIMER_SENDER_RESPONSE);
		}
		else
		{
			// check if need to continue send capabilities:	150ms/time
			usbpd_protocol_reset(); // reset message id...
			if(src_caps_counter < 50) // 3 times in previous...   30 * 150ms = 4.5s 
			{
				// send failed! - Start SourceCapabilityTimer to re-send...
				PE_TIMER_START(0, TIMER_SOURCE_CAPABILITY); 
			}
		}
		}
		break; 
	case PE_SRC_NEGOTIATE_CAPABILITY:
		{
		// stop SenderResponseTimer:
		PE_TIMER_STOP(0, TIMER_SENDER_RESPONSE); 
		// evaluate the sink request...
		_src_evaluate_sink_request();	
		// pps common timer:
		if(current_is_pps_request)
		{
			PE_TIMER_STOP(2, TIMER_SOURCE_PPS_COMM);
		}	
		}
		break; 
	case PE_SRC_CAPABILITY_RESPONSE:
		{
		// here, we reject the request as we didn't consider power reserve...
		_send_ctrl_message(CTRL_MSG_REJECT); 
		}
		break; 
	case PE_SRC_WAIT_NEW_CAPABILITIES:
		{
		// do nothing...
		}
		break; 
	case PE_SRC_TRANSITION_SUPPLY:
		{
		// Initialize and run SourceActivityTimer
		// If GotoMin send GotoMin Message, else send Accept Message
		if(!_send_ctrl_message(CTRL_MSG_ACCEPT)) // for Lecroy test, we have to check if success - 2018.10.22
			break; 
		// Wait tSrcTransition(25~35ms) and request Device Policy Manager to transition power supply:
		sw_delay_10ms(); 
		sw_delay_10ms();
		sw_delay_10ms();
		// switch to target power...
		_switch_out_current(src_negotiated_current); 
		_switch_out_voltage(src_negotiated_voltage); 
		// Send PS_RDY message:
		_send_ctrl_message(CTRL_MSG_PS_RDY); 
		}
		// no break!!! 
	case PE_SRC_READY:
		{
		// as no break above: 
		current_port_state = PE_SRC_READY;
		// flag that contract created:
		current_contract_status = 1; 
		// start source pps common timer for pps: 
		if(current_is_pps_request)
		{
			PE_TIMER_START(2, TIMER_SOURCE_PPS_COMM); 	
		}
		}
		break; 
	case PE_SRC_GIVE_SOURCE_CAP:
		{
		// build source capabilities message:
		_src_build_source_capabilities();
		// send capability: whatever success or failed...
		_send_general_message();
		// this is not a valud state:
		current_port_state = old_state; 
		}
		break;
	}
}

void _src_build_source_capabilities(void)
{
	// now we didn't do any modification...but we might modify something in future...
	memcpy((void*)pd_txmsg_ptr, (void*)&g_our_capabilities, sizeof(struct pd_message));
}

// true: can be met - false: can't be met or can be met later...
void _src_evaluate_sink_request(void)
{ 
	uint8 pdo_position, version; 
	uint16 opr_current; 
	uint16 opr_voltage; // for pps pdo... 
	uint16 max_current; // for fix pdo...
	uint16 cap_current; 
	uint32 cur_cap_pdo;  

	// check parter version:
	version = pd_rxmsg_ptr->_msg_head.bits._spec_revision;
	if(version < SPEC_REV30)
	{
		// if partner is not pd3.0, we go back to what it is...
		current_pd_version = version;  
	} 

	// evaluate power: only one power data object exist...
	// fix & variable RDO...
	// bit[30-28]: object position
	// bit[27]: giveback flag
	// bit[26]: cabability mismatch
	// bit[25]: usb communication capable
	// bit[24]: no usb suspend
	// bit[23]: unchunked extended message supported
	// bit[19-10]: operating current in 10mA
	// bit[09-00]: max operating current in 10mA 

	// little-endian -> big-endian:
	data_object_t = V32_ENDIAN_SWAP(pd_rxmsg_ptr->_data_objects[0]);

	// check position: 
	pdo_position = (data_object_t >> 28) & 0x07;
	if(pdo_position == 0 || pdo_position > cur_pdo_num)
	{
		// position should be valid...
		snk_req_evaluate_result = 0; 
		return; 	
	}

	// check capability mismatch bit first:
	if((data_object_t >> 26) & 0x01)
	{
		// we cannot meet the sink's requirement...
		snk_req_evaluate_result = 0; 
		return;  	
	}

	// we need still check if the voltage and current exceed our capablitity...
	cur_cap_pdo = V32_ENDIAN_SWAP(g_our_capabilities._data_objects[pdo_position-1]);
	// check if pps request...
	switch((cur_cap_pdo >> 30) & 0x03) 
	{
	case 0x03: // pps request:
		current_is_pps_request = 1; 
		break; 
	case 0x00: // fix request:
		current_is_pps_request = 0; 
		break;
	default:
		snk_req_evaluate_result = 0;
		return;   
	}
	
	if(current_is_pps_request)
	{
		// pps request...
		opr_voltage = (data_object_t >> 9) & 0x7FF;  // bit 19-9 in 20mV 
		opr_current = (data_object_t >> 0) & 0x07F;  // bit 6-0 in 50mA 
		// only current capabilty...
		cap_current = cur_cap_pdo & 0x000000FF; // bit 7-0 in 50mA
	}
	else
	{
		// fix request...
	   	opr_voltage = (cur_cap_pdo >> 10) & 0x3FF; // bit 19-10 in 50mV --- opr_voltage = cap_voltage
		opr_current = (data_object_t >> 10) & 0x3FF; // bit 19-10 in 10mA
		max_current = (data_object_t >> 0) & 0x3FF; // bit 9-0 in 10mA
		// only current capability...
		cap_current = (cur_cap_pdo & 0x3FF) * 10; // bit 9-0 in 10mA
	}

	{
		// judge if exceed what we have...
		if(opr_current > cap_current || max_current > cap_current) 
		{
			snk_req_evaluate_result = 0;
			return;
		}
	
		// record negotiated volatage & current:
		src_negotiated_voltage = opr_voltage; // voltage...
		src_negotiated_current = opr_current; 
	}

	// OK! 
	snk_req_evaluate_result = 1; 		
}	

// response data swap: 
bool _dfp2ufp_response_data_swap(uint8 step)
{
	switch(step)
	{
	case 1: 
		// we receive dr_swap, just accept...
		if(!_send_ctrl_message(CTRL_MSG_ACCEPT))
			return false;
		break; 
	case 2: 
		// update and wait for alt-mode operation...
		current_data_role = PORT_DATA_ROLE_UFP; 
		break;  	
	}
	return true; 
}	

#if (SUPPORT_CABLE_DISCOVERY == 1)
bool _do_cable_discover_identify(void)
{
	// message header:
	pd_txmsg_ptr->_msg_head.bits._message_type = DATA_MSG_VENDOR_DEFINED;
	pd_txmsg_ptr->_msg_head.bits._num_data_objects = 1;

	// vdm_header:
	pd_txmsg_ptr->_data_objects[0] = V32_ENDIAN_SWAP(0xFF008001);
	
	// send it directly without softreset even failed...
	{
		uint8 i; 
		for(i=0; i<20; ++i)
		{
			// set sop type to SOP'...
			usbpd_set_msg_type(MSG_TYPE_SOP1P);
			// send directly, not cause soft reset... 
			if(usbpd_message_transmission())
				break; 
			// delay some time:
			sw_delay_1ms(); 
		} 
		return (i < 20) ? true : false; 
	}
}
#endif

///////////////////////////////////// assist routines /////////////////////////////////////////////

/*
void _init_our_capabilities(uint8 pdo_num)
{
	uint8 i;
	uint16 voltage;
	uint16 current; 
	// init our source capabilities as maybe we use external power...
	// when we connect to charger, which will over-write this with its capabilities...
	// header:
	g_our_capabilities._msg_head.bits._num_data_objects = pdo_num; 
	g_our_capabilities._msg_head.bits._port_power_role = current_power_role;
	g_our_capabilities._msg_head.bits._spec_revision = current_pd_version;
	g_our_capabilities._msg_head.bits._port_data_role = current_data_role;
	g_our_capabilities._msg_head.bits._message_type = DATA_MSG_SOURCE_CAPS;
	g_our_capabilities._msg_head.bits._extended = 0;
	// data object:
	// dual-role power (bit-29) = 0
	// usb suspend	(bit-28) = 0
	// external power (bit-27) = 1
	// USB communication capable (bit-26) = 1
	// data role swap (bit-25) = 1
	// unchunk extend message support (bit-24) = 0
	// reserved (bit 23-22) = 0
	// peak current (bit 21-20) = 00
	// voltage (bit 19-10)
	// current (bit 09-00)
	// mandatory data object - 5V/2A: 
	// calculate voltage/current:
	voltage = (PREDEF_VOL[0] / 50); 
	current = (PREDEF_CUR[0] / 10); 
	data_object_t = 0x08000000 | ((uint32)voltage << 10) | current; 
	g_our_capabilities._data_objects[0] = V32_ENDIAN_SWAP(data_object_t);
	for(i = 1; i < pdo_num; ++i)
	{
		// calculate voltage/current:
		voltage = (PREDEF_VOL[i] / 50); 
		current = (PREDEF_CUR[i] / 10); 
	   	// data object:
		data_object_t = 0x00000000 | ((uint32)voltage << 10) | current;
		g_our_capabilities._data_objects[i] = V32_ENDIAN_SWAP(data_object_t);
	}
}
*/ 

void _build_our_capabilities(uint8 pdp_sel)
{
	uint8 i;  

	// ** data object for PDO: 
	// dual-role power (bit-29) = 0
	// usb suspend	(bit-28) = 0
	// external power (bit-27) = 1
	// USB communication capable (bit-26) = 1
	// data role swap (bit-25) = 1
	// unchunk extend message support (bit-24) = 0
	// reserved (bit 23-22) = 0
	// peak current (bit 21-20) = 00
	// voltage (bit 19-10)
	// current (bit 09-00)

	// ** data object for APDO:
	// type (bit 31-30) = 11
	// power limit (bit 27) = 1
	// max voltage (bit 24-17)
	// min voltage (bit 15-8)
	// max current (bit 7-0)

	if(pdp_sel < PDO_12V_SEL)
	{
		cur_pdo_num = PDO_12V_NUM; 
		for(i=0; i<PDO_12V_NUM; ++i)
			g_our_capabilities._data_objects[i] = V32_ENDIAN_SWAP(PDO_12V_VAL[pdp_sel][i]);	
	}
	else 
	{
		cur_pdo_num = PDO_20V_NUM; 
		for(i=0; i<PDO_20V_NUM; ++i)
			g_our_capabilities._data_objects[i] = V32_ENDIAN_SWAP(PDO_20V_VAL[pdp_sel-3][i]);
	}

	// init our source capabilities as maybe we use external power...
	// when we connect to charger, which will over-write this with its capabilities...
	// header:
	g_our_capabilities._msg_head.bits._num_data_objects = cur_pdo_num; 
	g_our_capabilities._msg_head.bits._port_power_role = current_power_role;
	g_our_capabilities._msg_head.bits._spec_revision = current_pd_version;
	g_our_capabilities._msg_head.bits._port_data_role = current_data_role;
	g_our_capabilities._msg_head.bits._message_type = DATA_MSG_SOURCE_CAPS;
	g_our_capabilities._msg_head.bits._extended = 0;
} 

// switch our power to negotiatied power cap...
void _switch_out_voltage(uint16 target_voltage)
{
	// voltage:	unit - 20mV for pps, 50mV for pd
	power_set_voltage(target_voltage, current_is_pps_request); 
}

void _switch_out_current(uint16 target_current)
{
	// current: unit - 50mA for pps, 10mA for pd
	power_set_current(target_current, current_is_pps_request);  
}

//////////////////////////////////////// PD3.0 New Message ////////////////////////////////////////

bool SNK_IS_SinkTxOK(void)
{	
	return true; 
} 

void _build_source_caps_extend_message(void)
{
	// response of get_source_cap_extened message...
	// message header: 
	pd_ext_txmsg_ptr->_msg_head.bits._message_type = EXTD_MSG_SOURCE_CAPS_EXT; 
	// extend header: 
	pd_ext_txmsg_ptr->_ext_head.bits._data_size0 =  0; 
	pd_ext_txmsg_ptr->_ext_head.bits._data_size1 = 24; 
	// data: 24 bytes
	memset((void*)pd_ext_txmsg_ptr->_data_bytes, 0x00, 24); // reset all values... 
}

// true: battery change - false: other changes...
bool _src_evaluate_sink_alert(void)
{
	// little-endian -> big-endian:
	data_object_t = V32_ENDIAN_SWAP(pd_rxmsg_ptr->_data_objects[0]);

	// Bit-25: Battery Status Change:
	if((data_object_t >> 25) & 1) 
	{
		return true; 
	}
	else
	{
		return false; 
	}
}

// when we receive get_status message...
void _build_status_message(void)
{
	 
}

// when receive status message...
void _src_evaluate_sink_status(void)
{
	
}

void _src_evaluate_sink_caps_extend(void)
{
	
}

void _build_pps_status_message(void)
{
	// message header: 
	pd_ext_txmsg_ptr->_msg_head.bits._message_type = EXTD_MSG_PPS_STATUS; 
	// extend header: 
	pd_ext_txmsg_ptr->_ext_head.bits._data_size0 = 0; 
	pd_ext_txmsg_ptr->_ext_head.bits._data_size1 = 4; 
	// data: 4 bytes
	pd_ext_txmsg_ptr->_data_bytes[0] = 0xFF;  // output voltage in 20mV...
	pd_ext_txmsg_ptr->_data_bytes[1] = 0xFF;
	pd_ext_txmsg_ptr->_data_bytes[2] = 0xFF;  // output current in 50mA...
	// bit-[2:1]: PTF - Persent Temperature Flag 
	// bit-[3]: OMF - Operation Mode Flag (1: CL(Current Limit), 0: CV(Contant Voltage)) 
	pd_ext_txmsg_ptr->_data_bytes[3] = 0x00; 	
}

///////////////////////////////////////// misc routines ///////////////////////////////////////////

uint8 extract_apdo_voltage(uint8 max_100mv)
{
	if(max_100mv >= 160) // 16V or 21V -> 15V or 20V 
		return (max_100mv - 1);
	else if(max_100mv > 90) // 11V -> 9V 
		return 90; 
	else  // 9V -> 5V 
		return 50;	
}

// state transition:
void usbpd_state_transition(uint8 new_state)
{
	new_port_state = new_state;
	usb_pd_add_event(0, USBPD_EVT_STATE_TRANSITION); 
}
	   
///////////////////////////////////////// general timers //////////////////////////////////////////

// ms timer0: 
void usbpd_ms_timer_start(uint8 index, uint16 count)
{
	// confirm to stop:
	PE_TIMER_STOP(3,  (1 << (0 + index))); 
	// set count: 
	regmap0D(0x32+index*2) = (count >> 0) & 0xFF; 
	regmap0D(0x33+index*2) = (count >> 8) & 0xFF;
	// start:
	PE_TIMER_START(3, (1 << (0 + index))); 
}

uint8 usbpd_ms_timer_is_timeout(uint8 index)
{
	// check raw interrupt status:
	if(regmap0B(0x16) & (1 << (0 + index)))
	{
		// OK! timeout! - don't forget to clear interrupt:
		regmap0B(0x06) = (1 << (0 + index));
		// return:
		return 1; 
	}
	else
	{
		// not overflow yet...
		return 0; 
	}
} 

// us timer0: 
void usbpd_us_timer_start(uint8 index, uint16 count)
{
	// confirm to stop:
	PE_TIMER_STOP(3, (1 << (3 + index))); 
	// set count: 
	regmap0D(0x38+index*2) = (count >> 0) & 0xFF;
	regmap0D(0x39+index*2) = (count >> 8) & 0xFF; 
	// start:
	PE_TIMER_START(3, (1 << (3 + index)));	
} 

uint8 usbpd_us_timer_is_timeout(uint8 index)
{
	// check raw interrupt status:
	if(regmap0B(0x16) & (1 << (3 + index)))
	{
		// OK! timeout! - don't forget to clear interrupt:
		regmap0B(0x06) = (1 << (3 + index));
		// return:
		return 1; 
	}
	else
	{
		// not overflow yet...
		return 0; 
	}	
}

// hardware delay...using timer2 as delay timer...
void usbpd_hw_delay_ms(uint16 ms)
{
	usbpd_ms_timer_start(2, ms); 
	while(!usbpd_ms_timer_is_timeout(2))
	{
	}		
}

void usbpd_hw_delay_us(uint16 us)
{
	usbpd_us_timer_start(2, us);
	while(!usbpd_us_timer_is_timeout(2))
	{
	}
} 



#endif
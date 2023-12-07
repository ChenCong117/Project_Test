#ifndef _USB_PD_COMMON_H
#define _USB_PD_COMMON_H

// control if support multi-chunk message...
#define SUPPORT_MULTI_CHUNK_MSG			0 	// default set to 0 for save code...

// control if support cable discovery...
#define SUPPORT_CABLE_DISCOVERY			0	// default set to 0 for save code...

// interrupt enable/disable:
#define INT_ENABLE()		do{EA=1;}while(0)
#define INT_DISABLE()		do{EA=0;}while(0)

// pd message header bit define:
// power role:
#define PORT_POWER_ROLE_SINK      		0
#define PORT_POWER_ROLE_SOURCE    		1
// data role:
#define PORT_DATA_ROLE_UFP        		0
#define PORT_DATA_ROLE_DFP        		1
// specification revsion:
#define SPEC_REV10                		0
#define SPEC_REV20                		1
#define SPEC_REV30						2 // PD3.0...

// control message type: 
#define CTRL_MSG_GOODCRC       			0x01
#define CTRL_MSG_GOTOMIN          	  	0x02
#define CTRL_MSG_ACCEPT             	0x03
#define CTRL_MSG_REJECT             	0x04
#define CTRL_MSG_PING               	0x05
#define CTRL_MSG_PS_RDY             	0x06
#define CTRL_MSG_GET_SOURCE_CAP     	0x07
#define CTRL_MSG_GET_SINK_CAP       	0x08
#define CTRL_MSG_DR_SWAP            	0x09
#define CTRL_MSG_PR_SWAP            	0x0A
#define CTRL_MSG_VCONN_SWAP         	0x0B
#define CTRL_MSG_WAIT               	0x0C
#define CTRL_MSG_SOFT_RESET         	0x0D
#define CTRL_MSG_NOT_SUPPORT			0x10 // PD3.0
#define CTRL_MSG_GET_SOURCE_CAP_EXT		0x11 
#define CTRL_MSG_GET_STATUS				0x12
#define CTRL_MSG_FR_SWAP				0x13 
#define CTRL_MSG_GET_PPS_STATUS			0x14
#define CTRL_MSG_GET_COUNTRY_CODES		0x15
#define CTRL_MSG_GET_SINK_CAP_EXT		0x16

// data message type:
#define DATA_MSG_SOURCE_CAPS 			0x01
#define DATA_MSG_REQUEST            	0x02
#define DATA_MSG_BIST               	0x03
#define DATA_MSG_SINK_CAPS   			0x04
#define DATA_MSG_BATTERY_STATUS			0x05 // PD3.0
#define DATA_MSG_ALERT					0x06
#define DATA_MSG_GET_COUNTRY_INFO		0x07
#define DATA_MSG_VENDOR_DEFINED    		0x0F

// extended message type: PD3.0
#define EXTD_MSG_SOURCE_CAPS_EXT		0x01
#define EXTD_MSG_STATUS					0x02
#define EXTD_MSG_GET_BATTERY_CAP		0x03
#define EXTD_MSG_GET_BATTERY_STATUS		0x04
#define EXTD_MSG_BATTERY_CAPS			0x05
#define EXTD_MSG_GET_MANU_INFO			0x06
#define EXTD_MSG_MANU_INFO				0x07
#define EXTD_MSG_SECURITY_REQ			0x08
#define EXTD_MSG_SECURITY_RESP			0x09
#define EXTD_MSG_FW_UPDATE_REQ			0x0A
#define EXTD_MSG_FW_UPDATE_RESP			0x0B
#define EXTD_MSG_PPS_STATUS				0x0C
#define EXTD_MSG_COUNTRY_INFO			0x0D
#define EXTD_MSG_COUNTRY_CODES			0x0E
#define EXTD_MSG_SINK_CAPS_EXT			0x0F

/**
 * VDM MESSAGE...
 */

// vdm message header bits define:
// command type:
#define TYPE_INITIATOR         			0x00
#define TYPE_RESP_ACK          			0x01
#define TYPE_RESP_NAK          			0x02
#define TYPE_RESP_BUSY         			0x03
// command define:
#define VDM_CMD_DISCOVER_IDENTITY  		0x01
#define VDM_CMD_DISCOVER_SVIDS     		0x02
#define VDM_CMD_DISCOVER_MODES     		0x03
#define VDM_CMD_ENTER_MODE         		0x04
#define VDM_CMD_EXIT_MODE          		0x05
#define VDM_CMD_ATTENTION          		0x06  
#define VDM_CMD_DP_STATUS_UPDATE   		0x10
#define VDM_CMD_DP_CONFIGURE       		0x11
// command response type for qc4:
#define VDM_RESP_QC_ACK					0xA0
#define VDM_RESP_QC_NACK				0x50
// command define for qc4:
#define VDM_CMD_QC_INQUIRE_CHG_TEMP		0x10
#define VDM_CMD_QC_INQUIRE_CON_TEMP		0x0B
#define VDM_CMD_QC_INQUIRE_CON_VOL 		0x06
#define VDM_CMD_QC_INQUIRE_CHG_TYPE		0x0C  
#define VDM_CMD_QC_INQUIRE_CHG_VER		0x0E

/**
 * attention: message in register map is little-endian, however,
 * 			  our memory modal is big-endian, so we have to conver it...
 */

//usb pd message structure: 
union pd_msg_head
{
	struct  
	{
		uint8 _message_type  	: 5;
		uint8 _port_data_role  	: 1;
		uint8 _spec_revision    : 2;		
		uint8 _port_power_role	: 1;
		uint8 _message_id		: 3;
		uint8 _num_data_objects	: 3;		
		uint8 _extended	    	: 1; // PD3.0...		    
	} bits;
	uint16 _value;	
};
struct pd_message
{
	union pd_msg_head _msg_head;
	uint32 _data_objects[7];	
};


//vendor define message:
union pd_vdm_head
{
	struct  
	{
		uint8 _command 		: 5;
		uint8 _reserved0 	: 1;
		uint8 _command_type	: 2;
		uint8 _object_pos 	: 3;		
		uint8 _reserved1 	: 2;
		uint8 _vdm_version	: 2;
		uint8 _vdm_type		: 1;		
		uint8 _svid0		: 8;		
        uint8 _svid1		: 8;    
	} bits;
	struct
	{
		uint8 _command0		: 8; 
		uint8 _command1		: 7;
		uint8 _vdm_type		: 1;
		uint8 _svid0		: 8;
		uint8 _svid1		: 8; 
	} bits_qc; // for qc4...
	uint32 _value;	
};
struct pd_vdm_message
{
	union pd_msg_head _msg_head;
	union pd_vdm_head _vdm_head;
	uint32 _data_objects[6];	
};

// extended message - PD3.0
union pd_ext_head
{
	struct 
	{
		uint8 _data_size1 	: 8; 
		uint8 _data_size0	: 1;
		uint8 _reserved		: 1;
		uint8 _req_chunk	: 1;
		uint8 _chunk_num	: 4;
		uint8 _chunked		: 1;
	} bits;
	uint16 _value; 
};

#define CHK_MSG_MAX			 26
struct pd_chk_message
{
	union pd_msg_head _msg_head;
	union pd_ext_head _ext_head;
	uint8 _data_bytes[CHK_MSG_MAX]; // Compatible with PD2.0 message...
};

#if (SUPPORT_MULTI_CHUNK_MSG == 1)
#define EXT_MSG_MAX			260 
#else 
#define EXT_MSG_MAX			CHK_MSG_MAX // for saving code space...
#endif 
struct pd_ext_message
{
	union pd_msg_head _msg_head;
	union pd_ext_head _ext_head;
	uint8 _data_bytes[EXT_MSG_MAX]; // flexible size...
};

// usb pd event type define (max 16 events supported): 
#define USBPD_EVT_HARDRST_RECEIVED			(1 <<  0)	// hardreset received...
#define USBPD_EVT_SRC_CAPS_CHANGED			(1 <<  1)	// source capabilities changed (when in source role)
#define USBPD_EVT_SNK_REQ_CHANGED	  		(1 <<  2)	// sink request changed (when in sink role)
#define USBPD_EVT_DO_POWER_SWAP				(1 <<  3)	// need to do power swap...
#define USBPD_EVT_PORT_DISCONNECTED			(1 <<  4)	// port disconnected...
#define USBPD_EVT_PORT_CONNECTED			(1 <<  5)	// port connected...
#define USBPD_EVT_STATE_TRANSITION			(1 <<  6) 	// port state transition...

// interrupt mask define:
// bmc rx:
#define BIT_RX_RCV_MESSAGE				(1 << 0)
#define BIT_RX_RCV_HARDRST				(1 << 1)
#define BIT_RX_RCV_CABLERST				(1 << 2)
#define BIT_RX_CHANNEL_CHG				(1 << 3) 	// idle->busy or busy->idle
// bmc tx:							
#define BIT_TX_SEND_MESSAGE_DONE		(1 << 0)	// message except GoodCRC message
#define BIT_TX_SEND_GOODCRC_DONE		(1 << 1)
#define BIT_TX_SEND_HARDRST_DONE		(1 << 2) 	// hard reset/cable reset
#define BIT_TX_MESSAGE_DISCARDED		(1 << 3)	
#define BIT_TX_GOODCRC_DISCARDED		(1 << 4)
#define BIT_TX_FRAME_GAP_EXPIRED		(1 << 5)
// timer index-0:
#define BIT_TIMER_NO_RESPONSE			(1 << 0)
#define BIT_TIMER_HARDRST_COMPLETE		(1 << 1)
#define BIT_TIMER_CRC_RECEIVE			(1 << 2)
#define BIT_TIMER_PS_TRANSITION			(1 << 3)
#define BIT_TIMER_SENDER_RESPONSE		(1 << 4)
#define BIT_TIMER_SINK_REQUEST			(1 << 5)
#define BIT_TIMER_SINK_WAIT_CAP			(1 << 6)
#define BIT_TIMER_SOURCE_CAPABILITY		(1 << 7)
// timer index-1:
#define BIT_TIMER_PS_HARD_RESET			(1 << 0)
#define BIT_TIMER_VCONN_ON				(1 << 1)
#define BIT_TIMER_VCONN_OFF				(1 << 2)
#define BIT_TIMER_BIST_CONT_MODE		(1 << 3)
#define BIT_TIMER_VDM_RESPONSE			(1 << 4)
#define BIT_TIMER_DISCOVER_IDENTITY		(1 << 5)
#define BIT_TIMER_SINK_TX				(1 << 6)
#define BIT_TIMER_CHUNK_NOT_SUPPORT		(1 << 7)
// timer index-2: 
#define BIT_TIMER_CHUNK_SENDER_REQ		(1 << 0)
#define BIT_TIMER_CHUNK_SENDER_RESP		(1 << 1)
#define BIT_TIMER_SOURCE_PPS_COMM		(1 << 2)
// timer index-3:							
#define BIT_TIMER_MS_GENERAL0			(1 << 0)
#define BIT_TIMER_MS_GENERAL1			(1 << 1)
#define BIT_TIMER_MS_GENERAL2			(1 << 2)
#define BIT_TIMER_US_GENERAL0			(1 << 3)
#define BIT_TIMER_US_GENERAL1			(1 << 4)
#define BIT_TIMER_US_GENERAL2			(1 << 5)
// vbus change detect: 
#define BIT_VBUS_CHG_READY				(1 << 0)
// cc voltage detect:
#define BIT_CC1_VOL_CHG					(1 << 7)
#define BIT_CC2_VOL_CHG					(1 << 6)
#define BIT_CC1_VOL_CHG_CCDB			(1 << 1)
#define BIT_CC2_VOL_CHG_CCDB			(1 << 0)

// for public debug: 
extern uint8 xdata g_usbpd_trace[16];  // at 0x4050 - 0x405F

// ams operation: 
#define SRC_AMS_SinkTxNG()  do{;}while(0) // 5V@1.5A
#define SRC_AMS_SinkTxOK()  do{;}while(0) // 5V@3.0A
bool SNK_IS_SinkTxOK(void);

#endif
#ifndef _USBPD_PROTOCOL_H
#define _USBPD_PROTOCOL_H

#include "includes.h"
#include "../usbpd_common.h"

// bmc rx interrupt signals:
extern bit sig_goodcrc_received; 
	
// bmc tx interrupt signals:
extern bit sig_send_message_done;   
extern bit sig_send_goodcrc_done;
extern bit sig_send_hardrst_done;
extern bit sig_message_discarded;
extern bit sig_goodcrc_discarded;

// init routines:
void usbpd_protocol_init(void);

// protocol layer reset:
void usbpd_protocol_reset(void);

// received message process:
// not use queue, but only one message buffer...
extern struct pd_message xdata * pd_rxmsg_ptr;
// process receive message:
void usbpd_message_reception(void); 	
// precheck if message is goodcrc
bool usbpd_check_goodcrc(void);
// reply good crc:
bool usbpd_reply_goodcrc(void); 

// send message process:
// not use queue, but only one message buffer...
extern struct pd_message xdata * pd_txmsg_ptr; 
// general message including softreset...
bool usbpd_message_transmission(void);
// set message type: sop/sop'/sop''
#define MSG_TYPE_SOP		(1 << 0)
#define MSG_TYPE_SOP1P		(1 << 1)
#define MSG_TYPE_SOP2P  	(1 << 2)
#define usbpd_set_msg_type(t) 	do{regmap0D(0x47) = 0x40 | (t);}while(0)

// hard(cable)reset process:
// send hard(cable) reset...
void usbpd_send_hardreset(uint8 is_cable);
// hard(cable) reset received...
void usbpd_hardreset_received(void);

// pd3.0 mesage pointer:
extern struct pd_ext_message xdata * pd_ext_rxmsg_ptr; 
extern struct pd_ext_message xdata * pd_ext_txmsg_ptr;


#endif

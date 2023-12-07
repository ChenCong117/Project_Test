#ifndef _EVENT_H
#define _EVENT_H


#include "includes.h"

// To save xdata/code space, we might use tidy version,
// set following macro to "YES" to enable tidy version...
#define EVENT_FULL	   		1
#define EVENT_TINY			2
#define EVENT_LEVEL_SET		EVENT_FULL	

// control if support callback function:
#define CB_FUNC_ENABLE		0			 

/**
 * event type define: based on modules
 */
enum {
	EVT_USB_PD = 0,	 		// event belong to dprx
	EVT_TYPE_NUM			// event gate
};
#define EVT_UN_DEF		EVT_TYPE_NUM


/**
 * param in struct event_des define:
 * attention! this is different with register map!!!
 */

//usb pd event types:
#define PBIT_USBPD_PROBE				(1 << 7)	//probe periodly...
#define PBIT_USBPD_CONNECT_DETECT		(1 << 4)	//detect port connetion...
#define PBIT_USBPD_PEEVT_PENDING		(1 << 3)	//policy engine timer need process...
#define PBIT_USBPD_PETMR_PENDING		(1 << 2)	//policy engine event need process...
#define PBIT_USBPD_PEMSG_PENDING		(1 << 1)	//policy engine message need process...
#define PBIT_USBPD_PRLMSG_RECEIVED		(1 << 0)	//protocol message received... 

//external command paramter: command type:
#define CMD_TYPE_BASE		 0
#define CMD_ENTER			 (CMD_TYPE_BASE + 0)
#define CMD_MOV_UP			 (CMD_TYPE_BASE + 1)
#define CMD_MOV_DN			 (CMD_TYPE_BASE + 2)


/*
 * event descriptor
 * 
 * param has different meaning according to type, as below:
 * XXX - DP, HDMI, TTL, LVDS, DAC, AUDIO, see above for details...
 * 
 * |	type 		|		param		|
 * --------------------------------------
 * | EVT_XXX_TX		| interrupt status	|
 * --------------------------------------
 * | EVT_XXX_RX		| interrupt status 	|
 * --------------------------------------
 * | EVT_EXT_CMD	|	command type	|
 * --------------------------------------
 */
struct event_des{
	struct event_des* next;
	uint8 type;
	uint8 param;	// be different according to different type....
};


/**
 * external interface
 */
//init:
void event_initialize(void);
//check if there is event to be processed...
uint8 event_pending(struct event_des* p_des);
//add event to event queue tail
uint8 event_add_tail(uint8 bisr, uint8 type, uint8 param);
//add event to event queue head, which will be processed first
uint8 event_add_head(uint8 bisr, uint8 type, uint8 param);

/**
 * when you have something to be done periodic, register your call back function;
 * when the registered call back function not need run, unregister it....
 * Attention: following function can not be called from isr...
 */
#if (CB_FUNC_ENABLE == YES)
typedef void (*call_back_t)(void);
//init:
void cb_list_init(void);
//register call back function:
void register_call_back(call_back_t cb) large;
//un-register call back function:
void unregister_call_back(call_back_t cb) large;
//calling call-back...
void traverse_call_back_list(void) large;
#endif

#endif
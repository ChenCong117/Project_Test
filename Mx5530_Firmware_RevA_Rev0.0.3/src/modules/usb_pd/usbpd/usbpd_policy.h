#ifndef _USBPD_POLICY_H
#define _USBPD_POLICY_H

#include "includes.h"
#include "../usbpd_common.h"

// variables & status: 
// power role... 
extern bit current_power_role; 
// data role... 
extern bit current_data_role; 
// vconn role... 
extern bit current_vconn_role; 
// current version...
extern uint8 xdata current_pd_version;
// connect status: 
extern uint8 xdata current_connect_status; 

// init routine:
void usbpd_policy_init(void);

// reset routine:
void usbpd_policy_reset(void); 

// process usb pd messages:
void usbpd_message_process(void);

// process pe timers:
void usbpd_timer_process(uint8 index, uint8 bits); 

// process event which need to do something: 
void usbpd_event_process(uint16 event_bits); 

// state transition:
void usbpd_state_transition(uint8 new_state); 

// usb pd general timers: 3 ms timers + 3 us timers...
// ms timer: 
void usbpd_ms_timer_start(uint8 index, uint16 count); 
uint8 usbpd_ms_timer_is_timeout(uint8 index); 
// us timer: 
void usbpd_us_timer_start(uint8 index, uint16 count); 
uint8 usbpd_us_timer_is_timeout(uint8 index); 

// hardware delay...
void usbpd_hw_delay_ms(uint16 ms); 
void usbpd_hw_delay_us(uint16 us); 

#endif
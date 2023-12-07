#ifndef _USB_PD_H
#define _USB_PD_H

#include "includes.h"
#include "../usbpd_common.h"

#if (FMOD_USB_PD_OPEN == YES)

// init routine:
void usb_pd_init(void);

// process:
void usb_pd_process(uint8 param);

// isr:
void usb_pd_isr1(void);

// event register:
void usb_pd_add_event(uint8 bisr, uint16 evt_type); 

// disconnect process, just for test...
void usb_pd_check_connect(uint8 bisr); 

// test function:
#define USB_PD_TEST_EN		0
#if (USB_PD_TEST_EN == YES)
void usb_pd_test(void);
#endif


#endif

#endif
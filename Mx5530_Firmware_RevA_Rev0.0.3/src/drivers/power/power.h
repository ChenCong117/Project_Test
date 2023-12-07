#ifndef _POWER_H
#define _POWER_H

#include "includes.h"

// power module init:
void power_init(void);

// set voltage by dac...
void power_set_voltage(uint16 voltage, uint8 pps_request); 

// set current by dac...
void power_set_current(uint16 current, uint8 pps_request);

// get current voltage...
//uint16 power_read_voltage(void);

// get current current...
//uint16 power_read_current(void);
//uint8 power_read_current(void);

// get temperature in Degree Celsius...(0~170 -> -20~150)
//uint8 power_read_temperature(uint8 is_external);


#endif 
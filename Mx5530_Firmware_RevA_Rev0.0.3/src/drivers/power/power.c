
#include "power.h"


// power module init:
void power_init(void)
{
	// enable dac control: 
	//regmap10(0x04) = 0x83; // dac_fw_en = 1, dac_current_en = 1, dac_voltage_en = 1
	// enable adc control:
	//regmap10(0x07) = 0x01; // adc_en = 1
	//regmap10(0x06) = 0x00; // reset all...
}

//////////////////////////////////////////// DAC //////////////////////////////////////////////////

// set voltage by dac...
// 12-bit dac: 0x000 -> 0V, 0xFFF -> 2.5V 
// range: 0.3~2.2V -> 3~22V, so the formular is: 0xFFF * voltage / 25
// voltage_mV: 0~20V -> 0~20000mV
/*
void power_set_voltage(uint16 voltage_mV)
{	 
	uint32 value; 
	// calcualte: 
	value = ((uint32)voltage_mV << 12) / 25000; // "*0xFFF" -> "<<12"
	// take action...
	regmap10(0x01) = (value >> 0) & 0xFF; 
	regmap10(0x02) = (value >> 8) & 0x0F; 
}
*/

// set current by dac...
// 8-bit dac: 0x00 -> 0V, 0xFF -> 2.5V 
// range: 0.4~2.4V -> 0~5A, so the formular is: (current / 5) * (2.4 - 0.4) + 0.4
// current_mA: 0~5A -> 0~5000mA 
/*
void power_set_current(uint16 current_mA)
{
	uint32 vol_t; 
	uint32 value;
	// calculate: 
	vol_t = (uint32)current_mA * 4 + 4000; 	// in 0.1mV unit...
	value = 0xFF * vol_t / 25000;
	// take action...
	regmap10(0x03) = value & 0xFF;
} 
*/

void power_set_voltage(uint16 voltage, uint8 pps_request)
{
	if(pps_request)
	{		
		// pps unit: 20mV 
		regmap0D(0x4A) = (voltage >> 8) & 0x07; // vout_pps[10:0] - 0D4A[2:0] - 0D4B[7:0]
		regmap0D(0x4B) = (voltage >> 0) & 0xFF;
		// control: 
		regmap0D(0x48) |=  (1 << 5); // pps_request = 1
		regmap0D(0x4A) |=  (1 << 7); // vout_pd_pps_rdy = 0 - > 1 
		regmap0D(0x4A) &= ~(1 << 7);
	}
	else
	{
		// fix: unit: 50mV
		regmap0D(0x49) = (voltage >> 0) & 0x7F;
		// control: 
		regmap0D(0x48) &= ~(1 << 5); // pps_request = 0
		regmap0D(0x49) |=  (1 << 7); // vout_pd_rdy = 0 -> 1
		regmap0D(0x49) &= ~(1 << 7);
	}
} 

void power_set_current(uint16 current, uint8 pps_request)
{
	if(pps_request)
	{
		// pps: unit 50mA
		regmap0D(0x4C) = (current >> 0) & 0x7F; 
	}
	else
	{
		// fix: unit 10mA
		regmap0D(0x4D) = (current >> 8) & 0x03; 	
		regmap0D(0x4E) = (current >> 0) & 0xFF; 	
	}
}

//////////////////////////////////////////////// ADC //////////////////////////////////////////////

#define ADC_SEL_VOLTAGE			0
#define ADC_SEL_CURRENT			1
#define ADC_SEL_TEMPINT			2
#define ADC_SEL_TEMPEXT			3
/*
static uint16 _read_adc_value(uint8 sel)
{
	uint16 value; 
	// select target:
	regmap10(0x07) &= 0xF1; // bit[3:1]
	regmap10(0x07) |= (sel & 7) << 1; 	
	// wait adc sample ready:
	while((regmap11(0x02) & (1 << 4)) == 0)
	{
	}
	// ok:
	value = regmap11(0x02) & 0x0F; 
	value = (value << 4) | regmap11(0x01); 
	return value; 
}
*/

// get current voltage...
// 12-bit dac: 0x000 -> 0V, 0xFFF -> 2.7V 
// range: 0.3~2.2V -> 3~22V, so the formular is: voltage = value * 27000 / 0xFFF (mV)
// voltage_mV: 0~20V -> 0~20000mV
/*
uint16 power_read_voltage(void)
{
	uint16 value;
	uint16 vol_t; 
	// read adc:
	value = _read_adc_value(ADC_SEL_VOLTAGE); 
	// calculate: 
	vol_t = ((uint32)value * 27000) >> 12; //  "/0xFFF" -> ">>12"
	// in mV... 
	return vol_t;	
}
*/
/*
uint16 power_read_voltage(void)
{
	uint16 value; 
	value = regmap11(0x05) & 0x07; 
	value = (value << 8) | regmap11(0x04);
	return value; 
}
*/
// get current current...
/*
uint16 power_read_current(void)
{
	uint16 value;
	uint16 vol_t; 
	uint16 cur_t; 
	// read adc:
	value = _read_adc_value(ADC_SEL_CURRENT); 
	// 1. calculate voltage:
	vol_t = ((uint32)value * 27000) >> 12; // "/0xFFF" -> ">>12"
	// 2. calculate current: current = (voltage - 4000) / 4000; 
	cur_t = (vol_t - 4000) / 4000; 
	// in mA...
	return cur_t; 
}
*/
/*
uint8 power_read_current(void)
{
	return regmap11(0x06); 
}
*/
// get extern temperature...
// range: 0~170 -> -20~150
/*
uint8 power_read_temperature(uint8 is_external)
{
	uint16 value;
	uint16 vol_t; 
	uint16 tem_t; 
	// read adc:
	value = _read_adc_value(is_external ? ADC_SEL_TEMPEXT : ADC_SEL_TEMPINT); 
	// 1. calculate voltage:
	vol_t = ((uint32)value * 27000) >> 12; // "/0xFFF" -> ">>12"	
	// 2. calculate temperature: temperature = (voltage - 600) / 10; 
	tem_t = (vol_t - 600) / 10; 
	// in range of (0 - 1700)...
	return tem_t & 0xFF; 
}
*/
/*
uint8 power_read_temperature(uint8 is_external)
{
	uint8 value; 
	if(is_external)
	{
		value = regmap11(0x03);
	}
	else
	{
		value = regmap11(0x02);
	}
	return value; 
}
*/

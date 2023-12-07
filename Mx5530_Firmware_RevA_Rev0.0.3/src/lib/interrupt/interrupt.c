
#include "interrupt.h"
#include "sw_iic/sw_iic.h"
#include "usb_pd/usbpd/usb_pd.h"

#if (IMOD_INTERRUPT_OPEN == YES)

#define INT_ENABLE()		do{EA=1;}while(0)
#define INT_DISABLE()		do{EA=0;}while(0)

// register map:
static uint8 xdata reg_mcu_int0_raw_status	_at_	(INTCTRL_REG_BASE + 0x05);
static uint8 xdata reg_mcu_int1_raw_status	_at_	(INTCTRL_REG_BASE + 0x06);
static uint8 xdata reg_mcu_int0_status		_at_	(INTCTRL_REG_BASE + 0x07);
static uint8 xdata reg_mcu_int1_status		_at_	(INTCTRL_REG_BASE + 0x08);
static uint8 xdata reg_hif_int0_raw_status	_at_	(INTCTRL_REG_BASE + 0x09);
static uint8 xdata reg_hif_int1_raw_status	_at_	(INTCTRL_REG_BASE + 0x0A);
static uint8 xdata reg_hif_int0_status		_at_	(INTCTRL_REG_BASE + 0x0B);
static uint8 xdata reg_hif_int1_status		_at_	(INTCTRL_REG_BASE + 0x0C);

static uint8 xdata reg_mcu_int0_mask		_at_ 	(INTCTRL_REG_BASE + 0x10);
static uint8 xdata reg_mcu_int1_mask		_at_	(INTCTRL_REG_BASE + 0x11);	
static uint8 xdata reg_hif_int0_mask		_at_	(INTCTRL_REG_BASE + 0x12);
static uint8 xdata reg_hif_int1_mask		_at_	(INTCTRL_REG_BASE + 0x13);
	 
void interrupt_init(void)
{
	// MCU native interrupt control:
	EA = 0;
	ES = 0;
	// open target interrupt:
	EX0 = 1;
	EX1 = 1;
	ET0 = 1;
	ET1 = 0;	
	
	// CT391 interrupt control: 1->masked, 0->open
	reg_mcu_int0_mask = 0xFF;  
	reg_mcu_int1_mask = 0xFF; 
	reg_mcu_int0_mask &= ~(EX0_HOSTIF_INT0_ACT); // open hostif int0, always opened...
	#if (FMOD_USB_PD_OPEN == YES)
	reg_mcu_int1_mask &= ~(EX1_USB_TYPEC_ACT); // open usb type-c interrupt...
	#endif

	// hostif interrupt control:
	reg_hif_int0_mask = 0xFF;
	reg_hif_int1_mask = 0xFF;
	#if (DRV_SW_IIC_OPEN == YES)
	//reg_hif_int0_mask &= ~(HIF_INT0_IICSW_OP_END);	// open sw_iic - we don't use interrupt but polling...
	#endif
	
	// key gate:
	EA = 1;  	
}

/**
 * external interupt 0 isr:
 */
void ex0_isr(void) interrupt INT_NUM_0_EX0
{
	uint8 hif_status = 0;
	uint8 status = reg_mcu_int0_status;

	// dispatch routines:

	// hostif need distinguish to details:
	if(status & EX0_HOSTIF_INT0_ACT)
	{
		hif_status = reg_hif_int0_status;
		// software iic:
		#if (DRV_SW_IIC_OPEN == YES)
		if(hif_status & HIF_INT0_IICSW_OP_END)
			iic_isr();	
		#endif

		// clear:
		reg_hif_int0_status = hif_status;
	}
	
	// clear interrupt: will be done in each module isr, so NOT do it here...
}


/**
 * external interrupt 1 isr:
 */
void ex1_isr(void) interrupt INT_NUM_2_EX1
{
	uint8 status = reg_mcu_int1_status;

	// usb_pd:
	#if (FMOD_USB_PD_OPEN == YES)
	if(status & EX1_USB_TYPEC_ACT)
		usb_pd_isr1();
	#endif

	// clear interrupt: will be done in each module isr, so NOT do it here...
}

#else // #if (IMOD_INTERRUPT_OPEN == YES)

void ex0_isr(void) interrupt INT_NUM_0_EX0
{
	// do nothing...
}

void ex1_isr(void) interrupt INT_NUM_2_EX1
{
	// do nothing...
}

#endif 
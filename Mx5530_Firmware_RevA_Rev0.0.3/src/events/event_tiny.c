						   
#include "event.h"

#if (IMOD_EVENT_OPEN == YES)

#if (EVENT_LEVEL_SET == EVENT_TINY)

// event descriptor:
static uint8 data event_params[EVT_TYPE_NUM]; 

/**
 * interrupt mutex:
 */
#define INT_ENABLE()		do{EA=1;}while(0)
#define INT_DISABLE()		do{EA=0;}while(0)


void event_initialize(void) 
{
	uint8 data i;

	for(i=0; i<EVT_TYPE_NUM; ++i)
		event_params[i] = 0x00;

	#if (CB_FUNC_ENABLE == YES)
   	cb_list_init();
	#endif
}

uint8 event_pending(struct event_des* p_des) 
{
	uint8 data i;
		
	// find if there events:
	for(i=0; i<EVT_TYPE_NUM; ++i)
	{
		if(event_params[i])
			break;
	}
	if(i >= EVT_TYPE_NUM)
		return 0;

	// find it:
	INT_DISABLE();
	p_des->type = i;
	p_des->param = event_params[i];
	// reset paramters:
	event_params[i] = 0x00;
	INT_ENABLE();

	return 1;
}

/**
 * add event to evnet queue tail...
 */
uint8 event_add_tail(uint8 bisr, uint8 type, uint8 param) 
{
	//parameter check:
	if(type >= EVT_TYPE_NUM)
		return 0;

	//prevent from interrupt by isr...
	if(!bisr) 
		INT_DISABLE();

	//add event whatever it exist or not:
	event_params[type] |= param;

	if(!bisr) 
		INT_ENABLE();

	return 1;	
}


/**
 * add event to event queue head:
 */
uint8 event_add_head(uint8 bisr, uint8 type, uint8 param) 
{
	// for tidy version, _head is same with _tail...
	return event_add_tail(bisr, type, param);	
}

#endif

#endif
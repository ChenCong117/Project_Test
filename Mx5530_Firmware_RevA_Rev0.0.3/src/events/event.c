
#include "event.h"

#if (IMOD_EVENT_OPEN == YES)


#if (EVENT_LEVEL_SET == EVENT_FULL)

// for check error:
extern uint8 xdata g_evt_des_not_enough; // defined in sched.c

/**
 * event linked
 */
#define EVENT_DES_NUM			16		  
static struct event_des xdata event_des_pool[EVENT_DES_NUM];
static struct event_des xdata event_head_t;
static struct event_des xdata * event_head;
static uint8 find_free_des(void);
static void event_des_init(void);

/**
 * interrupt mutex:
 */
#define INT_ENABLE()		do{EA=1;}while(0)
#define INT_DISABLE()		do{EA=0;}while(0)


void event_initialize(void) 
{
	g_evt_des_not_enough = 0;

 	event_des_init();

	#if (CB_FUNC_ENABLE == YES)
   	cb_list_init();
	#endif
}

uint8 event_pending(struct event_des* p_des) 
{
	struct event_des xdata * p;

	if(!event_head->next)
		return 0;

	//remove first node: thinking about priority???
	INT_DISABLE();
	p = event_head->next;
	event_head->next = p->next;
	INT_ENABLE();
	
	//fill data:
	p_des->type = p->type;
	p_des->param = p->param;

	//return des to pool:
	p->next = null;
	p->type = EVT_UN_DEF;
	p->param = 0;

	return 1;
}

/**
 * add event to evnet queue tail...
 */
uint8 event_add_tail(uint8 bisr, uint8 type, uint8 param) 
{
	uint8_t i;
	struct event_des xdata * p;
	struct event_des xdata * q;

	//prevent from interrupt by isr...
	if(!bisr) 
		INT_DISABLE();

	//check if there is same event already:
	p = event_head;
	while(p->next)
	{
		p = p->next;

		//check if same event pending already...
		if(p->type == type && p->param == param)
		{
			if(!bisr) 
				INT_ENABLE();
			return 0;
		}
	}

	//constuct des:
	i = find_free_des();
	if(i == EVT_UN_DEF)
	{
		//indicate the error...
		g_evt_des_not_enough++;
		if(!bisr)
			INT_ENABLE();
		return 0; // ignore event as no des space...
	}
	q = &event_des_pool[i];
	q->next = null;
	q->type = type;
	q->param = param;

	//add event to link:
	p->next = q;
	
	if(!bisr) 
		INT_ENABLE();
	return 1;	
}


/**
 * add event to event queue head:
 */
uint8 event_add_head(uint8 bisr, uint8 type, uint8 param) 
{
	uint8 i;
	struct event_des xdata * p;
	struct event_des xdata * q;

	//prevent from interrupt by isr...
	if(!bisr)
		INT_DISABLE();

	//check if there is same event already:
	p = event_head;
	while(p->next)
	{
		p = p->next;

		//check if same event pending already...
		if(p->type == type && p->param == param)
		{
			if(!bisr)
				INT_ENABLE();
			return 0;
		}
	}

	//constuct des:
	i = find_free_des();
	if(i == EVT_UN_DEF)
	{
		g_evt_des_not_enough++;
		if(!bisr)
			INT_ENABLE();
		return 0; // ignore event as no des space...
	}
	q = &event_des_pool[i];
	q->next = null;
	q->type = type;
	q->param = param;

	//add event to link:
	p = event_head->next;
	event_head->next = q;
	q->next = p;
	
	if(!bisr)
		INT_ENABLE();
	return 1;	
}


/***************************** static functions ****************************/

uint8 find_free_des(void)
{
	uint8 i;
	for(i=0; i<EVENT_DES_NUM; ++i)
	{								
		if(event_des_pool[i].type == EVT_UN_DEF)
		{
			event_des_pool[i].type = EVT_UN_DEF - 1; // indicate that this is unuseful...
			return i;
		}
	}
	return EVT_UN_DEF;
}

void event_des_init(void)
{
	uint8_t i;

	//des pool init:
	for(i=0; i<EVENT_DES_NUM; ++i)
	{
		event_des_pool[i].next = null;
		event_des_pool[i].type = EVT_UN_DEF;
		event_des_pool[i].param = 0;
	}

	//header init:
	event_head_t.next = null;
	event_head_t.type = EVT_UN_DEF;
	event_head_t.param = 0;
	event_head = &event_head_t; // for easy use...
}


#endif

#endif
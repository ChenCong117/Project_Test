
#include "event.h"

#if (IMOD_EVENT_OPEN == YES)

#if (CB_FUNC_ENABLE == YES)

/**
 * call back function list:
 */
#define CB_NODE_NUM				5	  	// now max support 5 call back...
#define CB_NODE_MAGIC			0x6A
struct cb_node{
	struct cb_node* next;
	uint8_t magic_num;
	call_back_t fn;
};
static struct cb_node xdata cb_node_pool[CB_NODE_NUM];
static struct cb_node xdata cb_list_head_t;
static struct cb_node xdata * cb_list_head;
static uint8 find_free_node(void);
static void cb_list_init(void);

//register call back function:
void register_call_back(call_back_t cb) large
{
	uint8 index;
	struct cb_node xdata *p;

	index = find_free_node();
	if(index >= CB_NODE_NUM)
	{
		return;
	}

	p = cb_list_head;
	while(p->next)
	{
		p = p->next;
		//check if resitered already...
		if(p->fn == cb)
			return;
	}
	p->next = &cb_node_pool[index];
	p->next->next = null;
	p->next->fn = cb;
}

//un-register call back function:
void unregister_call_back(call_back_t cb) large
{
	struct cb_node xdata *p, *q;

	p = cb_list_head;
	while(p->next)
	{
		q = p;
		p = p->next;
		if(cb == p->fn)
		{
			q->next = p->next;	//remove from list...
			p->magic_num = 0;
			return;
		}
	}
}

void traverse_call_back_list(void) large
{
	struct cb_node xdata *p;

	p = cb_list_head;
	while(p->next)
	{
		p = p->next;
		if(p->magic_num == CB_NODE_MAGIC)	 
			p->fn(); // call function...
	}
}

void cb_list_init(void)
{
 	uint8 i;

	//des pool init:
	for(i=0; i<CB_NODE_NUM; ++i)
	{
		cb_node_pool[i].next = null;
		cb_node_pool[i].magic_num = 0;
		cb_node_pool[i].fn = null;
	}

	//header init:
	cb_list_head_t.next = null;
	cb_list_head = &cb_list_head_t;
}

///////////////////////////////////////////////////////////////////////

uint8 find_free_node(void)
{
	uint8 i;
	for(i=0; i<CB_NODE_NUM; ++i)
	{
		if(cb_node_pool[i].magic_num != CB_NODE_MAGIC)
		{
			cb_node_pool[i].magic_num = CB_NODE_MAGIC;
			return i;
		}
	}
	return CB_NODE_NUM;
}





#endif

#endif


//---------------------------------------------------------------------------
// Chrontel Inc. Company Confidential Strictly Private
//
// $Archive: software time delay driver interface$
// $Revision: 1.0 $
// $Author: zhijian $
// $Date: 2013/02/07 $
//
// --------------------------------------------------------------------------
// >>>>>>>>>>>>>>>>>>>>>>>>> COPYRIGHT NOTICE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// --------------------------------------------------------------------------
// Copyright 2013(c) Chrontel Inc.
// This is an unpublished work.
// --------------------------------------------------------------------------

#ifndef __DELAY_H__
#define __DELAY_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "includes.h"

#if (AMOD_SW_DELAY_OPEN == YES)

#define SW_DELAY_1US_EN   	NO
 
#define SW_DELAY_2US_EN 	NO

#define SW_DELAY_10US_EN  	YES

#define SW_DELAY_100US_EN 	YES

#define SW_DELAY_1MS_EN 	YES

#define SW_DELAY_10MS_EN  	YES 

#define SW_DELAY_100MS_EN 	YES

/*1.
**Software delay 1us. (受限于系统时钟频率,大约只能精确到0.96us)
*/
void sw_delay_1us(void);
/*2.
**Software delay 2us. 
*/
void sw_delay_2us(void);
/*3.
**Software delay 10us.
*/
void sw_delay_10us(void);
/*4.
**Software delay 100us.
*/
void sw_delay_100us(void);
/*5.
**Software delay 1ms.
*/
void sw_delay_1ms(void);
/*6.
**Software delay 10ms.
*/
void sw_delay_10ms(void);
/*7.
**Software delay 100ms.
*/
void sw_delay_100ms(void);

#endif // #if (AMOD_SW_DELAY == YES)

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H__ */


#include "sw_delay.h"

/*1-3
**us延时原理: LCALL(2c) + Mov R7, #DELAY_xUS(1c) + DJNZ R7,ret(DELAY_xUS*2c) + RET(2c), 1c = 1/13.5 us
**限制: DELAY_xUS shall be smaller than 255
**整个过程是参考对应的汇编代码的分析得出的, 必须这么写才能得到最短,最清晰的指令时序 
*/

#if (SW_DELAY_1US_EN == YES)
/*1. drv_sw_delay_1us: software delay 1us (受限于系统时钟频率,大约只能精确到0.96us) 
@No param
@No return
*/
void sw_delay_1us(void)
{
	uint8 r = 4;

	while (--r);	
}
#endif 
#if (SW_DELAY_2US_EN == YES)
/*2. drv_sw_delay_2us: software delay 2us 
@No param
@No return
*/
void sw_delay_2us(void)
{
	uint8 r = 11;
	
	while (--r);	
}
#endif
#if (SW_DELAY_10US_EN == YES)
/*3. drv_sw_delay_10us: software delay 10us 
@No param
@No return
*/
void sw_delay_10us(void)
{
	uint8 r = 65;
	
	while (--r);	
}
#endif
#if (SW_DELAY_100US_EN == YES)	
/*4. drv_sw_delay_100us: software delay 100us 
@No param
@No return
@原理: LCALL(2c) + MOV(1c) + 20*(32*2c+3c)+ 5*NOP(5c) + RET(2c) = 1350c, 1c = 1/13.5 us (可以有别的公式, 我只想到这个, 可能不是最优化的)
*/
void sw_delay_100us(void)
{
	uint8 r1, r2;	

	for (r1=20; r1>0; --r1)
		for (r2=32; r2>0; --r2)
			;

	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
}
#endif
#if (SW_DELAY_1MS_EN == YES)
/*5. drv_sw_delay_1ms: software delay 1ms 
@No param
@No return
@原理: LCALL(2c) + MOV(1c) + 78*(85*2c+3c)+ 1*NOP(1c) + RET(2c) = 13500c, 1c = 1/13.5 us (可以有别的公式, 我只想到这个, 可能不是最优化的)
*/
void sw_delay_1ms(void)
{
	uint8_t r1, r2;
	
	for (r1=78; r1>0; --r1)
		for (r2=85; r2>0; --r2)
			;

	_nop_();
}
#endif 
#if (SW_DELAY_10MS_EN == YES)
/*6. drv_sw_delay_10ms: software delay 10ms 
@No param
@No return
@原理: LCALL(2c) + MOV(1c) + 4*((197*2c+3c)*85+3c)+ 3*NOP(1c) + RET(2c) = 135000c, 1c = 1/13.5 us (可以有别的公式, 我只想到这个, 可能不是最优化的)
*/
void sw_delay_10ms(void)
{
	uint8_t r1, r2, r3;
	
	for (r1=4; r1>0; --r1)
		for (r2=85; r2>0; --r2)
			for (r3=197; r3>0; --r3)
				;

	_nop_();
	_nop_();
	_nop_();
}
#endif
#if (SW_DELAY_100MS_EN == YES)
/*7. drv_sw_delay_100ms: software delay 100ms 
@No param
@No return
@原理: LCALL(2c) + MOV(1c) + 14*((236*2c+3c)*203+3c)+ 3*NOP(1c) + RET(2c) = 1350000c, 1c = 1/13.5 us (可以有别的公式, 我只想到这个, 可能不是最优化的)
*/
void sw_delay_100ms(void)
{
	uint8_t r1, r2, r3;
	
	for (r1=14; r1>0; --r1)
		for (r2=203; r2>0; --r2)
			for (r3=236; r3>0; --r3)
				;

	_nop_();
	_nop_();
	_nop_();
}
#endif
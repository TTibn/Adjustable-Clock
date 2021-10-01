/*
 * Timer.c

 *
 *  Created on: 6 May 2021
 *      Author: TT
 */

#include <stdio.h>
#include "io.h"
#include "alt_types.h"
#include "system.h"
#include <stdbool.h>

#include "priv/alt_legacy_irq.h"

#define TIMER_STAT_REG 0 // status register address offset
#define TIMER_CTRL_REG 1 // control register address  offset
#define TIMER_PRDL_REG 2 // period reg (lower 16bits) addr offset
#define TIMER_PRDH_REG 3 // period reg (upper 16bits) addr offset


alt_u16 clock_ticks = 0;


void init_timer(alt_u32 timer_base, alt_u32 period)
{
	alt_u16 high,low;

	/* unpack 32-bit timeout period into two 16-bit half words*/
	high = (alt_u16) (period >> 16);
	low = (alt_u16)	(period & 0x0000ffff);

	/* write timeout_period*/
	IOWR(timer_base, TIMER_PRDH_REG, high);
	IOWR(timer_base, TIMER_PRDL_REG, low);

	/* configure timer to start, continuous mode; enable interrupt*/
	IOWR(timer_base, TIMER_CTRL_REG, 0x0007);
}

void timer_0_isr(void* context, alt_u32 id)
{
	//clear timer's status register to (time-out) bit (bit 0) to start the timer over again
	IOWR(TIMER_0_BASE, TIMER_STAT_REG, 0);

	if (clock_ticks==999)
		clock_ticks=0;
	else clock_ticks++;
}


int main(){
	alt_u8 led_pattern = 0; //change = 1; //count = 0;

	IOWR(LED_BASE, 0 , led_pattern);
	init_timer(TIMER_0_BASE, 50000); // init timer with period = 1 msec

	//declare timer_0 isr
	alt_irq_register(TIMER_0_IRQ, NULL, (alt_isr_func)timer_0_isr);

	while(1)
	{

		if (clock_ticks==0)
		{
			if (led_pattern==1)
			{
				led_pattern = 0;
			}
			else if (led_pattern==0)
			{
				led_pattern=1;
			}
			IOWR(LED_BASE, 0 , led_pattern);
		}
	}
}

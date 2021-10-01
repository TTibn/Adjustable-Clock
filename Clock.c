/*
 * Clock.c
 *
 *  Created on: May 16, 2021
 *      Author: Theocharis Theocharidis
 */

#include <stdio.h>
#include "io.h"
#include "alt_types.h"
#include "system.h"
#include <stdbool.h>

#include "priv/alt_legacy_irq.h"

#define TIMER_STAT_REG 0 //status
#define TIMER_CTRL_REG 1 //control
#define TIMER_PRDL_REG 2 //period low
#define TIMER_PRDH_REG 3 //period high

alt_u16 clock_ticks = 0;
//Declaration and initialization of a table of 10 places for the display of numbers 0 to 9 in leds 
alt_u32 matrix[10] = {0b1000000, 0b1111001, 0b0100100, 0b0110000, 0b0011001, 0b0010010, 0b0000010, 0b1111000, 0b0000000, 0b0010000};

int s = 0, secs = 0, mins = 0, hours = 0; // Initialization of global variables for the measurement of seconds, minutes and hours 

void init_timer(alt_u32 timer_base, alt_u32 period)
{
  alt_u16 high, low;

  /* unpack 32-bit timeout period into two 16-bit half words*/
  high = (alt_u16)(period >> 16);
  low = (alt_u16)(period & 0x0000ffff);

  /* write time out period */

  IOWR(timer_base, TIMER_PRDH_REG, high);
  IOWR(timer_base, TIMER_PRDL_REG, low);

  /* configure timer to start, continuous mode; enable interrupt*/

  IOWR(timer_base, TIMER_CTRL_REG, 0x0007);
}

void timer_0_isr(void *context, alt_u32 id)
{
  //clear timer status register to (time-out) bit (bit 0) to start the timer over again

  IOWR(TIMER_0_BASE, TIMER_STAT_REG, 0);

  if (clock_ticks == 999)
  {
    clock_ticks = 0;
    s = s + 1;
    secs = secs + 1;

  }
  else
    clock_ticks++;
}

//main function
int main()
{
  init_timer(TIMER_0_BASE, 49999);

  alt_u32 sw; //Δήλωση μεταβλητής για τα buttons
  alt_u32 base;
  int sign=0;
  bool flag=false;
  int change = 0;

  int secP = 0, minsP1 = 0, hoursP1 = 0, minsP2 = 0, hoursP2 = 0; //Declaration and initialization of variables used as indexes in the matrix table 

  //declare timer_0 isr
  alt_irq_register(TIMER_0_IRQ, NULL, (alt_isr_func)timer_0_isr);

  IOWR(LED_SEC1_BASE, 0, matrix[secP]);  //Write the value of the table in the right PIO representing the seconds 
  IOWR(LED_SEC2_BASE, 0, matrix[secP]);  //Write the value of the table in the left PIO representing the seconds 
  IOWR(LED_MIN1_BASE, 0, matrix[minsP1]);//Write the table value in the right PIO representing the minutes 
  IOWR(LED_MIN2_BASE, 0, matrix[minsP2]);//Write the value of the table on the left PIO representing the minutes 
  IOWR(LED_HOURS1_BASE, 0, matrix[hoursP1]);//Write the value of the table in the right PIO representing the hours 
  IOWR(LED_HOURS2_BASE, 0, matrix[hoursP2]);//Write the value of the table on the left PIO representing the hours 
  while (1)
  {

/*Interrupt Code Block */
	if (clock_ticks == 0)
	{
		IOWR(LED_SEC1_BASE, 0, matrix[s]);

		sw = IORD(SW_IN_BASE, 0); //Check if a button was pressed 

		if (!(sw & 0x01)){//Check if button1 (sw1) is pressed. ON mode (On) or The clock enters set mode 
			flag = true;
		}
	}
/* Code block for mode status and clock setting mode */

	if (flag ==false){ //If the flag is false then the Interrupt is reactivated by activating in the Timer Control register, the Start, Continuous and InterruptOnTimeout (ito) fields 
		IOWR(TIMER_0_BASE, TIMER_CTRL_REG, 0x0007);
	}

	if (flag ==true){ //The watch is in the adjustment mode 
		IOWR(TIMER_0_BASE, TIMER_CTRL_REG, 0x0006);// Disables InterruptOnTimeout (ito) in the Timer Control register, so it does not interrupt 
		if (!(sw & 0x01)) //If the button1 (sw1) is pressed again, the Interrupt is reactivated and the time display on the LEDs of the board continues. 
		{
			flag=false;
		}
		if (!(sw & 0x02))
		{//If the Push button 1 is pressed we move to the right in the successive PIOs for adjustment 
			sign=sign+1;
			if (sign==4){
				sign=0;
			}
		}
		if (!(sw & 0x04))
		{//If the Push button 2 is pressed we move to the left to the successive PIOs for adjustment 
			sign=sign-1;
			if (sign==-1){
				sign=4;
			}
		}
		if (!(sw & 0x08)) // When the Push button 3 is pressed, the seconds or minutes or hours go up depending on the PIOs to be adjusted. 
		{
			IOWR(TIMER_0_BASE, TIMER_CTRL_REG, 0x0005); // Disable continuous mode in the timer control register 
			if (sign==1){
				hoursP1=hoursP1+1;
				//mins=mins+60;
				change=hoursP1;
				base=LED_HOURS1_BASE;
			}
			else if (sign==2){
				minsP1=minsP1+1;
				change=minsP1;
				base=LED_MIN1_BASE;
			}
			else if (sign==3){
				//s = s + 1;
				secP = secP + 1;
				base=LED_SEC1_BASE;
				change=secP;
			}
			IOWR(base, 0, matrix[change]);
		}

		if (!(sw & 0xA))// When the Push button 4 is pressed the seconds or minutes or hours go down depending on the PIOs to be adjusted 
		{ IOWR(TIMER_0_BASE, TIMER_CTRL_REG, 0x0005); // Disable continuous mode in the timer register control 
			if (sign==1){
				//mins=mins-60;
				hoursP1 = hoursP1 - 1;
				change=hoursP1;
				base=LED_HOURS1_BASE;
			}
			else if (sign==2){
				//mins=mins-1;
				minsP1 = minsP1 - 1;
				change=minsP1;
				base=LED_MIN1_BASE;
			}
			else if (sign==3){
				s = s - 1;
				//secP = secP - 1;
				base=LED_SEC1_BASE;
				change=secP;
			}
			IOWR(base, 0, matrix[change]);
		}
	}

	/* Clock Code Block  */
    if (s == 10) //When the tenths of a second become 10, the corresponding variable is reset and the number of seconds increases by one. 
    {
      s = 0;
      secP = secP + 1;
      IOWR(LED_SEC2_BASE, 0, matrix[secP]); // The variable secP is written as a numerator in the positions of the table to display the corresponding value of the table as a digit in the left PIO of seconds 
    }

    if (secs == 60)// When the seconds become 60 then the corresponding variable is reset to increase the following minute index variable 
    {

      minsP1 = minsP1 + 1;
      IOWR(LED_MIN1_BASE, 0, matrix[minsP1]);
      secs = 0;

      IOWR(LED_SEC2_BASE, 0, matrix[0]);
      IOWR(LED_SEC1_BASE, 0, matrix[0]);

      s = 0;
      secP = 0;
      mins = mins + 1;
    }

    if (minsP1 == 10) //If the minute index for the right (No. 1) PIO becomes 10 and therefore exceeds the maximum value 9 that the table can display then: 
    {// Reset this indicator and increase by one (1) the 2 minute index for the left PIO displaying the minutes 
      minsP1 = 0;
      minsP2 = minsP2 + 1;
      IOWR(LED_MIN2_BASE, 0, matrix[minsP2]); // Write (and display) in the left PIO of the minutes the value of the matrix table numbered minP2 
    }

    if (mins == 60)// When the minutes become 60 then the corresponding variable is reset to increase the following hour-variable 
    {
      hoursP1 = hoursP1 + 1;

      IOWR(LED_HOURS1_BASE, 0, matrix[hoursP1]);
      mins = 0;

      IOWR(LED_MIN1_BASE, 0, matrix[0]);
      IOWR(LED_MIN2_BASE, 0, matrix[0]);

      minsP1 = 0;
      minsP2 = 0;
      hours = hours + 1;
    }

    if (hoursP1==10)// When the hour hand index for the right (No. 1) PIO becomes 10 and therefore exceeds the maximum value 9 that the table can display then: 
    {// Reset this indicator and increase by one (1) the 2 hour indicator for the left PIO displaying the hours 
    	hoursP1=0;
    	hoursP2 = hoursP2 + 1;
       IOWR(LED_HOURS2_BASE, 0, matrix[hoursP2]);
    }

    if ((hoursP1==2) && (hoursP2==4)){// Control so that when the hourly variables become 2 and 4 then they are zeroed 
    	hoursP2=0;
    	hoursP1=0;
    }

    if (hours == 24) // When the hour variable becomes 24 then the hour variable is reset and in the corresponding PIOs is written 00 
    {
    	hours = 0;
    	IOWR(LED_HOURS1_BASE, 0, matrix[0]);
    	IOWR(LED_HOURS2_BASE, 0, matrix[0]);
    	hoursP1= 0;
        hoursP2= 0;
    }
  }
}



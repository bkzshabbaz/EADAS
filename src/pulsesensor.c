/*
 * pulsesensor.c
 *
 *  Created on: Nov 16, 2015
 *      Author: Abhimanyu
 */
#include <msp430.h>
#include "pulsesensor.h"

volatile int i=0,beatinterval=0,bpm=0,flagup = 1;
volatile unsigned int result,highp=0,lowp=4096,avp=0,time=0;
char u_str[50];
unsigned int busy = 0;  //TODO: Should this be defined here?

void initialize_timer()
{
	TA1CCTL0 = CCIE | CAP;						// Timer1_A3
	TA1CCR0 = 0x3E80; 							// Count to 16000 , 16000*0.125us = 2ms
	TA1CTL = TASSEL_2 | MC_1 | TAIE;			// Timer MC = 1, TAIE = 1, TASSEL = 2
}

void initialize_adc()
{
	P8DIR &= ~BIT4;								//P8.4 is Pulse-Sensor Input
	P8SEL0 |= BIT4;								//Set P8.4 as ADC A7
	P8SEL1 |= BIT4;

	ADC12CTL0 &= ~ADC12ENC;						// Disable Conversion
	ADC12CTL0 |= ADC12SHT0_5 | ADC12ON; 		// 96 ADC12CLK cycles | ADC Module on
	ADC12CTL1 |= ADC12SHP | ADC12SSEL_3 | ADC12DIV2; //CLK Predivider = 1 | CLK Divider = 1/3 | SMCLK
	ADC12CTL2 |= ADC12RES_0; 					//Res = 12bit
	ADC12CTL3 |= ADC12CSTARTADD_0; 				//Result in ADC12MEM0
	ADC12MCTL0 |= ADC12INCH_7 | ADC12VRSEL_0;   //Input channel select A7
	ADC12CTL0 |= ADC12ENC;						// Enable Conversion
}


// Timer A3 interrupt service routine
#pragma vector=TIMER1_A1_VECTOR
__interrupt void Timer1_A1 (void)
{
	if(i>=750)
	{
		P1OUT ^= BIT0;
		i=0;
	}
			i++;		// Toggle P1.0 using exclusive-OR
	ADC12CTL0 |= ADC12SC;		// Trigger Conversion
	busy = 1;
	while(ADC12CTL1 & ADC12BUSY);	// Trigger a flag do rest of work outside ISR.
	result = ADC12MEM0&0x0FFF;

	if(result>highp)
		highp = result;
	if (result<lowp && result>1500)
		lowp=result;

	avp = (highp+lowp)/2;

	time++;
	if(result>=avp && flagup==1)
		{	flagup=0;
			P9OUT ^= BIT7;
			beatinterval = time;
		}
	if(result<avp && flagup == 0)
		{
		flagup=1;
		time=0;
		P9OUT ^= BIT7;
		}



	bpm = 6000/(2*beatinterval);
	sprintf(u_str,"%d  %d  %d\n\r",bpm,flagup,result); //TODO: remove this and print_uart
	print_uart(u_str);
	switch(TA1IV);				// Read and Clear Interrupt flags
}



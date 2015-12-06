/*
 * interrupts.c
 *
 *  Created on: Nov 17, 2015
 *      Author: sammy
 */

/**
 * Button 2 press ISR
 */
#include <msp430.h>
#define MAX_BUFFER 256

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
	extern int alarm_heartrate;
	extern int alarm_fall;
	extern int distress_sent;
	//Reset button detected.  Clear fall flags.
	alarm_heartrate = 0;
	alarm_fall = 0;
	distress_sent = 0;
	P1IFG &= ~BIT2;
	P1OUT &= ~BIT7;
}

volatile int i=0,beatinterval=0,bpm=0,flagup = 1,beat_detect=1;
volatile unsigned int result,highp=0,lowp=4096,avp=0,time=0;
char u_str[50];
extern volatile int ADC_request;
unsigned int adc_flag = 0;
/*
 * Timer A3 interrupt service routine
 */
#pragma vector=TIMER1_A1_VECTOR
__interrupt void Timer1_A1 (void)
{


	if(i>=250)
	{
		//P1OUT |=BIT0;
		adc_flag = 1;

	if(ADC_request==0)
	{
		ADC12CTL0 |= ADC12SC;
		ADC_request=1;
	}
	if(time<65536)
		time++;
	if(i>1250)
	{
		i=250;
		if(beat_detect==1)
			beat_detect=0;

	}
	i++;
	switch(TA1IV);				// Read and Clear Interrupt flags
	}
	if(i<500)
		i++;
	switch(TA1IV);
}

/*
 * ISR for UART
 *
 */
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
extern unsigned char receive_buffer[];
extern unsigned char send_buffer[];
extern unsigned int current_index;
extern unsigned int current_send;
extern unsigned int size;

extern char* command;

  switch(__even_in_range(UCA1IV, USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
    	receive_buffer[current_index] = UCA1RXBUF;
    	if (current_index < (MAX_BUFFER - 1)) {
    		current_index++;
    		size++;
    	} else {
    	    current_index = 0;
    	}
		break;
    case USCI_UART_UCTXIFG:
    	if (send_buffer[current_send] != '\0') {
    		UCA1TXBUF = send_buffer[current_send++];
    	} else {
    		UCA1IE &= ~UCTXIE;
    		current_send = 0;
    	}
		break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
  }
}

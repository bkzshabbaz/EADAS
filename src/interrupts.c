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

/*
 * Timer A3 interrupt service routine
 */
#pragma vector=TIMER1_A1_VECTOR
__interrupt void Timer1_A1 (void)
{
	extern volatile int i,beatinterval,bpm,flagup;
	extern volatile unsigned int result,highp,lowp,avp,time;
	if(i>=750)
	{
		P1OUT ^= BIT0;
		i=0;
	}
			i++;		// Toggle P1.0 using exclusive-OR
	ADC12CTL0 |= ADC12SC;		// Trigger Conversion
	while(ADC12CTL1 & ADC12BUSY);	// TODO : Change code to so that we dont have to keep polling
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
//	sprintf(u_str,"%d  %d  %d\n\r",bpm,flagup,result); //TODO: remove this and print_uart
//	print_uart(u_str);
	switch(TA1IV);				// Read and Clear Interrupt flags
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

extern char* command;

  switch(__even_in_range(UCA1IV, USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
    	receive_buffer[current_index] = UCA1RXBUF;
    	if (current_index < (100 - 1))
    		current_index++;
    	  else
    		  current_index = 0;
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

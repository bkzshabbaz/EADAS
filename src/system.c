/*
 * This File has peripheral Initialization functions and calls
 */
#include <msp430.h>
#include "system.h"
#include "usci.h"
#include "lcd.h"
#include "pulsesensor.h"
/*
 * Initialize clock and peripherals
 */
void system_init(void){

	initialize_clock();
	initialize_spi();
	initialize_uart();
	initialize_timer();
	initialize_adc();
	lcdInit();
}

/*
 * Initialize clock and basic GPIO
 */
void initialize_clock()
{

	P9OUT |= BIT7;
	P9DIR |= BIT7;

	P1DIR |= BIT0 | BIT4; 		//Configure LED and Chip Select for Gyro
	P3DIR |= BIT2;				//Configure Chip Select for XM

	P1SEL1 |= BIT5;             // Configure sclk
	P1SEL1 &= ~ BIT4;
	P3SEL1 &= ~BIT2;

	//Push button
	P1DIR &= ~BIT2;			//Button as input
	P1REN |= BIT2;			//Enable pull up resistor
	P1IE |= BIT2;			//Enable interrupt for button
	P1IFG &= ~BIT2;			//Clear interrupt
	P1IES &= ~BIT2;			//Set edge for interrupt
	P1SEL0 &= ~BIT2;		//Set GPIO
	P1SEL1 &= ~BIT2;

	P2SEL0 |= BIT0 | BIT1;                    // Configure SOMI and MISO
	PJSEL0 |= BIT4 | BIT5;                    // For XT1

	// Disable the GPIO power-on default high-impedance mode to activate
	// previously configured port settings
	PM5CTL0 &= ~LOCKLPM5;

	// XT1 Setup
	CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
	CSCTL1 = DCOFSEL_6;							// Set DCO to 8MHz
	CSCTL1 &= ~DCORSEL;
	CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
	CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
	CSCTL4 &= ~LFXTOFF;
	do
	{
		CSCTL5 &= ~LFXTOFFG;                    // Clear XT1 fault flag
		SFRIFG1 &= ~OFIFG;
	}while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
	CSCTL0_H = 0;                             // Lock CS registers
}


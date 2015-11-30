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
    //P9OUT |= BIT7;
    asm("   BIS.B   #0x80,&0x00000282");
    //P9DIR |= BIT7;
    asm("   BIS.B   #0x80,&0x00000284");

    //P1DIR |= BIT0 | BIT4;         //Configure LED and Chip Select for Gyro
    asm("   BIS.B   #0x11,&0x00000204");

    //P3DIR |= BIT2;              //Configure Chip Select for XM
    asm("   BIS.B   #0x04,&0x00000224");

    //P1SEL1 |= BIT5;             // Configure sclk
    asm("   BIS.B   #0x20,&0x0000020c");
    //P1SEL1 &= ~ BIT4;
    asm("   AND.B   #0x00ef,&0x0000020c");

    //P3SEL1 &= ~BIT2;
    asm("   BIC.B   #0x04,&0x0000022c");
    //Push button
    //P1DIR &= ~BIT2;           //Button as input
    asm("   BIC.B   #0x04,&0x00000204");
    //P1REN |= BIT2;            //Enable pull up resistor
    asm("   BIS.B   #0x04,&0x00000206");
    //P1IE |= BIT2;         //Enable interrupt for button
    asm("   BIS.B   #0x04,&0x0000021a");
    //P1IFG &= ~BIT2;           //Clear interrupt
    asm("   BIC.B   #0x04,&0x0000021c");
    //P1IES &= ~BIT2;           //Set edge for interrupt
    asm("   BIC.B   #0x04,&0x00000218");
    //P1SEL0 &= ~BIT2;      //Set GPIO
    asm("   BIC.B   #0x04,&0x0000020a");
    //P1SEL1 &= ~BIT2;
    asm("   BIC.B   #0x04,&0x0000020c");

    //P2SEL0 |= BIT0 | BIT1;                    // Configure SOMI and MISO
    asm("   BIS.B   #0x03,&0x0000020b");

    //PJSEL0 |= BIT4 | BIT5;                    // For XT1
    asm("   BIS.W   #0x0030,&0x0000032a");

	//keypad initialization
	//P8DIR |= BIT5 + BIT6 + BIT7;  //0xE0 00000265
	asm("   BIS.B   #0xE0,&0x00000265");

	//P8OUT &= ~(BIT5 + BIT6 + BIT7);
	asm("   AND.B   #0x001f,&0x00000263");

	//P9DIR &= ~(BIT0 + BIT1 + BIT5 + BIT6);
	asm("   AND.B   #0x009c,&0x00000284");

	//P9OUT &= ~(BIT0 + BIT1 + BIT5 + BIT6);
	asm("   AND.B   #0x009c,&0x00000282");

	//P9REN |= (BIT0 + BIT1 + BIT5 + BIT6);
	asm("   BIS.B   #0x63,&0x00000286");

	// Disable the GPIO power-on default high-impedance mode to activate
	// previously configured port settings
	//PM5CTL0 &= ~LOCKLPM5;
	asm("   BIC.W   #0x0001,&0x00000130");

	// XT1 Setup
	//CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
	asm("   MOV.B   #0x00a5,&0x0161");

	//CSCTL1 = DCOFSEL_6;							// Set DCO to 8MHz
	asm("   MOV.W   #0x000C,&0x00000162");

	//CSCTL1 &= ~DCORSEL;
	asm("   AND.W   #0xffbf,&0x00000162");

	//CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK; //00000164
	asm("   MOV.W   #0x0033,&0x00000164");

	//CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
	asm("   CLR.W   &0x00000166");

	//CSCTL4 &= ~LFXTOFF;
	asm("   BIC.W   #1,&0x00000168");

	do
	{
		CSCTL5 &= ~LFXTOFFG;                    // Clear XT1 fault flag
		SFRIFG1 &= ~OFIFG;
	}while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
	CSCTL0_H = 0;                             // Lock CS registers
}

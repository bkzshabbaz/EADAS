#include <msp430.h> 
#include "LSM9DS0.h"
/*
 * Some code used from Sparkfun's LSM9DS0 library.
 */
#define LED0	BIT0	//LED 0
#define LED1	BIT7	//LED 1

volatile unsigned char RXData = 0;
volatile unsigned char TXData = 0;
volatile unsigned int sent_data = 0;

void initialize_clock()
{
	P9OUT |= BIT7;
	P9DIR |= BIT7;

	P1DIR |= BIT0 | BIT4; 		//Configure LED and Chip Select for Gyro
	P3DIR |= BIT2;				//Configure Chip Select for XM

	P1SEL1 |= BIT5;                    // Configure sclk
	P1SEL1 &= ~ BIT4;
	P3SEL1 &= ~BIT2;

	P2SEL0 |= BIT0 | BIT1;                    // Configure SOMI and MISO
	PJSEL0 |= BIT4 | BIT5;                    // For XT1

	// Disable the GPIO power-on default high-impedance mode to activate
	// previously configured port settings
	PM5CTL0 &= ~LOCKLPM5;

	// XT1 Setup
	CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
	CSCTL1 = DCOFSEL_0;                       // Set DCO to 1MHz
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

void initialize_spi()
{

	// Configure USCI_A0 for SPI operation
	UCA0CTLW0 = UCSWRST;                      // **Put state machine in reset**
										// 4-pin, 8-bit SPI master
	UCA0CTLW0 |= UCMST | UCSYNC | UCCKPL | UCMSB | UCMODE_1 | UCSTEM;
										// Clock polarity high, MSB
	UCA0CTLW0 |= UCSSEL__ACLK;                // ACLK
	UCA0BR0 = 0x02;                           // /2
	UCA0BR1 = 0;                              //
	UCA0MCTLW = 0;                            // No modulation
	UCA0CTLW0 &= ~UCSWRST;                    // **Initialize USCI state machine**
	UCA0IE |= UCRXIE | UCTXIE;                         // Enable USCI_A0 RX interrupt
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCA0IV, USCI_SPI_UCTXIFG))
  {
    case USCI_NONE: break;
    case USCI_SPI_UCRXIFG:
      RXData = UCA0RXBUF;
      UCA0IFG &= ~UCRXIFG;
      if (RXData == 0xD4)
    	  P9OUT |= LED1;

      break;
    case USCI_SPI_UCTXIFG:
		P1OUT &= ~BIT4;
		P3OUT |= BIT2;
		if (TXData == 0x00)
			UCA0IE &= ~UCTXIE;
		UCA0TXBUF = TXData;                   // Transmit characters
		TXData = 0x00;

		__delay_cycles(1000);
      break;
    default: break;
  }
}

int begin()
{
	enum gyro_scale gScl 	= G_SCALE_245DPS;
	enum accel_scale aScl 	= A_SCALE_2G;
	enum mag_scale mScl 	= M_SCALE_2GS;
	enum gyro_odr gODR 		= G_ODR_95_BW_125;
	enum accel_odr aODR 	= A_ODR_50;
	enum mag_odr mODR 		= M_ODR_50;

	volatile uint16_t whoami = lsm9ds0_begin(gScl, aScl, mScl, gODR, aODR, mODR);

	return (whoami == 0x49D4);
}

int read_gryo()
{
	uint8_t temp[6];

	//Start communication
	//gryo enable
//	P1OUT &= ~BIT4;
//	P3OUT |= BIT2;

	//xm enable
	P1OUT |= BIT4;
	P3OUT &= ~BIT2;

	RXData = 0xFF;
	__delay_cycles(5000);
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF  =   (0xC0 | (OUT_X_L_G & 0x3F));

	int i;
	for (i = 0; i < 6; ++i) {
		__delay_cycles(1000);
		while (!(UCA0IFG & UCTXIFG));
		UCA0TXBUF  =  0x00;

		__delay_cycles(1000);
		while (!(UCA0IFG & UCRXIFG));
		RXData = UCA0RXBUF;
		temp[i] = RXData;
	}
	//End communication
	//disable gyro
//	P1OUT |= BIT4;
//	P3OUT &= ~BIT2;

	//disable xm
	P1OUT &= ~BIT4;
	P3OUT |= BIT2;
	return 0;
}

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    initialize_clock();
    initialize_spi();
    volatile unsigned int i;

    P1OUT |= LED0;
	P1DIR |= LED0; //LED

	P9OUT |= LED1;
	P9DIR |= LED1;

	if (!begin())
		// Unable to initialize

	//initialize_gryo();
	//read_gryo();
	for(;;) {
		P1OUT ^= LED0;				// Toggle P1.0 using exclusive-OR

		i = 100000;                          // SW Delay
		do i--;
		while(i != 0);
	}
	return 0;
}

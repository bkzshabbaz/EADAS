#include <msp430.h> 
#include "LSM9DS0.h"
#include <stdio.h>

/*
 * Some code used from Sparkfun's LSM9DS0 library.
 */
#define LED0	BIT0	//LED 0
#define LED1	BIT7	//LED 1

volatile unsigned char RXData = 0;
volatile unsigned char TXData = 0;
volatile unsigned int sent_data = 0;

extern int16_t gx, gy, gz; // x, y, and z axis readings of the gyroscope
extern int16_t ax, ay, az; // x, y, and z axis readings of the accelerometer
extern int16_t mx, my, mz; // x, y, and z axis readings of the magnetometer

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

void initialize_uart()
{
	 //Configure GPIO
	P3SEL0 |= BIT4 | BIT5;                    // USCI_A1 UART operation using Backchannel
	P3SEL1 &= ~(BIT4 | BIT5);

/*
 *  Baud Rate calculation
 *  1000000/(16*9600) = 6.5104166666666667
 *  Fractional portion = 0.5104166666666667
 *	User's Guide Table 21-4: UCBRSx = 0x04
 *	UCBRFx = int ( (0.5104166666666667)*16) = 8
 */
	// Configure USCI_A1 for UART mode
	UCA1CTLW0 |= UCSWRST;                      // Put eUSCI in reset
	//UCA1CTLW0 &= ~0x0010;
	UCA1CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
	UCA1BR0 = 6; //9600
	UCA1BR1 = 0x00;
	UCA1MCTLW |= UCOS16 | UCBRF_8 | 0x4900;
	UCA1CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
}

void print_uart(unsigned char *character) {
	while (*character != '\0') {
		while (!(UCA1IFG & UCTXIFG));
    	UCA1TXBUF = *character++;
	}
}

void print_uartn(unsigned char *character, unsigned int n) {
	while (n--) {
		while (!(UCA1IFG & UCTXIFG));
    	UCA1TXBUF = *character++;
	}
}

void print_uartc(unsigned char character) {
	while (!(UCA1IFG & UCTXIFG));
	UCA1TXBUF = character;

}

//From: http://users.ece.utexas.edu/~valvano/arm/UART.c
void print_dec_uart(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
	  print_dec_uart(n/10);
    n = n%10;
  }
  print_uartc(n+'0'); /* n is between 0 and 9 */
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

//int putchar(int c) {
//	while (!(UCA1IFG & UCTXIFG));
//		UCA1TXBUF = c;
//}

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

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    initialize_clock();
    initialize_spi();
    initialize_uart();

    volatile unsigned int i;

    P1OUT |= LED0;
	P1DIR |= LED0; //LED

	P9OUT |= LED1;
	P9DIR |= LED1;

	if (!begin())
	{
		printf("Unable to initialize LSM9DS0!\n");
	} else {
		printf("LSM9DS0 initialized!\n");
	}


	for(;;) {
		readGyro();
		readAccel();
		P1OUT ^= LED0;				// Toggle P1.0 using exclusive-OR

		i = 100000;                          // SW Delay
		do i--;
		while(i != 0);
	}
	return 0;
}

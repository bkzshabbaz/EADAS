#include <msp430.h> 

#define SCK 	BIT4    // Serial clock P1.4 SCL
#define CSG		BIT2	//CSG (slave select) P3.2
#define SDOG	BIT7	//SDOG on pin MISO P1.7
#define SDA		BIT6	//SDA   MOSI (SDA) P1.6
#define LED0	BIT0	//LED 0
#define LED1	BIT7	//LED 1

volatile unsigned char RXData = 0;
volatile unsigned char TXData;
void initialize_clock()
{
	// Disable the GPIO power-on default high-impedance mode to activate
	// previously configured port settings
	PM5CTL0 &= ~LOCKLPM5;

	// Startup clock system with max DCO setting ~8MHz
	CSCTL0_H = CSKEY >> 8;                    // Unlock clock registers
	CSCTL1 = DCOFSEL_0;// | DCORSEL;             // Set DCO to 1MHz
	CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
	CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
	CSCTL0_H = 0;                             // Lock CS registers

}

void initialize_uart()
{
	// Configure GPIO
	P2SEL0 |= BIT0 | BIT1;                    // USCI_A0 UART operation
	P2SEL1 &= ~(BIT0 | BIT1);


	// Configure USCI_A0 for UART mode
	UCA0CTLW0 = UCSWRST;                      // Put eUSCI in reset
	UCA0CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
	// Baud Rate calculation
	// 8000000/(16*115200) = 52.083
	// Fractional portion = 0.083
	// User's Guide Table 21-4: UCBRSx = 0x04
	// UCBRFx = int ( (52.083-52)*16) = 1
	UCA0BR0 = 52; //115200 baud rate
	UCA0BR1 = 0x00;
	UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900;
	UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
}

void initialize_spi()
{
	//Start SPI configuration here
	UCB0CTL1 |= UCSWRST;                      // **Put state machine in reset**
	UCB0CTL0 |= UCMST|UCSYNC|UCCKPL|UCMSB;    // 3-pin, 8-bit SPI master, Clock polarity high, MSB
	UCB0CTL1 |= UCSSEL_2;
	UCA0CTLW0 |= UCSSEL__ACLK;

	UCB0BR0 = 0x02;                           // /2
	UCB0BR1 = 0;                              //
	//UCB1MCTL = 0;                             // No modulation

	/* Configure the ports */
	P3OUT &= ~CSG;	//Chip select P3.2

	//P1SEL1 = 0; //Clear first
	P1SEL0 |=  (SDOG | SDA | SCK);  		//Initialize P1.4/6/7 for clock MOSI/MISO
	P1SEL1 &= ~(SDOG | SDA | SCK);

	P1DIR |= SCK;
	P1DIR &= ~SDOG;
	P3DIR |= CSG;
	/* End configuring the ports */
	volatile unsigned int i;
	i = 1000;                          // SW Delay to let everything settle
	do i--;
	while(i != 0);
	UCB0CTL1 &= ~UCSWRST;        			// **Initialize USCI state machine**

	/*enable interrupts*/
	UCB0IE |= (UCRXIE | UCTXIE);

	P3OUT |= CSG;							//reset slave
	P3OUT &= ~CSG;
}

void print_uart(unsigned char *character) {
	while (*character != '\0') {
		while (!(UCA0IFG & UCTXIFG));
    	UCA0TXBUF = *character++;
	}
}

#pragma vector=USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
	volatile unsigned int i;


	switch(__even_in_range(UCB0IV, USCI_SPI_UCTXIFG))
	  {
	    case USCI_NONE: break;
	    case USCI_SPI_UCRXIFG:
			RXData = UCB0RXBUF;
			UCB0IFG &= ~UCRXIFG;
			P1OUT &= ~LED0;
	      	break;
	    case USCI_SPI_UCTXIFG:
			UCA0TXBUF = TXData;                   // Transmit characters
			UCA0IE &= ~UCTXIE;
			P9OUT &= ~LED1;
			// __bic_SR_register_on_exit(LPM0_bits); // Wake up to setup next TX
	      	break;
	  }
}

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	//initialize_uart();
    initialize_clock();

    volatile unsigned int i;

    P1OUT |= LED0;
	P1DIR |= LED0; //LED

	P9OUT |= LED1;
	P9DIR |= LED1;
	initialize_spi();

	//print_uart("Hello world\n\r");
	__bis_SR_register(GIE);     // enable interrupts
	TXData = 0x01;
	UCB0TXBUF = TXData;
	for(;;) {
//		P1OUT ^= BIT0;				// Toggle P1.0 using exclusive-OR

//		i = 10000;                          // SW Delay
//		do i--;
//		while(i != 0);
	}
	return 0;
}

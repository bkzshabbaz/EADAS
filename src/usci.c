#include <msp430.h>
#include "usci.h"

volatile unsigned char RXData = 0;
volatile unsigned char TXData = 0;
volatile unsigned int sent_data = 0;

/*
 * --------------------------------------------UART---------------------------------------------------------------
 */
void initialize_uart()
{
	 //Configure GPIO
	P3SEL0 |= BIT4 | BIT5;                    // USCI_A1 UART operation using Backchannel
	P3SEL1 &= ~(BIT4 | BIT5);

	// Baud Rate calculation
	// 8000000/(16*9600) = 52.083
	// Fractional portion = 0.083
	// User's Guide Table 21-4: UCBRSx = 0x04
	// UCBRFx = int ( (52.083-52)*16) = 1
	UCA1BR0 = 52;                             // 8000000/16/9600
	UCA1BR1 = 0x00;
	UCA1MCTLW |= UCOS16 | UCBRF_1 | 0x4900;
	UCA1CTLW0 |= UCSWRST;                      // Put eUSCI in reset
	UCA1CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
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


/*
 * -------------------------------------------------------------SPI-------------------------------------------------------------------------
 *
 */

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
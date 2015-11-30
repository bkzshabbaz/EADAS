#include <msp430.h>
#include "usci.h"

extern unsigned char buffer[];
extern unsigned char send_buffer[];
extern unsigned int current_index;
extern unsigned int current_send;
/*
 * --------------------------------------------UART---------------------------------------------------------------
 */
void transmit_uart(char* str)
{
	int i = 0;
	while (*str != '\0') {
		send_buffer[i++] = *str++;
	}
	send_buffer[i] = '\0';
	current_index = 0;
    UCA1IE |= UCTXIE;
	UCA1TXBUF = send_buffer[current_send++];
}

void initialize_uart()
{
	 //Configure GPIO
    //P3SEL0 |= BIT4 | BIT5;                    // USCI_A1 UART operation using Backchannel
	asm("    BIS.B   #0x0030,&0x0000022a");
	//P3SEL1 &= ~(BIT4 | BIT5);
	asm("    AND.B   #0x00cf,&0x0000022c");
	// Baud Rate calculation
	// 8000000/(16*19200)
	// http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
	//UCA1BR0 = 26;
	asm("    MOV.B   #0x001a,&0x000005e6");
	//UCA1BR1 = 0x00;
	asm("    CLR.B   &0x000005e7");
	//UCA1MCTLW |= 0xD601;
	asm("    BIS.W   #0xD601,&0x000005e8");
	//UCA1CTLW0 |= UCSWRST;                      // Put eUSCI in reset
	asm("    BIS.W   #0x0001,&0x000005e0");
	//UCA1CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
	asm("    BIS.W   #0x0080,&0x000005e0");
	//UCA1CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
	asm("    BIC.W   #1,&0x000005e0");

	//UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
	asm("    BIS.W   #0x0001,&0x000005fa");
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
	//UCA0CTLW0 = UCSWRST;                      // **Put state machine in reset**
    asm("   MOV.W   #1,&0x000005c0");
										// 4-pin, 8-bit SPI master
	//UCA0CTLW0 |= UCMST | UCSYNC | UCCKPL | UCMSB | UCMODE_1 | UCSTEM;
	asm("   BIS.W   #0x6b02,&0x000005c0");
										// Clock polarity high, MSB
	//UCA0CTLW0 |= UCSSEL__ACLK;                // ACLK
	asm("   BIS.W   #0x0040,&0x000005c0");

	//UCA0MCTLW = 0;                            // No modulation
	asm("   CLR.W   &0x000005c8");

	//UCA0CTLW0 &= ~UCSWRST;                    // **Initialize USCI state machine**
	asm("   BIC.W   #1,&0x000005c0");
}

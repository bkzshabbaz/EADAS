#ifndef _USCI_H_
#define _USCI_H_

#define LED0	BIT0	//LED 0
#define LED1	BIT7	//LED 1

void initialize_uart(void);
void transmit_uart(char* str);
void print_uart(unsigned char *character);
void print_uartn(unsigned char *character, unsigned int n);
void print_uartc(unsigned char character);
void print_dec_uart(unsigned long n);
void initialize_spi(void);

#endif /* _USCI_H_ */

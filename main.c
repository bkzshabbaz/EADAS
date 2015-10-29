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
  unsigned char data_in = 0;
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

int initialize_gryo()
{
	//Start communication
	P1OUT &= ~BIT4;
	P3OUT |= BIT2;
	__delay_cycles(1000);
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF  =   (0x80 | (WHO_AM_I_G & 0x3F));

	__delay_cycles(1000);
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF  =  0x00;

	__delay_cycles(1000);
	while (!(UCA0IFG & UCRXIFG));
	RXData = UCA0RXBUF;

	if (RXData == 0xD4) //Read the WHO AM I FLAG
		P9OUT |= LED1;
	else
		return 1;   //If who am i doesn't return expected, return

	/* CTRL_REG1_G sets output data rate, bandwidth, power-down and enables
	Bits[7:0]: DR1 DR0 BW1 BW0 PD Zen Xen Yen
	DR[1:0] - Output data rate selection
		00=95Hz, 01=190Hz, 10=380Hz, 11=760Hz
	BW[1:0] - Bandwidth selection (sets cutoff frequency)
		 Value depends on ODR. See datasheet table 21.
	PD - Power down enable (0=power down mode, 1=normal or sleep mode)
	Zen, Xen, Yen - Axis enable (o=disabled, 1=enabled)	*/
	__delay_cycles(1000);
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF  	=   (CTRL_REG1_G & 0x3F);
	UCA0TXBUF	=	0x0F;

	/* CTRL_REG2_G sets up the HPF
	Bits[7:0]: 0 0 HPM1 HPM0 HPCF3 HPCF2 HPCF1 HPCF0
	HPM[1:0] - High pass filter mode selection
		00=normal (reset reading HP_RESET_FILTER, 01=ref signal for filtering,
		10=normal, 11=autoreset on interrupt
	HPCF[3:0] - High pass filter cutoff frequency
		Value depends on data rate. See datasheet table 26.
	*/
	__delay_cycles(1000);
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF  	=   (CTRL_REG2_G & 0x3F);
	UCA0TXBUF	=	0x00;

	/* CTRL_REG3_G sets up interrupt and DRDY_G pins
	Bits[7:0]: I1_IINT1 I1_BOOT H_LACTIVE PP_OD I2_DRDY I2_WTM I2_ORUN I2_EMPTY
	I1_INT1 - Interrupt enable on INT_G pin (0=disable, 1=enable)
	I1_BOOT - Boot status available on INT_G (0=disable, 1=enable)
	H_LACTIVE - Interrupt active configuration on INT_G (0:high, 1:low)
	PP_OD - Push-pull/open-drain (0=push-pull, 1=open-drain)
	I2_DRDY - Data ready on DRDY_G (0=disable, 1=enable)
	I2_WTM - FIFO watermark interrupt on DRDY_G (0=disable 1=enable)
	I2_ORUN - FIFO overrun interrupt on DRDY_G (0=disable 1=enable)
	I2_EMPTY - FIFO empty interrupt on DRDY_G (0=disable 1=enable) */
	// Int1 enabled (pp, active low), data read on DRDY_G:
	__delay_cycles(1000);
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF  	=   (CTRL_REG3_G & 0x3F);
	UCA0TXBUF	=	0x88;

	/* CTRL_REG4_G sets the scale, update mode
	Bits[7:0] - BDU BLE FS1 FS0 - ST1 ST0 SIM
	BDU - Block data update (0=continuous, 1=output not updated until read
	BLE - Big/little endian (0=data LSB @ lower address, 1=LSB @ higher add)
	FS[1:0] - Full-scale selection
		00=245dps, 01=500dps, 10=2000dps, 11=2000dps
	ST[1:0] - Self-test enable
		00=disabled, 01=st 0 (x+, y-, z-), 10=undefined, 11=st 1 (x-, y+, z+)
	SIM - SPI serial interface mode select
		0=4 wire, 1=3 wire */
	__delay_cycles(1000);
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF  	=   (CTRL_REG4_G & 0x3F);
	UCA0TXBUF	=	0x00;

	/* CTRL_REG5_G sets up the FIFO, HPF, and INT1
	Bits[7:0] - BOOT FIFO_EN - HPen INT1_Sel1 INT1_Sel0 Out_Sel1 Out_Sel0
	BOOT - Reboot memory content (0=normal, 1=reboot)
	FIFO_EN - FIFO enable (0=disable, 1=enable)
	HPen - HPF enable (0=disable, 1=enable)
	INT1_Sel[1:0] - Int 1 selection configuration
	Out_Sel[1:0] - Out selection configuration */
	__delay_cycles(1000);
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF  	=   (CTRL_REG5_G & 0x3F);
	UCA0TXBUF	=	0x00;

	// The receive buffer will be filled with data, read it.
	__delay_cycles(1000);
	while (!(UCA0IFG & UCRXIFG));
	RXData = UCA0RXBUF;

	//End communication
	P1OUT |= BIT4;
	P3OUT &= ~BIT2;
	return 0;
}

int read_gryo()
{
	uint8_t temp[6];

	//Start communication
	P1OUT &= ~BIT4;
	P3OUT |= BIT2;
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
	P1OUT |= BIT4;
	P3OUT &= ~BIT2;

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

	initialize_gryo();
	read_gryo();
	for(;;) {
		P1OUT ^= LED0;				// Toggle P1.0 using exclusive-OR

		i = 100000;                          // SW Delay
		do i--;
		while(i != 0);
	}
	return 0;
}

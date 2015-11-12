/*

void initialize_timer()
{
	TA1CCTL0 = CCIE + CAP;						// Timer1_A3
	TA1CCR0 = 0x3E80;						// Count to 16000 , 16000*0.125us = 2ms
	TA1CTL = TASSEL_2 + MC_1 + TAIE;		// Timer MC = 1, TAIE = 1, TASSEL = 2
}

void initialize_adc()
{
	P9DIR &= ~BIT2;		//P9.2 is Pulse-Sensor Input
	P9SEL0 |= BIT2;		//Set P9.2 as ADC A10
	P9SEL1 |= BIT2;

	ADC12CTL0 &= ~ADC12ENC;		// Disable Conversion
	ADC12CTL0 = ADC12SHT0_1 + ADC12ON; // 8 ADC12CLK cycles | ADC Module on
	ADC12CTL1 = ADC12SSEL_3; //CLK Predivider = 1 | CLK Divider = 1 | SMCLK
	ADC12CTL2 = ADC12RES_1; //Res = 10bit
	ADC12CTL3 = ADC12CSTARTADD_0; //Result in ADC12MEM0
	ADC12MCTL0 = ADC12INCH_10;//ADC12IER0 = ADC12IE0; // Interrupt when result ready or polling?
	*/

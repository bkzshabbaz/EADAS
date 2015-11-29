#include <msp430.h>
#include "lcd.h"
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include "fona808.h"

#define DEFAULT_PHONE "6463026046"

const uint8_t lcdSeg[][2] = { { 0x00, 0x00 }, //space
		{ 0x00, 0x00 }, //! N/A
		{ 0x00, 0xC0 }, //"
		{ 0x00, 0x00 }, //# N/A
		{ 0x00, 0x00 }, //$ N/A
		{ 0x00, 0x00 }, //% N/A
		{ 0x00, 0x00 }, //& N/A
		{ 0x00, 0x00 }, //' N/A
		{ 0x00, 0x00 }, //( N/A
		{ 0x00, 0x00 }, //) N/A
		{ 0x00, 0x00 }, //* N/A
		{ 0x00, 0x00 }, //+ N/A
		{ 0x00, 0x00 }, //, N/A
		{ 0x03, 0x00 }, //-
		{ 0x00, 0x00 }, //. N/A
		{ 0x00, 0x00 }, /// N/A
		{ 0xFC, 0x28 }, //0
		{ 0x60, 0x20 }, //1
		{ 0xDB, 0x00 }, //2
		{ 0xF3, 0x00 }, //3
		{ 0x67, 0x00 }, // 4
		{ 0xB7, 0x00 }, // 5
		{ 0xBF, 0x00 }, // 6
		{ 0xE4, 0x00 }, // 7
		{ 0xFF, 0x00 }, // 8
		{ 0xF7, 0x00 }, // 9
		{ 0x00, 0x00 }, //: N/A
		{ 0x00, 0x00 }, //; N/A
		{ 0x00, 0x00 }, //< N/A
		{ 0x00, 0x00 }, //= N/A
		{ 0x00, 0x00 }, //> N/A
		{ 0x00, 0x00 }, //? N/A
		{ 0x00, 0x00 }, //@ N/A
		{ 0xEF, 0x00 }, //A
		{ 0xF1, 0x50 }, //B
		{ 0x9C, 0x00 }, //C
		{ 0xF0, 0x50 }, //D
		{ 0x9F, 0x00 }, //E
		{ 0x8F, 0x00 }, //F
		{ 0xBD, 0x00 }, //G
		{ 0x6F, 0x00 }, //H
		{ 0x90, 0x50 }, //I
		{ 0x78, 0x00 }, //J
		{ 0x0E, 0x22 }, //K
		{ 0x1C, 0x00 }, //L
		{ 0x6C, 0xA0 }, //M
		{ 0x6C, 0x82 }, //N
		{ 0xFC, 0x00 }, //O
		{ 0xCF, 0x00 }, //P
		{ 0xFC, 0x02 }, //Q
		{ 0xCF, 0x02 }, //R
		{ 0xB7, 0x00 }, //S
		{ 0x80, 0x50 }, //T
		{ 0x7C, 0x00 }, //U
		{ 0x0C, 0x28 }, //V
		{ 0x6C, 0x0A }, //W
		{ 0x00, 0xAA }, //X
		{ 0x00, 0xB0 }, //Y
		{ 0x90, 0x28 } //Z
};


const uint8_t lcdSymbols[5] = { 0x00, 0x01, 0x02, 0x04, 0x08 };
volatile uint8_t count = 0;

int segment =1;
int counter =0;
static char phnNum[11];

void lcdInit(void) {

	lcdIOInit();
	lcdOff();
	//clear Memory
	LCDCMEMCTL |= LCDCLRM + LCDCLRBM;
	lcdSegOn();


	LCDCCTL0 |= LCDSSEL + LCD4MUX + LCDDIV__1 + LCDPRE_4; // LCD source VLOCLK, Low power signals.
	//LCDCBLKCTL |= LCDBLKMOD_0;
	//LCDCBLKCTL |= LCDBLKDIV__8; 	//Blink segments from LCD blink memory
	//LCDCBLKCTL |= LCDBLKMOD_1;
	LCDCCPCTL |= LCDCPCLKSYNC;  //charge pump clock sync
	LCDCVCTL |= LCDCPEN + VLCD1; //charge pump enabled


	//LCDCMEMCTL |= LCDDISP;
	lcdOn();
	lcdPrint("      ",1,6); // To avoid flickr while starting LCD
}

void lcdPrint(char *c, uint8_t start, uint8_t end) {

	uint8_t arrVal = 0;
	uint8_t loop = 0;
	char d;
	//LCDCMEMCTL |= LCDCLRM;
	//__delay_cycles(3000);

	loop = start;
	while (*c) {
		if (loop > end)
			break;
		d = toupper(*c);
		arrVal = (int) d - 32;

		switch (loop) {

		case 1:
			LCDM10 = lcdSeg[arrVal][0];
			LCDM11 = lcdSeg[arrVal][1];
			LCDCPCTL1 |= LCDS18 + LCDS19 + LCDS20 + LCDS21;
			break;

		case 2:
			LCDM6 = lcdSeg[arrVal][0];
			LCDM7 = lcdSeg[arrVal][1];
			LCDCPCTL0 |= LCDS10 + LCDS11 + LCDS12 + LCDS13;
			break;

		case 3:
			LCDM4 = lcdSeg[arrVal][0];
			LCDM5 = lcdSeg[arrVal][1];
			LCDCPCTL0 |= LCDS6 + LCDS7 + LCDS8 + LCDS9;
			break;

		case 4:
			LCDM19 = lcdSeg[arrVal][0];
			LCDM20 = lcdSeg[arrVal][1];
			LCDCPCTL2 |= LCDS36 + LCDS37 + LCDS38 + LCDS39;
			break;

		case 5:
			LCDM15 = lcdSeg[arrVal][0];
			LCDM16 = lcdSeg[arrVal][1];
			LCDCPCTL1 |= LCDS28 + LCDS29 + LCDS30 + LCDS31;
			break;

		case 6:
			LCDM8 = lcdSeg[arrVal][0];
			LCDM9 = lcdSeg[arrVal][1];
			LCDCPCTL0 |= LCDS14 + LCDS15;
			LCDCPCTL1 |= LCDS16 + LCDS17;
			break;

		default:
			break;

		}
		loop++;
		c++;

	}

}

void lcdBlinkSym(uint8_t symbol) {

	LCDM3 = lcdSymbols[symbol];
	LCDCPCTL0 |= LCDS4 + LCDS5;

}

void lcdIOInit(void) {

	//Initialize GPIO for LCD pins
	//P1DIR &= ~((1 << 4) + (1 << 5));
	//P1OUT &= ~((1 << 4) + (1 << 5));
	//P1REN |= (1 << 4) + (1 << 5);

	/*P2DIR &= ~((1 << 4) + (1 << 5) + (1 << 6) + (1 << 7));
	P2OUT &= ~((1 << 4) + (1 << 5) + (1 << 6) + (1 << 7));
	P2REN |= (1 << 4) + (1 << 5) + (1 << 6) + (1 << 7);
	*/

	/*P3DIR = 0x00;
	P3OUT = 0x00;
	P3REN = 0xFF;
	*/

	P4DIR &= ~((1 << 4) + (1 << 5) + (1 << 6));
	P4OUT &= ~((1 << 4) + (1 << 5) + (1 << 6));
	P4REN |= (1 << 4) + (1 << 5) + (1 << 6);

	P5DIR = 0x00;
	P5OUT = 0x00;
	P5REN = 0xFF;

	P6DIR &= ~(1 << 7);
	P6OUT &= ~(1 << 7);
	P6REN |= (1 << 7);

	P7DIR = 0x00;
	P7OUT = 0x00;
	P7REN = 0xFF;

	P8DIR &= ~((1 << 0) + (1 << 1) + (1 << 2) + (1 << 3));
	P8OUT &= ~((1 << 0) + (1 << 1) + (1 << 2) + (1 << 3));
	P8REN |= (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3);

	P10DIR &= ~((1 << 0) + (1 << 1) + (1 << 2));
	P10OUT &= ~((1 << 0) + (1 << 1) + (1 << 2));
	P10REN |= (1 << 0) + (1 << 1) + (1 << 2);

	//lcdTimerInit();

}

void lcdOff(void) {
	LCDCCTL0 &= ~LCDON; // turnoff LCD module
}
void lcdOn(void) {
	LCDCCTL0 |= LCDON; // turn on LCD module
}
void lcdSegOn(void) {
	LCDCCTL0 |= LCDSON; //turn on LCD segments
}
void lcdSegOff(void) {
	LCDCCTL0 &= ~LCDSON; //turn off LCD segments
}

void incSeg(void){
	segment++;
	counter++;
}

void chkSeg(void){
	if(segment>6){
		LCDCMEMCTL |= LCDCLRM;
		segment=1;
	}
}
void entrPhone(void){

	lcdPrint("Enter",1,6);
	__delay_cycles(10000000);
	LCDCMEMCTL |= LCDCLRM;
	lcdPrint("Your",1,6);
	__delay_cycles(10000000);
	LCDCMEMCTL |= LCDCLRM;
	lcdPrint("Phone",1,6);
	__delay_cycles(10000000);
	LCDCMEMCTL |= LCDCLRM;
	lcdPrint("Number",1,6);
	__delay_cycles(10000000);
	LCDCMEMCTL |= LCDCLRM;
	while(1){
	if(counter<10){
	P8OUT |= BIT5;
			if (P9IN & BIT0) {
				while (P9IN & BIT0);
				chkSeg();
				lcdPrint("1",segment,segment);
				phnNum[counter]='1';
				incSeg();
			} else if (P9IN & BIT1) {
				while (P9IN & BIT1);
				chkSeg();
				lcdPrint("4", segment,segment);
				phnNum[counter]='4';
				incSeg();
			} else if (P9IN & BIT5) {
				while (P9IN & BIT5);
				chkSeg();
				lcdPrint("7", segment, segment);
				phnNum[counter]='7';
				incSeg();
			}
			P8OUT &= ~BIT5;

			P8OUT |= BIT6;
			if (P9IN & BIT0) {
				while(P9IN & BIT0);
				chkSeg();
				lcdPrint("2", segment, segment);
				phnNum[counter]='2';
				incSeg();
			} else if (P9IN & BIT1) {
				while(P9IN & BIT1);
				chkSeg();
				lcdPrint("5", segment, segment);
				phnNum[counter]='5';
				incSeg();
			} else if (P9IN & BIT5) {
				while(P9IN & BIT5);
				chkSeg();
				lcdPrint("8", segment, segment);
				phnNum[counter]='8';
				incSeg();
			} else if (P9IN & BIT6) {
				while(P9IN & BIT6);
				chkSeg();
				lcdPrint("0", segment, segment);
				phnNum[counter]='0';
				incSeg();
			}
			P8OUT &= ~BIT6;

			P8OUT |= BIT7;
			if (P9IN & BIT0) {
				while(P9IN & BIT0);
				chkSeg();
				lcdPrint("3", segment, segment);
				phnNum[counter]='3';
				incSeg();
			} else if (P9IN & BIT1) {
				while(P9IN & BIT1);
				chkSeg();
				lcdPrint("6", segment, segment);
				phnNum[counter]='6';
				incSeg();
			} else if (P9IN & BIT5) {
				while(P9IN & BIT5);
				chkSeg();
				lcdPrint("9", segment, segment);
				phnNum[counter]='9';
				incSeg();
			}else if(P9IN & BIT6){
				while(P9IN & BIT6);
				if(counter==0){
				set_phone_number(DEFAULT_PHONE);
				break;
				}
			}
			P8OUT &= ~BIT7;

	}else{
		P8OUT |= BIT7;
		if(P9IN & BIT6){
			while(P9IN & BIT6);
			set_phone_number(phnNum);
			LCDCMEMCTL |= LCDCLRM;
			break;
		}
		P8OUT &= ~BIT7;

	}
	}
}

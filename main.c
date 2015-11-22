/*	EADAS
 * 	Peripheral Usage
 * 		Pulse Sensor
 * 			- P8.2
 * 			- Timer1_A1
 * 			- ADC Channel A7
 * 			- P9.7 (Will be changing to heart icon on LCD)
 * 		Gyro
 *
 * 		LCD
 *
 * 		GSM Module
 *
 *
 */

#include <msp430.h> 
#include "LSM9DS0.h"
#include <stdio.h>
#include <math.h>
#include "usci.h"
#include "system.h"
#include "lcd.h"
#include "fona808.h"

/*
 * Some code used from Sparkfun's LSM9DS0 library.
 */
#define LED0	BIT0	//LED 0
#define LED1	BIT7	//LED 1

int alarm_fall 		= 0;	//Alarm because a fall was detected
int alarm_heartrate = 0;	//Alarm because a change in heartrate was detected
int distress_sent 	= 0;		//Flag to indicate whether we've sent a distress signal

#ifdef DEBUG
extern int16_t gx, gy, gz; // x, y, and z axis readings of the gyroscope
extern int16_t ax, ay, az; // x, y, and z axis readings of the accelerometer
extern int16_t mx, my, mz; // x, y, and z axis readings of the magnetometer
#endif

extern volatile int bpm;
extern volatile unsigned int result;
int volatile ADC_request=0;
char bpm_str[5];

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
    //Test code to force the FONA to detect the baud rate.

    system_init();

	__bis_SR_register(GIE);
    __no_operation();

    P1OUT &= ~LED0;
	P1DIR |= LED0;

	P9OUT |= LED1;
	P9DIR |= LED1;

	if (!begin())
	{
		printf("Unable to initialize LSM9DS0!\n");
	} else {
		printf("LSM9DS0 initialized!\n");
	}

	initialize_fona();

	set_phone_number("6463026046"); //TODO: This should come from the configuration.

	for(;;) {
		readGyro();

		if(!(ADC12CTL1 & ADC12BUSY) && ADC_request==1)
		{
			result = ADC12MEM0&0x0FFF;
			ADC_request=0;
		}
		sprintf(bpm_str,"%d",bpm);
		if(bpm<100)
		{
			lcdPrint(" ", 4, 4);
			lcdPrint(bpm_str,5,6);
		}
		else
			lcdPrint(bpm_str,4,6);

		if (alarm_fall) {
			if (!distress_sent) {
			    //send_sms("I've fallen and I can't get up\r");
				distress_sent = 1;
			}

			lcdPrint("FAL", 1, 3);
		} else {
			lcdPrint("GUD", 1, 3);
		}

#ifdef DEBUG
		/*******************************************************************
		 * WARNING!!!
		 * TURNING ON PRINTFs WILL CAUSE TIMING ISSUES.
		 * E.G., UART and TIMER WON'T WORK PROPERLY.
		 *******************************************************************/
		printf("G: %.2f", fabs(calcGyro(gx)));
		printf(", ");
		printf("%.2f",fabs(calcGyro(gy)));
		printf(", ");
		printf("%.2f\n",fabs(calcGyro(gz)));

		/*
		 * The accelerometer will read 1g when for an axis that's
		 * vertical.
		 */
		//readAccel();
//		printf("A: %.2f", fabs(calcAccel(ax)));
//		printf(", ");
//		printf("%.2f",fabs(calcAccel(ay)));
//		printf(", ");
//		printf("%.2f\n",fabs(calcAccel(az)));
		//TODO: Update the LCD with status information

#endif
	}
	return 0;
}

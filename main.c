#include <msp430.h> 
#include "SparkFunLSM9DS1.h"
#include <stdio.h>
#include <math.h>
#include "usci.h"
#include "system.h"
#include "lcd.h"

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

unsigned char buffer[100];
unsigned int current_index = 0;

int begin()
{
	lsm9ds1_init();
	volatile uint16_t whoami = lsm9ds1_begin();

	return (whoami == 0x683D);
}

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    system_init();

	UCA1IE |= UCTXIE;		//Turn on TX interrupts and send the command to UART.
    //__bis_SR_register(GIE);
    __no_operation();

    P1OUT &= ~LED0;
	P1DIR |= LED0;

	P9OUT |= LED1;
	P9DIR |= LED1;

	if (!begin())
	{
		printf("Unable to initialize LSM9DS1!\n");
	} else {
		printf("LSM9DS1 initialized!\n");
	}

	for(;;) {
		readGyro();
		if (alarm_fall) {
			lcdPrint("FALL", 0, 1, 4);
		} else {
			lcdPrint("GOOD", 0, 1, 4);
		}

#ifdef DEBUG
//		printf("G: %.2f", fabs(calcGyro(gx)));
//        printf(", ");
//        printf("%.2f",fabs(calcGyro(gy)));
//        printf(", ");
//        printf("%.2f\n",fabs(calcGyro(gz)));

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

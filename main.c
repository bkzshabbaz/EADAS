#include <msp430.h> 
#include "LSM9DS0.h"
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

#ifdef DEBUG
extern int16_t gx, gy, gz; // x, y, and z axis readings of the gyroscope
extern int16_t ax, ay, az; // x, y, and z axis readings of the accelerometer
extern int16_t mx, my, mz; // x, y, and z axis readings of the magnetometer
#endif

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

    P1OUT &= ~LED0;
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
		if (alarm_fall) //TODO: Move this to an ISR based on a GPIO edge
			P1OUT |= LED0;

#ifdef DEBUG
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
		//TODO: use an interrupt to signal alarm.
						// Toggle P1.0 using exclusive-OR
#endif
	}
	return 0;
}

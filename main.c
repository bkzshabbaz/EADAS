/*	EADAS
 * 	Peripheral Usage
 * 		Pulse Sensor
 * 			- P8.4
 * 			- Timer1_A1
 * 			- ADC Channel A7
 * 			- Heart Segment on LCD
 * 		Gyro Connections:
 *
 *      LSM9DS0 ------------- MSP430FR6989
 *      CSG     ------------- P1.4
 *      CSXM    ------------- P3.2
 *      SDOG    ------------- P2.1
 *      SDOXM   ------------- P2.1
 *      SCL     ------------- P1.5
 *      SDA     ------------- P2.0
 *      VDD     ------------- 3v3
 *      GND     ------------- GND
 * 		LCD
 *
 * 		GSM Module
 *      UART - P3.4 P3.5
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


extern volatile int i,beatinterval,bpm,flagup,beat_detect;
extern volatile unsigned int result,highp,lowp,avp,time;
int volatile ADC_request=0;
char bpm_str[5];
#define AVG_NUM 15
static int bpms[AVG_NUM],bpmav=0;
unsigned int bpm_threshold = 0,nobeatcounter=0;
extern unsigned int adc_flag;

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

void send_distress(char *message)
{
    send_sms(message);
    distress_sent = 1;
}

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    system_init();

	__bis_SR_register(GIE);
    __no_operation();

    P1OUT &= ~LED0;
	P1DIR |= LED0;

	P9OUT &= ~LED1;
	P9DIR |= LED1;

	if (!begin())
	{
		//printf("Unable to initialize LSM9DS0!\n");
	} else {
		//printf("LSM9DS0 initialized!\n");
	}

	initialize_fona();

	entrPhone();
	int i=-1,j;
	for(;;) {
		readGyro();
		if (adc_flag) {
			if(!(ADC12CTL1 & ADC12BUSY) && ADC_request==1)
			{
				result = ADC12MEM0&0x0FFF;
				ADC_request=0;

				if(result>highp && result<3500)
					highp = result;
				if (result<lowp && result>1500)
					lowp=result;

				avp = (highp+lowp)/2;

				if(result>=avp && flagup==1)
					{
						flagup=0;
						beatinterval = time;
						time=0;
						lcdBlinkSym(HRT,1);
						beat_detect=1;
					}

				if(result<avp && flagup == 0)
					{
						flagup=1;

						lcdBlinkSym(HRT,0);

					}

				bpm = 60000/(2*beatinterval);

				if(bpm<200){
					i=(i+1)%AVG_NUM;
					bpms[i]=bpm;
				}
					bpmav=0;
					if(beat_detect==1 || nobeatcounter<1000)
					{
						if(beat_detect==1)
							nobeatcounter=0;
						for(j=0;j<15;j++)
							bpmav=bpmav+bpms[j];
						bpmav=bpmav/15;
					}
					if(beat_detect==0)
						nobeatcounter++;

			}





			sprintf(bpm_str,"%d",bpmav);
			if(bpmav<10)
			{
				lcdPrint("  ", 4, 5);
				lcdPrint(bpm_str,6,6);
			}
			else if(bpmav<100)
			{
				lcdPrint(" ", 4, 4);
				lcdPrint(bpm_str,5,6);
			}
			else
				lcdPrint(bpm_str,4,6);

			if(bpmav>130 || bpmav==0)
			{
				bpm_threshold++;

			} else if (bpm_threshold != 0){
				bpm_threshold--;
			}
		}
		if (bpm_threshold > 1000) {
			P1OUT |= BIT0;
			if (!distress_sent) {
				send_distress("Heart attack!");
			}
			lcdPrint("HRT", 1, 3);
		} else if (alarm_fall) {
			if (!distress_sent) {
				send_distress("I've fallen!");
			}

			lcdPrint("FAL", 1, 3);
			P9OUT |= LED1;
		} else {
			lcdPrint("GUD", 1, 3);
			P1OUT &= ~BIT0;
			P9OUT &= ~LED1;
		}

#ifdef DEBUG
		/*******************************************************************
		 * WARNING!!!
		 * TURNING ON PRINTFs WILL CAUSE TIMING ISSUES.
		 * E.G., UART and TIMER WON'T WORK PROPERLY.
		 *******************************************************************/
		/*printf("G: %.2f", fabs(calcGyro(gx)));
		printf(", ");
		printf("%.2f",fabs(calcGyro(gy)));
		printf(", ");
		printf("%.2f\n",fabs(calcGyro(gz)));*/

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

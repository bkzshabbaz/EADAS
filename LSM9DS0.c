/*
 * LSM9DS0.c
 *
 *  Created on: Oct 30, 2015
 *      Author: sammy
 *      Connection:
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
 */

#include "LSM9DS0.h"
#include <stdio.h>

static float gRes, aRes, mRes;
static enum gyro_scale gScale;
static enum accel_scale aScale;
static enum mag_scale mScale;
extern int alarm_fall;

static void enable_chip(enum chip_select cs)
{
	//Start communication
	//gryo enable
	if (cs == GYRO) {
		P1OUT &= ~BIT4;
		P3OUT |= BIT2;
	} else if (cs == ACCEL) {
		//xm enable
		P1OUT |= BIT4;
		P3OUT &= ~BIT2;
	}
}

static void disable_chip(enum chip_select cs)
{
	//End communication
	if (cs == GYRO) {
		P1OUT |= BIT4;
		P3OUT &= ~BIT2;
	} else if (cs == ACCEL) {
		//disable xm
		P1OUT &= ~BIT4;
		P3OUT |= BIT2;
	}
}

void calcgRes()
{
	// Possible gyro scales (and their register bit settings) are:
	// 245 DPS (00), 500 DPS (01), 2000 DPS (10). Here's a bit of an algorithm
	// to calculate DPS/(ADC tick) based on that 2-bit value:
	switch (gScale)
	{
	case G_SCALE_245DPS:
		gRes = 245.0 / 32768.0;
		break;
	case G_SCALE_500DPS:
		gRes = 500.0 / 32768.0;
		break;
	case G_SCALE_2000DPS:
		gRes = 2000.0 / 32768.0;
		break;
	}
}

void calcaRes()
{
	// Possible accelerometer scales (and their register bit settings) are:
	// 2 g (000), 4g (001), 6g (010) 8g (011), 16g (100). Here's a bit of an
	// algorithm to calculate g/(ADC tick) based on that 3-bit value:
	aRes = aScale == A_SCALE_16G ? 16.0 / 32768.0 :
		   (((float) aScale + 1.0) * 2.0) / 32768.0;
}

void calcmRes()
{
	// Possible magnetometer scales (and their register bit settings) are:
	// 2 Gs (00), 4 Gs (01), 8 Gs (10) 12 Gs (11). Here's a bit of an algorithm
	// to calculate Gs/(ADC tick) based on that 2-bit value:
	mRes = mScale == M_SCALE_2GS ? 2.0 / 32768.0 :
	       (float) (mScale << 2) / 32768.0;
}

void readBytes(enum chip_select cs, uint8_t subAddress,
		uint8_t * dest, uint8_t count)
{
	enable_chip(cs);
	uint8_t temp;

	while (!(UCA0IFG & UCTXIFG));
	if (count > 1)
		UCA0TXBUF  	=   (0xC0 | (subAddress & 0x3F));
	else
		UCA0TXBUF  	=   (0x80 | (subAddress & 0x3F));

	while (!(UCA0IFG & UCRXIFG));
	temp = UCA0RXBUF;

	int i;
	for (i=0; i<count; i++)
	{
		while (!(UCA0IFG & UCTXIFG));
		UCA0TXBUF  =  0x00;

		while (!(UCA0IFG & UCRXIFG));
		dest[i] = UCA0RXBUF;
	}
	disable_chip(cs);
}

uint8_t readByte(enum chip_select cs, uint8_t subAddress)
{
	uint8_t temp;
	// Use the multiple read function to read 1 byte.
	// Value is returned to `temp`.
	readBytes(cs, subAddress, &temp, 1);
	return temp;
}



void writeByte(enum chip_select cs, uint8_t subAddress, uint8_t data)
{
	enable_chip(cs);
	uint8_t temp;
	// If write, bit 0 (MSB) should be 0
	// If single write, bit 1 should be 0
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF  	=   (subAddress & 0x3F);

	while (!(UCA0IFG & UCRXIFG));
	temp = UCA0RXBUF;

	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF  		=   data; // Send data

	while (!(UCA0IFG & UCRXIFG));
	temp = UCA0RXBUF;

	disable_chip(cs);
}

void initGyro()
{
	/* CTRL_REG1_G sets output data rate, bandwidth, power-down and enables
	Bits[7:0]: DR1 DR0 BW1 BW0 PD Zen Xen Yen
	DR[1:0] - Output data rate selection
		00=95Hz, 01=190Hz, 10=380Hz, 11=760Hz
	BW[1:0] - Bandwidth selection (sets cutoff frequency)
		 Value depends on ODR. See datasheet table 21.
	PD - Power down enable (0=power down mode, 1=normal or sleep mode)
	Zen, Xen, Yen - Axis enable (o=disabled, 1=enabled)	*/
	writeByte(GYRO, CTRL_REG1_G, 0x0F); // Normal mode, enable all axes

	/* CTRL_REG2_G sets up the HPF
	Bits[7:0]: 0 0 HPM1 HPM0 HPCF3 HPCF2 HPCF1 HPCF0
	HPM[1:0] - High pass filter mode selection
		00=normal (reset reading HP_RESET_FILTER, 01=ref signal for filtering,
		10=normal, 11=autoreset on interrupt
	HPCF[3:0] - High pass filter cutoff frequency
		Value depends on data rate. See datasheet table 26.
	*/
	writeByte(GYRO, CTRL_REG2_G, 0x00); // Normal mode, high cutoff frequency

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
	writeByte(GYRO, CTRL_REG3_G, 0x88);

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
	writeByte(GYRO, CTRL_REG4_G, 0x00); // Set scale to 245 dps

	/* CTRL_REG5_G sets up the FIFO, HPF, and INT1
	Bits[7:0] - BOOT FIFO_EN - HPen INT1_Sel1 INT1_Sel0 Out_Sel1 Out_Sel0
	BOOT - Reboot memory content (0=normal, 1=reboot)
	FIFO_EN - FIFO enable (0=disable, 1=enable)
	HPen - HPF enable (0=disable, 1=enable)
	INT1_Sel[1:0] - Int 1 selection configuration
	Out_Sel[1:0] - Out selection configuration */
	writeByte(GYRO, CTRL_REG5_G, 0x00);
}

void initAccel()
{
	/* CTRL_REG0_XM (0x1F) (Default value: 0x00)
	Bits (7-0): BOOT FIFO_EN WTM_EN 0 0 HP_CLICK HPIS1 HPIS2
	BOOT - Reboot memory content (0: normal, 1: reboot)
	FIFO_EN - Fifo enable (0: disable, 1: enable)
	WTM_EN - FIFO watermark enable (0: disable, 1: enable)
	HP_CLICK - HPF enabled for click (0: filter bypassed, 1: enabled)
	HPIS1 - HPF enabled for interrupt generator 1 (0: bypassed, 1: enabled)
	HPIS2 - HPF enabled for interrupt generator 2 (0: bypassed, 1 enabled)   */
	writeByte(ACCEL, CTRL_REG0_XM, 0x00);

	/* CTRL_REG1_XM (0x20) (Default value: 0x07)
	Bits (7-0): AODR3 AODR2 AODR1 AODR0 BDU AZEN AYEN AXEN
	AODR[3:0] - select the acceleration data rate:
		0000=power down, 0001=3.125Hz, 0010=6.25Hz, 0011=12.5Hz,
		0100=25Hz, 0101=50Hz, 0110=100Hz, 0111=200Hz, 1000=400Hz,
		1001=800Hz, 1010=1600Hz, (remaining combinations undefined).
	BDU - block data update for accel AND mag
		0: Continuous update
		1: Output registers aren't updated until MSB and LSB have been read.
	AZEN, AYEN, and AXEN - Acceleration x/y/z-axis enabled.
		0: Axis disabled, 1: Axis enabled									 */
	writeByte(ACCEL, CTRL_REG1_XM, 0x57); // 100Hz data rate, x/y/z all enabled

	//Serial.println(xmReadByte(CTRL_REG1_XM));
	/* CTRL_REG2_XM (0x21) (Default value: 0x00)
	Bits (7-0): ABW1 ABW0 AFS2 AFS1 AFS0 AST1 AST0 SIM
	ABW[1:0] - Accelerometer anti-alias filter bandwidth
		00=773Hz, 01=194Hz, 10=362Hz, 11=50Hz
	AFS[2:0] - Accel full-scale selection
		000=+/-2g, 001=+/-4g, 010=+/-6g, 011=+/-8g, 100=+/-16g
	AST[1:0] - Accel self-test enable
		00=normal (no self-test), 01=positive st, 10=negative st, 11=not allowed
	SIM - SPI mode selection
		0=4-wire, 1=3-wire													 */
	writeByte(ACCEL, CTRL_REG2_XM, 0x00); // Set scale to 2g

	/* CTRL_REG3_XM is used to set interrupt generators on INT1_XM
	Bits (7-0): P1_BOOT P1_TAP P1_INT1 P1_INT2 P1_INTM P1_DRDYA P1_DRDYM P1_EMPTY
	*/
	// Accelerometer data ready on INT1_XM (0x04)
	writeByte(ACCEL, CTRL_REG3_XM, 0x04);
}

void initMag()
{
	/* CTRL_REG5_XM enables temp sensor, sets mag resolution and data rate
	Bits (7-0): TEMP_EN M_RES1 M_RES0 M_ODR2 M_ODR1 M_ODR0 LIR2 LIR1
	TEMP_EN - Enable temperature sensor (0=disabled, 1=enabled)
	M_RES[1:0] - Magnetometer resolution select (0=low, 3=high)
	M_ODR[2:0] - Magnetometer data rate select
		000=3.125Hz, 001=6.25Hz, 010=12.5Hz, 011=25Hz, 100=50Hz, 101=100Hz
	LIR2 - Latch interrupt request on INT2_SRC (cleared by reading INT2_SRC)
		0=interrupt request not latched, 1=interrupt request latched
	LIR1 - Latch interrupt request on INT1_SRC (cleared by readging INT1_SRC)
		0=irq not latched, 1=irq latched 									 */
	writeByte(ACCEL, CTRL_REG5_XM, 0x94); // Mag data rate - 100 Hz, enable temperature sensor

	/* CTRL_REG6_XM sets the magnetometer full-scale
	Bits (7-0): 0 MFS1 MFS0 0 0 0 0 0
	MFS[1:0] - Magnetic full-scale selection
	00:+/-2Gauss, 01:+/-4Gs, 10:+/-8Gs, 11:+/-12Gs							 */
	writeByte(ACCEL, CTRL_REG6_XM, 0x00); // Mag scale to +/- 2GS

	/* CTRL_REG7_XM sets magnetic sensor mode, low power mode, and filters
	AHPM1 AHPM0 AFDS 0 0 MLP MD1 MD0
	AHPM[1:0] - HPF mode selection
		00=normal (resets reference registers), 01=reference signal for filtering,
		10=normal, 11=autoreset on interrupt event
	AFDS - Filtered acceleration data selection
		0=internal filter bypassed, 1=data from internal filter sent to FIFO
	MLP - Magnetic data low-power mode
		0=data rate is set by M_ODR bits in CTRL_REG5
		1=data rate is set to 3.125Hz
	MD[1:0] - Magnetic sensor mode selection (default 10)
		00=continuous-conversion, 01=single-conversion, 10 and 11=power-down */
	writeByte(ACCEL, CTRL_REG7_XM, 0x00); // Continuous conversion mode

	/* CTRL_REG4_XM is used to set interrupt generators on INT2_XM
	Bits (7-0): P2_TAP P2_INT1 P2_INT2 P2_INTM P2_DRDYA P2_DRDYM P2_Overrun P2_WTM
	*/
	writeByte(ACCEL, CTRL_REG4_XM, 0x04); // Magnetometer data ready on INT2_XM (0x08)

	/* INT_CTRL_REG_M to set push-pull/open drain, and active-low/high
	Bits[7:0] - XMIEN YMIEN ZMIEN PP_OD IEA IEL 4D MIEN
	XMIEN, YMIEN, ZMIEN - Enable interrupt recognition on axis for mag data
	PP_OD - Push-pull/open-drain interrupt configuration (0=push-pull, 1=od)
	IEA - Interrupt polarity for accel and magneto
		0=active-low, 1=active-high
	IEL - Latch interrupt request for accel and magneto
		0=irq not latched, 1=irq latched
	4D - 4D enable. 4D detection is enabled when 6D bit in INT_GEN1_REG is set
	MIEN - Enable interrupt generation for magnetic data
		0=disable, 1=enable) */
	writeByte(ACCEL, INT_CTRL_REG_M, 0x09); // Enable interrupts for mag, active-low, push-pull
}
// readGyro() -- Read the gyroscope output registers.
// This function will read all six gyroscope output registers.
// The readings are stored in the class' gx, gy, and gz variables. Read
// those _after_ calling readGyro().
void readGyro()
{
	uint8_t temp[6]; // We'll read six bytes from the gyro into temp
	readBytes(GYRO, OUT_X_L_G, temp, 6); // Read 6 bytes, beginning at OUT_X_L_G
	gx = (temp[1] << 8) | temp[0]; // Store x-axis values into gx
	gy = (temp[3] << 8) | temp[2]; // Store y-axis values into gy
	gz = (temp[5] << 8) | temp[4]; // Store z-axis values into gz

	if (fabs(calcGyro(gx)) > GYRO_THRESHOLD
	 || fabs(calcGyro(gy)) > GYRO_THRESHOLD
	 || fabs(calcGyro(gz)) > GYRO_THRESHOLD) {
		alarm_fall = 1;
		printf("Fall detected\n");
	}

}

// readAccel() -- Read the accelerometer output registers.
// This function will read all six accelerometer output registers.
// The readings are stored in the class' ax, ay, and az variables. Read
// those _after_ calling readAccel().
void readAccel()
{
	uint8_t temp[6]; // We'll read six bytes from the accelerometer into temp
	readBytes(ACCEL, OUT_X_L_A, temp, 6); // Read 6 bytes, beginning at OUT_X_L_A
	ax = (temp[1] << 8) | temp[0]; // Store x-axis values into ax
	ay = (temp[3] << 8) | temp[2]; // Store y-axis values into ay
	az = (temp[5] << 8) | temp[4]; // Store z-axis values into az
}

void calLSM9DS0(float * gbias, float * abias)
{

}

void setGyroODR(enum gyro_odr gRate)
{
	// We need to preserve the other bytes in CTRL_REG1_G. So, first read it:
	uint8_t temp = readByte(GYRO, CTRL_REG1_G);
	// Then mask out the gyro ODR bits:
	temp &= 0xFF^(0xF << 4);
	// Then shift in our new ODR bits:
	temp |= (gRate << 4);
	// And write the new register value back into CTRL_REG1_G:
	writeByte(GYRO, CTRL_REG1_G, temp);
}

void setGyroScale(enum gyro_scale gScl)
{
	// We need to preserve the other bytes in CTRL_REG4_G. So, first read it:
	uint8_t temp = readByte(GYRO, CTRL_REG4_G);
	// Then mask out the gyro scale bits:
	temp &= 0xFF^(0x3 << 4);
	// Then shift in our new scale bits:
	temp |= gScl << 4;
	// And write the new register value back into CTRL_REG4_G:
	writeByte(GYRO, CTRL_REG4_G, temp);

	// We've updated the sensor, but we also need to update our class variables
	// First update gScale:
	gScale = gScl;
	// Then calculate a new gRes, which relies on gScale being set correctly:
	calcgRes();
}

void setAccelODR(enum accel_odr aRate)
{
	// We need to preserve the other bytes in CTRL_REG1_XM. So, first read it:
	uint8_t temp = readByte(ACCEL, CTRL_REG1_XM);
	// Then mask out the accel ODR bits:
	temp &= 0xFF^(0xF << 4);
	// Then shift in our new ODR bits:
	temp |= (aRate << 4);
	// And write the new register value back into CTRL_REG1_XM:
	writeByte(ACCEL, CTRL_REG1_XM, temp);
}

void setAccelScale(enum accel_scale aScl)
{
	// We need to preserve the other bytes in CTRL_REG2_XM. So, first read it:
	uint8_t temp = readByte(ACCEL, CTRL_REG2_XM);
	// Then mask out the accel scale bits:
	temp &= 0xFF^(0x3 << 3);
	// Then shift in our new scale bits:
	temp |= aScl << 3;
	// And write the new register value back into CTRL_REG2_XM:
	writeByte(ACCEL, CTRL_REG2_XM, temp);

	// We've updated the sensor, but we also need to update our class variables
	// First update aScale:
	aScale = aScl;
	// Then calculate a new aRes, which relies on aScale being set correctly:
	calcaRes();
}

void setMagODR(enum mag_odr mRate)
{
	// We need to preserve the other bytes in CTRL_REG5_XM. So, first read it:
	uint8_t temp = readByte(ACCEL, CTRL_REG5_XM);
	// Then mask out the mag ODR bits:
	temp &= 0xFF^(0x7 << 2);
	// Then shift in our new ODR bits:
	temp |= (mRate << 2);
	// And write the new register value back into CTRL_REG5_XM:
	writeByte(ACCEL, CTRL_REG5_XM, temp);
}

void setMagScale(enum mag_scale mScl)
{
	// We need to preserve the other bytes in CTRL_REG6_XM. So, first read it:
	uint8_t temp = readByte(ACCEL, CTRL_REG6_XM);
	// Then mask out the mag scale bits:
	temp &= 0xFF^(0x3 << 5);
	// Then shift in our new scale bits:
	temp |= mScl << 5;
	// And write the new register value back into CTRL_REG6_XM:
	writeByte(ACCEL, CTRL_REG6_XM, temp);

	// We've updated the sensor, but we also need to update our class variables
	// First update mScale:
	mScale = mScl;
	// Then calculate a new mRes, which relies on mScale being set correctly:
	calcmRes();
}

void setAccelABW(enum accel_abw abwRate)
{
	// We need to preserve the other bytes in CTRL_REG2_XM. So, first read it:
	uint8_t temp = readByte(ACCEL, CTRL_REG2_XM);
	// Then mask out the accel ABW bits:
	temp &= 0xFF^(0x3 << 7);
	// Then shift in our new ODR bits:
	temp |= (abwRate << 7);
	// And write the new register value back into CTRL_REG2_XM:
	writeByte(ACCEL, CTRL_REG2_XM, temp);
}

uint16_t lsm9ds0_begin(enum gyro_scale gScl, enum accel_scale aScl, enum mag_scale mScl,
		enum gyro_odr gODR, enum accel_odr aODR, enum mag_odr mODR)
{
	gScale = gScl;
	aScale = aScl;
	mScale = mScl;

	// Once we have the scale values, we can calculate the resolution
	// of each sensor. That's what these functions are for. One for each sensor
	calcgRes(); // Calculate DPS / ADC tick, stored in gRes variable
	calcmRes(); // Calculate Gs / ADC tick, stored in mRes variable
	calcaRes(); // Calculate g / ADC tick, stored in aRes variable

	/*
	 * Use the following code to verify write to a register.
	volatile uint8_t odr1 = readByte(GYRO,CTRL_REG1_G);
	writeByte(GYRO,CTRL_REG1_G, 0xFF);
	volatile uint8_t odr2 = readByte(GYRO,CTRL_REG1_G);
	*/

	uint8_t gTest = readByte(GYRO, WHO_AM_I_G);		// Read the gyro WHO_AM_I
	uint8_t xmTest = readByte(ACCEL, WHO_AM_I_XM);	// Read the accel/mag WHO_AM_I

	initGyro();
	setGyroODR(gODR);
	setGyroScale(gScale);

	initAccel();
	setAccelODR(aODR); // Set the accel data rate.
	setAccelScale(aScale); // Set the accel range.

	initMag();
	setMagODR(mODR); // Set the magnetometer output data rate.
	setMagScale(mScale); // Set the magnetometer's range.
	return (xmTest << 8) | gTest;
}

float calcGyro(int16_t gyro)
{
	// Return the gyro raw reading times our pre-calculated DPS / (ADC tick):
	return gRes * gyro;
}

float calcAccel(int16_t accel)
{
	// Return the accel raw reading times our pre-calculated g's / (ADC tick):
	return aRes * accel;
}

float calcMag(int16_t mag)
{
	// Return the mag raw reading times our pre-calculated Gs / (ADC tick):
	return mRes * mag;
}

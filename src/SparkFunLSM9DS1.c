/******************************************************************************
SFE_LSM9DS1.cpp
SFE_LSM9DS1 Library Source File
Jim Lindblom @ SparkFun Electronics
Original Creation Date: February 27, 2015
https://github.com/sparkfun/LSM9DS1_Breakout

This file implements all functions of the LSM9DS1 class. Functions here range
from higher level stuff, like reading/writing LSM9DS1 registers to low-level,
hardware reads and writes. Both SPI and I2C handler functions can be found
towards the bottom of this file.

Development environment specifics:
	IDE: Arduino 1.6
	Hardware Platform: Arduino Uno
	LSM9DS1 Breakout Version: 1.0

This code is beerware; if you see me (or any other SparkFun employee) at the
local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.
******************************************************************************/

#include "SparkFunLSM9DS1.h"
#include "LSM9DS1_Registers.h"
#include "LSM9DS1_Types.h"
#include <stdbool.h>
#include <msp430.h>

#define LSM9DS1_COMMUNICATION_TIMEOUT 1000
static struct IMUSettings settings;

float magSensitivity[4] = {0.00014, 0.00029, 0.00043, 0.00058};

static void enable_chip(enum chip_select cs)
{
	//Start communication
	//gryo enable
	if (cs == GYRO_X) {
		P1OUT &= ~BIT4;
		P3OUT |= BIT2;
	} else if (cs == MAGNO) {
		//xm enable
		P1OUT |= BIT4;
		P3OUT &= ~BIT2;
	}
}

static void disable_chip(enum chip_select cs)
{
	//End communication
	if (cs == GYRO_X) {
		P1OUT |= BIT4;
		P3OUT &= ~BIT2;
	} else if (cs == MAGNO) {
		//disable xm
		P1OUT &= ~BIT4;
		P3OUT |= BIT2;
	}
}

void lsm9ds1_init()
{
	settings.gyro.enabled = true;
	settings.gyro.enableX = true;
	settings.gyro.enableY = true;
	settings.gyro.enableZ = true;
	// gyro scale can be 245, 500, or 2000
	settings.gyro.scale = 245;
	// gyro sample rate: value between 1-6
	// 1 = 14.9    4 = 238
	// 2 = 59.5    5 = 476
	// 3 = 119     6 = 952
	settings.gyro.sampleRate = 6;
	// gyro cutoff frequency: value between 0-3
	// Actual value of cutoff frequency depends
	// on sample rate.
	settings.gyro.bandwidth = 0;
	settings.gyro.lowPowerEnable = false;
	settings.gyro.HPFEnable = false;
	// Gyro HPF cutoff frequency: value between 0-9
	// Actual value depends on sample rate. Only applies
	// if gyroHPFEnable is true.
	settings.gyro.HPFCutoff = 0;
	settings.gyro.flipX = false;
	settings.gyro.flipY = false;
	settings.gyro.flipZ = false;
	settings.gyro.orientation = 0;
	settings.gyro.latchInterrupt = true;

	settings.accel.enabled = true;
	settings.accel.enableX = true;
	settings.accel.enableY = true;
	settings.accel.enableZ = true;
	// accel scale can be 2, 4, 8, or 16
	settings.accel.scale = 2;
	// accel sample rate can be 1-6
	// 1 = 10 Hz    4 = 238 Hz
	// 2 = 50 Hz    5 = 476 Hz
	// 3 = 119 Hz   6 = 952 Hz
	settings.accel.sampleRate = 6;
	// Accel cutoff freqeuncy can be any value between -1 - 3. 
	// -1 = bandwidth determined by sample rate
	// 0 = 408 Hz   2 = 105 Hz
	// 1 = 211 Hz   3 = 50 Hz
	settings.accel.bandwidth = -1;
	settings.accel.highResEnable = false;
	// accelHighResBandwidth can be any value between 0-3
	// LP cutoff is set to a factor of sample rate
	// 0 = ODR/50    2 = ODR/9
	// 1 = ODR/100   3 = ODR/400
	settings.accel.highResBandwidth = 0;

	settings.mag.enabled = true;
	// mag scale can be 4, 8, 12, or 16
	settings.mag.scale = 4;
	// mag data rate can be 0-7
	// 0 = 0.625 Hz  4 = 10 Hz
	// 1 = 1.25 Hz   5 = 20 Hz
	// 2 = 2.5 Hz    6 = 40 Hz
	// 3 = 5 Hz      7 = 80 Hz
	settings.mag.sampleRate = 7;
	settings.mag.tempCompensationEnable = false;
	// magPerformance can be any value between 0-3
	// 0 = Low power mode      2 = high performance
	// 1 = medium performance  3 = ultra-high performance
	settings.mag.XYPerformance = 3;
	settings.mag.ZPerformance = 3;
	settings.mag.lowPowerEnable = false;
	// magOperatingMode can be 0-2
	// 0 = continuous conversion
	// 1 = single-conversion
	// 2 = power down
	settings.mag.operatingMode = 0;

	settings.temp.enabled = true;
	int i;
	for (i=0; i<3; i++)
	{
		gBias[i] = 0;
		aBias[i] = 0;
		mBias[i] = 0;
		gBiasRaw[i] = 0;
		aBiasRaw[i] = 0;
		mBiasRaw[i] = 0;
	}
	_autoCalc = false;
}

uint16_t lsm9ds1_begin()
{
	constrainScales();
	// Once we have the scale values, we can calculate the resolution
	// of each sensor. That's what these functions are for. One for each sensor
	calcgRes(); // Calculate DPS / ADC tick, stored in gRes variable
	calcmRes(); // Calculate Gs / ADC tick, stored in mRes variable
	calcaRes(); // Calculate g / ADC tick, stored in aRes variable
	
	// To verify communication, we can read from the WHO_AM_I register of
	// each device. Store those in a variable so we can return them.
	uint8_t mTest = readByte(MAGNO, WHO_AM_I_M);		// Read the gyro WHO_AM_I
	uint8_t xgTest = readByte(GYRO_X, WHO_AM_I_XG);	// Read the accel/mag WHO_AM_I
	uint16_t whoAmICombined = (xgTest << 8) | mTest;
	
	if (whoAmICombined != ((WHO_AM_I_AG_RSP << 8) | WHO_AM_I_M_RSP))
		return 0;
	
	// Gyro initialization stuff:
	initGyro();	// This will "turn on" the gyro. Setting up interrupts, etc.
	
	// Accelerometer initialization stuff:
	initAccel(); // "Turn on" all axes of the accel. Set up interrupts, etc.
	
	// Once everything is initialized, return the WHO_AM_I registers we read:
	return whoAmICombined;
}

void initGyro()
{
	uint8_t tempRegValue = 0;
	
	// CTRL_REG1_G (Default value: 0x00)
	// [ODR_G2][ODR_G1][ODR_G0][FS_G1][FS_G0][0][BW_G1][BW_G0]
	// ODR_G[2:0] - Output data rate selection
	// FS_G[1:0] - Gyroscope full-scale selection
	// BW_G[1:0] - Gyroscope bandwidth selection
	
	// To disable gyro, set sample rate bits to 0. We'll only set sample
	// rate if the gyro is enabled.
	if (settings.gyro.enabled)
	{
		tempRegValue = (settings.gyro.sampleRate & 0x07) << 5;
	}
	switch (settings.gyro.scale)
	{
		case 500:
			tempRegValue |= (0x1 << 3);
			break;
		case 2000:
			tempRegValue |= (0x3 << 3);
			break;
		// Otherwise we'll set it to 245 dps (0x0 << 4)
	}
	tempRegValue |= (settings.gyro.bandwidth & 0x3);
	writeByte(GYRO_X, CTRL_REG1_G, tempRegValue);
	
	// CTRL_REG2_G (Default value: 0x00)
	// [0][0][0][0][INT_SEL1][INT_SEL0][OUT_SEL1][OUT_SEL0]
	// INT_SEL[1:0] - INT selection configuration
	// OUT_SEL[1:0] - Out selection configuration
	writeByte(GYRO_X, CTRL_REG2_G, 0x00);
	
	// CTRL_REG3_G (Default value: 0x00)
	// [LP_mode][HP_EN][0][0][HPCF3_G][HPCF2_G][HPCF1_G][HPCF0_G]
	// LP_mode - Low-power mode enable (0: disabled, 1: enabled)
	// HP_EN - HPF enable (0:disabled, 1: enabled)
	// HPCF_G[3:0] - HPF cutoff frequency
	tempRegValue = settings.gyro.lowPowerEnable ? (1<<7) : 0;
	if (settings.gyro.HPFEnable)
	{
		tempRegValue |= (1<<6) | (settings.gyro.HPFCutoff & 0x0F);
	}
	writeByte(GYRO_X, CTRL_REG3_G, tempRegValue);
	
	// CTRL_REG4 (Default value: 0x38)
	// [0][0][Zen_G][Yen_G][Xen_G][0][LIR_XL1][4D_XL1]
	// Zen_G - Z-axis output enable (0:disable, 1:enable)
	// Yen_G - Y-axis output enable (0:disable, 1:enable)
	// Xen_G - X-axis output enable (0:disable, 1:enable)
	// LIR_XL1 - Latched interrupt (0:not latched, 1:latched)
	// 4D_XL1 - 4D option on interrupt (0:6D used, 1:4D used)
	tempRegValue = 0;
	if (settings.gyro.enableZ) tempRegValue |= (1<<5);
	if (settings.gyro.enableY) tempRegValue |= (1<<4);
	if (settings.gyro.enableX) tempRegValue |= (1<<3);
	if (settings.gyro.latchInterrupt) tempRegValue |= (1<<1);
	writeByte(GYRO_X, CTRL_REG4, tempRegValue);
	
	// ORIENT_CFG_G (Default value: 0x00)
	// [0][0][SignX_G][SignY_G][SignZ_G][Orient_2][Orient_1][Orient_0]
	// SignX_G - Pitch axis (X) angular rate sign (0: positive, 1: negative)
	// Orient [2:0] - Directional user orientation selection
	tempRegValue = 0;
	if (settings.gyro.flipX) tempRegValue |= (1<<5);
	if (settings.gyro.flipY) tempRegValue |= (1<<4);
	if (settings.gyro.flipZ) tempRegValue |= (1<<3);
	writeByte(GYRO_X, ORIENT_CFG_G, tempRegValue);
}

void initAccel()
{
	uint8_t tempRegValue = 0;
	
	//	CTRL_REG5_XL (0x1F) (Default value: 0x38)
	//	[DEC_1][DEC_0][Zen_XL][Yen_XL][Zen_XL][0][0][0]
	//	DEC[0:1] - Decimation of accel data on OUT REG and FIFO.
	//		00: None, 01: 2 samples, 10: 4 samples 11: 8 samples
	//	Zen_XL - Z-axis output enabled
	//	Yen_XL - Y-axis output enabled
	//	Xen_XL - X-axis output enabled
	if (settings.accel.enableZ) tempRegValue |= (1<<5);
	if (settings.accel.enableY) tempRegValue |= (1<<4);
	if (settings.accel.enableX) tempRegValue |= (1<<3);
	
	writeByte(GYRO_X, CTRL_REG5_XL, tempRegValue);
	
	// CTRL_REG6_XL (0x20) (Default value: 0x00)
	// [ODR_XL2][ODR_XL1][ODR_XL0][FS1_XL][FS0_XL][BW_SCAL_ODR][BW_XL1][BW_XL0]
	// ODR_XL[2:0] - Output data rate & power mode selection
	// FS_XL[1:0] - Full-scale selection
	// BW_SCAL_ODR - Bandwidth selection
	// BW_XL[1:0] - Anti-aliasing filter bandwidth selection
	tempRegValue = 0;
	// To disable the accel, set the sampleRate bits to 0.
	if (settings.accel.enabled)
	{
		tempRegValue |= (settings.accel.sampleRate & 0x07) << 5;
	}
	switch (settings.accel.scale)
	{
		case 4:
			tempRegValue |= (0x2 << 3);
			break;
		case 8:
			tempRegValue |= (0x3 << 3);
			break;
		case 16:
			tempRegValue |= (0x1 << 3);
			break;
		// Otherwise it'll be set to 2g (0x0 << 3)
	}
	if (settings.accel.bandwidth >= 0)
	{
		tempRegValue |= (1<<2); // Set BW_SCAL_ODR
		tempRegValue |= (settings.accel.bandwidth & 0x03);
	}
	writeByte(GYRO_X, CTRL_REG6_XL, tempRegValue);
	
	// CTRL_REG7_XL (0x21) (Default value: 0x00)
	// [HR][DCF1][DCF0][0][0][FDS][0][HPIS1]
	// HR - High resolution mode (0: disable, 1: enable)
	// DCF[1:0] - Digital filter cutoff frequency
	// FDS - Filtered data selection
	// HPIS1 - HPF enabled for interrupt function
	tempRegValue = 0;
	if (settings.accel.highResEnable)
	{
		tempRegValue |= (1<<7); // Set HR bit
		tempRegValue |= (settings.accel.highResBandwidth & 0x3) << 5;
	}
	writeByte(GYRO_X, CTRL_REG7_XL, tempRegValue);
}

// This is a function that uses the FIFO to accumulate sample of accelerometer and gyro data, average
// them, scales them to  gs and deg/s, respectively, and then passes the biases to the main sketch
// for subtraction from all subsequent data. There are no gyro and accelerometer bias registers to store
// the data as there are in the ADXL345, a precursor to the LSM9DS0, or the MPU-9150, so we have to
// subtract the biases ourselves. This results in a more accurate measurement in general and can
// remove errors due to imprecise or varying initial placement. Calibration of sensor data in this manner
// is good practice.
void calibrate(bool autoCalc)
{  
	uint8_t data[6] = {0, 0, 0, 0, 0, 0};
	uint8_t samples = 0;
	int ii;
	int32_t aBiasRawTemp[3] = {0, 0, 0};
	int32_t gBiasRawTemp[3] = {0, 0, 0};
	
	// Turn on FIFO and set threshold to 32 samples
	enableFIFO(true);
	setFIFO(FIFO_THS, 0x1F);
	while (samples < 0x1F)
	{
		samples = (readByte(GYRO_X, FIFO_SRC) & 0x3F); // Read number of stored samples
	}
	for(ii = 0; ii < samples ; ii++) 
	{	// Read the gyro data stored in the FIFO
		readGyro();
		gBiasRawTemp[0] += gx;
		gBiasRawTemp[1] += gy;
		gBiasRawTemp[2] += gz;
		readAccel();
		aBiasRawTemp[0] += ax;
		aBiasRawTemp[1] += ay;
		aBiasRawTemp[2] += az - (int16_t)(1./aRes); // Assumes sensor facing up!
	}  
	for (ii = 0; ii < 3; ii++)
	{
		gBiasRaw[ii] = gBiasRawTemp[ii] / samples;
		gBias[ii] = calcGyro(gBiasRaw[ii]);
		aBiasRaw[ii] = aBiasRawTemp[ii] / samples;
		aBias[ii] = calcAccel(aBiasRaw[ii]);
	}
	
	enableFIFO(false);
	setFIFO(FIFO_OFF, 0x00);
	
	if (autoCalc) _autoCalc = true;
}
//
//void calibrateMag(bool loadIn)
//{
//	int i, j;
//	int16_t magMin[3] = {0, 0, 0};
//	int16_t magMax[3] = {0, 0, 0}; // The road warrior
//
//	for (i=0; i<128; i++)
//	{
//		while (!magAvailable());
//		readMag();
//		int16_t magTemp[3] = {0, 0, 0};
//		magTemp[0] = mx;
//		magTemp[1] = my;
//		magTemp[2] = mz;
//		for (j = 0; j < 3; j++)
//		{
//			if (magTemp[j] > magMax[j]) magMax[j] = magTemp[j];
//			if (magTemp[j] < magMin[j]) magMin[j] = magTemp[j];
//		}
//	}
//	for (j = 0; j < 3; j++)
//	{
//		mBiasRaw[j] = (magMax[j] + magMin[j]) / 2;
//		mBias[j] = calcMag(mBiasRaw[j]);
//		if (loadIn)
//			magOffset(j, mBiasRaw[j]);
//	}
//
//}
//
//void magOffset(uint8_t axis, int16_t offset)
//{
//	if (axis > 2)
//		return;
//	uint8_t msb, lsb;
//	msb = (offset & 0xFF00) >> 8;
//	lsb = offset & 0x00FF;
//	mWriteByte(OFFSET_X_REG_L_M + (2 * axis), lsb);
//	mWriteByte(OFFSET_X_REG_H_M + (2 * axis), msb);
//}
//
//void initMag()
//{
//	uint8_t tempRegValue = 0;
//
//	// CTRL_REG1_M (Default value: 0x10)
//	// [TEMP_COMP][OM1][OM0][DO2][DO1][DO0][0][ST]
//	// TEMP_COMP - Temperature compensation
//	// OM[1:0] - X & Y axes op mode selection
//	//	00:low-power, 01:medium performance
//	//	10: high performance, 11:ultra-high performance
//	// DO[2:0] - Output data rate selection
//	// ST - Self-test enable
//	if (settings.mag.tempCompensationEnable) tempRegValue |= (1<<7);
//	tempRegValue |= (settings.mag.XYPerformance & 0x3) << 5;
//	tempRegValue |= (settings.mag.sampleRate & 0x7) << 2;
//	mWriteByte(CTRL_REG1_M, tempRegValue);
//
//	// CTRL_REG2_M (Default value 0x00)
//	// [0][FS1][FS0][0][REBOOT][SOFT_RST][0][0]
//	// FS[1:0] - Full-scale configuration
//	// REBOOT - Reboot memory content (0:normal, 1:reboot)
//	// SOFT_RST - Reset config and user registers (0:default, 1:reset)
//	tempRegValue = 0;
//	switch (settings.mag.scale)
//	{
//	case 8:
//		tempRegValue |= (0x1 << 5);
//		break;
//	case 12:
//		tempRegValue |= (0x2 << 5);
//		break;
//	case 16:
//		tempRegValue |= (0x3 << 5);
//		break;
//	// Otherwise we'll default to 4 gauss (00)
//	}
//	mWriteByte(CTRL_REG2_M, tempRegValue); // +/-4Gauss
//
//	// CTRL_REG3_M (Default value: 0x03)
//	// [I2C_DISABLE][0][LP][0][0][SIM][MD1][MD0]
//	// I2C_DISABLE - Disable I2C interace (0:enable, 1:disable)
//	// LP - Low-power mode cofiguration (1:enable)
//	// SIM - SPI mode selection (0:write-only, 1:read/write enable)
//	// MD[1:0] - Operating mode
//	//	00:continuous conversion, 01:single-conversion,
//	//  10,11: Power-down
//	tempRegValue = 0;
//	if (settings.mag.lowPowerEnable) tempRegValue |= (1<<5);
//	tempRegValue |= (settings.mag.operatingMode & 0x3);
//	mWriteByte(CTRL_REG3_M, tempRegValue); // Continuous conversion mode
//
//	// CTRL_REG4_M (Default value: 0x00)
//	// [0][0][0][0][OMZ1][OMZ0][BLE][0]
//	// OMZ[1:0] - Z-axis operative mode selection
//	//	00:low-power mode, 01:medium performance
//	//	10:high performance, 10:ultra-high performance
//	// BLE - Big/little endian data
//	tempRegValue = 0;
//	tempRegValue = (settings.mag.ZPerformance & 0x3) << 2;
//	mWriteByte(CTRL_REG4_M, tempRegValue);
//
//	// CTRL_REG5_M (Default value: 0x00)
//	// [0][BDU][0][0][0][0][0][0]
//	// BDU - Block data update for magnetic data
//	//	0:continuous, 1:not updated until MSB/LSB are read
//	tempRegValue = 0;
//	mWriteByte(CTRL_REG5_M, tempRegValue);
//}

uint8_t accelAvailable()
{
	uint8_t status = readByte(GYRO_X, STATUS_REG_1);
	
	return (status & (1<<0));
}

uint8_t gyroAvailable()
{
	uint8_t status = readByte(GYRO_X, STATUS_REG_1);
	
	return ((status & (1<<1)) >> 1);
}

uint8_t tempAvailable()
{
	uint8_t status = readByte(GYRO_X, STATUS_REG_1);
	
	return ((status & (1<<2)) >> 2);
}

void readAccel()
{
	uint8_t temp[6]; // We'll read six bytes from the accelerometer into temp	
	readBytes(GYRO_X, OUT_X_L_XL, temp, 6); // Read 6 bytes, beginning at OUT_X_L_XL
	ax = (temp[1] << 8) | temp[0]; // Store x-axis values into ax
	ay = (temp[3] << 8) | temp[2]; // Store y-axis values into ay
	az = (temp[5] << 8) | temp[4]; // Store z-axis values into az
	if (_autoCalc)
	{
		ax -= aBiasRaw[X_AXIS];
		ay -= aBiasRaw[Y_AXIS];
		az -= aBiasRaw[Z_AXIS];
	}
}

void readTemp()
{
	uint8_t temp[2]; // We'll read two bytes from the temperature sensor into temp	
	readBytes(GYRO_X, OUT_TEMP_L, temp, 2); // Read 2 bytes, beginning at OUT_TEMP_L
	temperature = ((int16_t)temp[1] << 8) | temp[0];
}

void readGyro()
{
	uint8_t temp[6]; // We'll read six bytes from the gyro into temp
	readBytes(GYRO_X, OUT_X_L_G, temp, 6); // Read 6 bytes, beginning at OUT_X_L_G
	gx = (temp[1] << 8) | temp[0]; // Store x-axis values into gx
	gy = (temp[3] << 8) | temp[2]; // Store y-axis values into gy
	gz = (temp[5] << 8) | temp[4]; // Store z-axis values into gz
	if (_autoCalc)
	{
		gx -= gBiasRaw[X_AXIS];
		gy -= gBiasRaw[Y_AXIS];
		gz -= gBiasRaw[Z_AXIS];
	}
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

void setGyroScale(uint16_t gScl)
{
	// Read current value of CTRL_REG1_G:
	uint8_t ctrl1RegValue = readByte(GYRO_X, CTRL_REG1_G);
	// Mask out scale bits (3 & 4):
	ctrl1RegValue &= 0xE7;
	switch (gScl)
	{
		case 500:
			ctrl1RegValue |= (0x1 << 3);
			settings.gyro.scale = 500;
			break;
		case 2000:
			ctrl1RegValue |= (0x3 << 3);
			settings.gyro.scale = 2000;
			break;
		default: // Otherwise we'll set it to 245 dps (0x0 << 4)
			settings.gyro.scale = 245;
			break;
	}
	writeByte(GYRO_X, CTRL_REG1_G, ctrl1RegValue);
	
	calcgRes();	
}

void setAccelScale(uint8_t aScl)
{
	// We need to preserve the other bytes in CTRL_REG6_XL. So, first read it:
	uint8_t tempRegValue = readByte(GYRO_X, CTRL_REG6_XL);
	// Mask out accel scale bits:
	tempRegValue &= 0xE7;
	
	switch (aScl)
	{
		case 4:
			tempRegValue |= (0x2 << 3);
			settings.accel.scale = 4;
			break;
		case 8:
			tempRegValue |= (0x3 << 3);
			settings.accel.scale = 8;
			break;
		case 16:
			tempRegValue |= (0x1 << 3);
			settings.accel.scale = 16;
			break;
		default: // Otherwise it'll be set to 2g (0x0 << 3)
			settings.accel.scale = 2;
			break;
	}
	writeByte(GYRO_X, CTRL_REG6_XL, tempRegValue);
	
	// Then calculate a new aRes, which relies on aScale being set correctly:
	calcaRes();
}

void setGyroODR(uint8_t gRate)
{
	// Only do this if gRate is not 0 (which would disable the gyro)
	if ((gRate & 0x07) != 0)
	{
		// We need to preserve the other bytes in CTRL_REG1_G. So, first read it:
		uint8_t temp = readByte(GYRO_X, CTRL_REG1_G);
		// Then mask out the gyro ODR bits:
		temp &= 0xFF^(0x7 << 5);
		temp |= (gRate & 0x07) << 5;
		// Update our settings struct
		settings.gyro.sampleRate = gRate & 0x07;
		// And write the new register value back into CTRL_REG1_G:
		writeByte(GYRO_X, CTRL_REG1_G, temp);
	}
}

void setAccelODR(uint8_t aRate)
{
	// Only do this if aRate is not 0 (which would disable the accel)
	if ((aRate & 0x07) != 0)
	{
		// We need to preserve the other bytes in CTRL_REG1_XM. So, first read it:
		uint8_t temp = readByte(GYRO_X, CTRL_REG6_XL);
		// Then mask out the accel ODR bits:
		temp &= 0x1F;
		// Then shift in our new ODR bits:
		temp |= ((aRate & 0x07) << 5);
		settings.accel.sampleRate = aRate & 0x07;
		// And write the new register value back into CTRL_REG1_XM:
		writeByte(GYRO_X, CTRL_REG6_XL, temp);
	}
}

void calcgRes()
{
	gRes = ((float) settings.gyro.scale) / 32768.0;
}

void calcaRes()
{
	aRes = ((float) settings.accel.scale) / 32768.0;
}

void calcmRes()
{
	//mRes = ((float) settings.mag.scale) / 32768.0;
	switch (settings.mag.scale)
	{
	case 4:
		mRes = magSensitivity[0];
		break;
	case 8:
		mRes = magSensitivity[1];
		break;
	case 12:
		mRes = magSensitivity[2];
		break;
	case 16:
		mRes = magSensitivity[3];
		break;
	}
	
}

void configInt(enum interrupt_select intr, uint8_t generator,
	                     enum h_lactive activeLow, enum pp_od pushPull)
{
	// Write to INT1_CTRL or INT2_CTRL. [interupt] should already be one of
	// those two values.
	// [generator] should be an OR'd list of values from the interrupt_generators enum
	writeByte(GYRO_X, intr, generator);
	
	// Configure CTRL_REG8
	uint8_t temp;
	temp = readByte(GYRO_X, CTRL_REG8);
	
	if (activeLow) temp |= (1<<5);
	else temp &= ~(1<<5);
	
	if (pushPull) temp &= ~(1<<4);
	else temp |= (1<<4);
	
	writeByte(GYRO_X, CTRL_REG8, temp);
}

void configInactivity(uint8_t duration, uint8_t threshold, bool sleepOn)
{
	uint8_t temp = 0;
	
	temp = threshold & 0x7F;
	if (sleepOn) temp |= (1<<7);
	writeByte(GYRO_X, ACT_THS, temp);
	
	writeByte(GYRO_X, ACT_DUR, duration);
}

uint8_t getInactivity()
{
	uint8_t temp = readByte(GYRO_X, STATUS_REG_0);
	temp &= (0x10);
	return temp;
}

void configAccelInt(uint8_t generator, bool andInterrupts)
{
	// Use variables from accel_interrupt_generator, OR'd together to create
	// the [generator]value.
	uint8_t temp = generator;
	if (andInterrupts) temp |= 0x80;
	writeByte(GYRO_X, INT_GEN_CFG_XL, temp);
}

void configAccelThs(uint8_t threshold, enum lsm9ds1_axis axis, uint8_t duration, bool wait)
{
	// Write threshold value to INT_GEN_THS_?_XL.
	// axis will be 0, 1, or 2 (x, y, z respectively)
	writeByte(GYRO_X, INT_GEN_THS_X_XL + axis, threshold);
	
	// Write duration and wait to INT_GEN_DUR_XL
	uint8_t temp;
	temp = (duration & 0x7F);
	if (wait) temp |= 0x80;
	writeByte(GYRO_X, INT_GEN_DUR_XL, temp);
}

uint8_t getAccelIntSrc()
{
	uint8_t intSrc = readByte(GYRO_X, INT_GEN_SRC_XL);
	
	// Check if the IA_XL (interrupt active) bit is set
	if (intSrc & (1<<6))
	{
		return (intSrc & 0x3F);
	}
	
	return 0;
}

void configGyroInt(uint8_t generator, bool aoi, bool latch)
{
	// Use variables from accel_interrupt_generator, OR'd together to create
	// the [generator]value.
	uint8_t temp = generator;
	if (aoi) temp |= 0x80;
	if (latch) temp |= 0x40;
	writeByte(GYRO_X, INT_GEN_CFG_G, temp);
}

void configGyroThs(int16_t threshold, enum lsm9ds1_axis axis, uint8_t duration, bool wait)
{
	uint8_t buffer[2];
	buffer[0] = (threshold & 0x7F00) >> 8;
	buffer[1] = (threshold & 0x00FF);
	// Write threshold value to INT_GEN_THS_?H_G and  INT_GEN_THS_?L_G.
	// axis will be 0, 1, or 2 (x, y, z respectively)
	writeByte(GYRO_X, INT_GEN_THS_XH_G + (axis * 2), buffer[0]);
	writeByte(GYRO_X, INT_GEN_THS_XH_G + 1 + (axis * 2), buffer[1]);
	
	// Write duration and wait to INT_GEN_DUR_XL
	uint8_t temp;
	temp = (duration & 0x7F);
	if (wait) temp |= 0x80;
	writeByte(GYRO_X, INT_GEN_DUR_G, temp);
}

uint8_t getGyroIntSrc()
{
	uint8_t intSrc = readByte(GYRO_X, INT_GEN_SRC_G);
	
	// Check if the IA_G (interrupt active) bit is set
	if (intSrc & (1<<6))
	{
		return (intSrc & 0x3F);
	}
	
	return 0;
}

void configMagInt(uint8_t generator, enum h_lactive activeLow, bool latch)
{
	// Mask out non-generator bits (0-4)
	uint8_t config = (generator & 0xE0);	
	// IEA bit is 0 for active-low, 1 for active-high.
	if (activeLow == INT_ACTIVE_HIGH) config |= (1<<2);
	// IEL bit is 0 for latched, 1 for not-latched
	if (!latch) config |= (1<<1);
	// As long as we have at least 1 generator, enable the interrupt
	if (generator != 0) config |= (1<<0);
	
	writeByte(MAGNO, INT_CFG_M, config);
}

void sleepGyro(bool enable)
{
	uint8_t temp = readByte(GYRO_X, CTRL_REG9);
	if (enable) temp |= (1<<6);
	else temp &= ~(1<<6);
	writeByte(GYRO_X, CTRL_REG9, temp);
}

void enableFIFO(bool enable)
{
	uint8_t temp = readByte(GYRO_X, CTRL_REG9);
	if (enable) temp |= (1<<1);
	else temp &= ~(1<<1);
	writeByte(GYRO_X, CTRL_REG9, temp);
}

void setFIFO(enum fifoMode_type fifoMode, uint8_t fifoThs)
{
	// Limit threshold - 0x1F (31) is the maximum. If more than that was asked
	// limit it to the maximum.
	uint8_t threshold = fifoThs <= 0x1F ? fifoThs : 0x1F;
	writeByte(GYRO_X, FIFO_CTRL, ((fifoMode & 0x7) << 5) | (threshold & 0x1F));
}

uint8_t getFIFOSamples()
{
	return (readByte(GYRO_X, FIFO_SRC) & 0x3F);
}

void constrainScales()
{
	if ((settings.gyro.scale != 245) && (settings.gyro.scale != 500) && 
		(settings.gyro.scale != 2000))
	{
		settings.gyro.scale = 245;
	}
		
	if ((settings.accel.scale != 2) && (settings.accel.scale != 4) &&
		(settings.accel.scale != 8) && (settings.accel.scale != 16))
	{
		settings.accel.scale = 2;
	}
		
	if ((settings.mag.scale != 4) && (settings.mag.scale != 8) &&
		(settings.mag.scale != 12) && (settings.mag.scale != 16))
	{
		settings.mag.scale = 4;
	}
}

uint8_t readByte(enum chip_select cs, uint8_t subAddress)
{
	uint8_t temp;
	// Use the multiple read function to read 1 byte.
	// Value is returned to `temp`.
	readBytes(cs, subAddress, &temp, 1);
	return temp;
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

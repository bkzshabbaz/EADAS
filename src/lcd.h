#ifndef _LCD_H
#define _LCD_H

#include<stdint.h>

#define ALPHA_NUM 0
#define SYMBOL 1

enum {
	CLR = 0, EXCL = 1, REC = 2, HRT = 3, TMR = 4
};

void lcdInit(void);
void lcdOff(void);
void lcdOn(void);
void lcdSegOn(void);
void lcdSegOff(void);
void lcdIOInit(void);
void lcdPrint(char *c, uint8_t start, uint8_t end);
void lcdBlinkSym(uint8_t symbol,uint8_t status);
void incSeg(void);
void chkSeg(void);
void entrPhone(void);

#endif

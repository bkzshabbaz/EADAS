#ifndef _LCD_H
#define _LCD_H

#include<stdint.h>
#define ALPHA_NUM 0
#define SYMBOL 1

void lcdInit(void);
void lcdOff(void);
void lcdOn(void);
void lcdSegOn(void);
void lcdSegOff(void);
void lcdIOInit(void);
void lcdPrint(char *c, int alphaNum,uint16_t start,uint16_t end);

#endif

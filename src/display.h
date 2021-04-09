#ifndef DISPLAY_H
#define	DISPLAY_H

#ifdef	__cplusplus
extern "C" {
#endif

void setSevenSegData(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
    
void setSevenSegPos(int pos, uint8_t data);

void setSevenSegDots(uint8_t dots);

void runDisplayTick(void);

uint8_t digitTo7Seg(int digit);

bool anyButton(void);
bool readLeftButton(void);
bool readRightButton(void);

void displayText(char c0, char c1, char c2, char c3);

void displayDecimal(int16_t value, uint8_t decimalPointPos);

void clearDisplay(void);

void displayHex(uint16_t value);

void displayCapacitance(double value, bool isUncertain);

#ifdef	__cplusplus
}
#endif

#endif	/* DISPLAY_H */


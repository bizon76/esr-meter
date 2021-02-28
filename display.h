#ifndef DISPLAY_H
#define	DISPLAY_H

#ifdef	__cplusplus
extern "C" {
#endif

void setSevenSegData(int pos, uint8_t data);

void setSevenSegDots(uint8_t dots);

void runDisplayTick(void);

uint8_t digitTo7Seg(int digit);

bool readButton1();
 
bool readButton2();

void displayText(char c1, char c2, char c3, char c4);

void clearDisplay(void);

void displayHex(uint16_t value);


#ifdef	__cplusplus
}
#endif

#endif	/* DISPLAY_H */


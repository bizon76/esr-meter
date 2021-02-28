#include "mcc_generated_files/mcc.h"
#include "display.h"

// Map segments to ports, a = bit 0, b = bit 1, ...
void output7Seg(uint8_t segs)
{
    PORTC = 0xff; // Set all high
    IO_RC0_TRIS = (segs & 0x01) == 0;
    IO_RC1_TRIS = (segs & 0x02) == 0;
    IO_RC2_TRIS = (segs & 0x04) == 0;
    IO_RC3_TRIS = (segs & 0x08) == 0;
    IO_RC4_TRIS = (segs & 0x10) == 0;
    IO_RC5_TRIS = (segs & 0x20) == 0;
    IO_RC6_TRIS = (segs & 0x40) == 0;
    IO_RC7_TRIS = (segs & 0x80) == 0;
}

void selectDigit(int digit)
{
    PORTB = 0; // Set all low
    TRISB = 0xff; // Set all to input first
    IO_RA2_PORT = 0;
    IO_RA2_TRIS = 1;
    if(digit < 0)
        return;
    IO_RB4_TRIS = digit != 0;
    IO_RB5_TRIS = digit != 1;
    IO_RB6_TRIS = digit != 2;
    IO_RB7_TRIS = digit != 3;
    IO_RA2_TRIS = digit != 4;
}

const uint8_t digitTable[] = {
    0b00111111, //0
    0b00000110, //1
    0b01011011, //2
    0b01001111, //3
    0b01100110, //4
    0b01101101, //5
    0b01111101, //6
    0b00000111, //7
    0b01111111, //8
    0b01101111, //9
    0b01110111, //A
    0b01111100, //b
    0b00111001, //C
    0b01011110, //d
    0b01111001, //E
    0b01110001, //F
};

const uint8_t letterTable[] = {
    0b00000000, // ' '
    0b01110111, //A
    0b01111100, //b
    0b00111001, //C
    0b01011110, //d
    0b01111001, //E
    0b01110001, //F
    0b10000000, //G *
    0b10000000, //H *
    0b10000000, //I *
    0b10000000, //J *
    0b10000000, //K *
    0b10000000, //L *
    0b10000000, //M *
    0b10000000, //N *
    0b10000000, //O *
    0b10000000, //P *
    0b10000000, //Q *
    0b10000000, //R *
    0b10000000, //S *
    0b01111000, //T *
    0b10000000, //U *
    0b10000000, //V *
    0b10000000, //W *
    0b10000000, //X *
    0b10000000, //Y *
    0b10000000, //Z *
};

uint8_t digitTo7Seg(int digit)
{
    if(digit > 15 || digit < 0)
        return 0x80; // dp
    return digitTable[digit];
}

 uint8_t sevenSegData[] = {0,0,0,0};
 uint8_t sevenSegDots = 0;
 uint8_t digits[] = {0,0,0,0};
 int currentDigit = 0;
 bool button1_down = false, button2_down = false;

 bool readButton1()
 {
     return button1_down;
 }
 
 bool readButton2()
 {
     return button2_down;
 }
 
void readButtonsStart()
{
    // Set weak pull ups on tristate for RC3,RC4
    IO_RC3_WPU = 1;
    IO_RC4_WPU = 1;
}

void readButtonsEnd()
{
    button1_down = !IO_RC3_PORT;
    button2_down = !IO_RC4_PORT;
    // Remove weak pull ups on tristate for RC3,RC4
    IO_RC3_WPU = 0;
    IO_RC4_WPU = 0;
}

uint8_t charToSevenSeg(char ch)
{
    char x = ch & 0x3f;
    if(x > 26)
        x = 0;
    return letterTable[x] | (ch & 0x80);
}

void displayText(char c1, char c2, char c3, char c4)
{
    setSevenSegData(0, charToSevenSeg(c1));
    setSevenSegData(1, charToSevenSeg(c2));
    setSevenSegData(2, charToSevenSeg(c3));
    setSevenSegData(3, charToSevenSeg(c4));
}

void displayHex(uint16_t value)
{
    setSevenSegData(0, digitTo7Seg( (value >> 12) & 0xf));
    setSevenSegData(1, digitTo7Seg( (value >> 8) & 0xf));
    setSevenSegData(2, digitTo7Seg( (value >> 4) & 0xf));
    setSevenSegData(3, digitTo7Seg( value & 0xf));
}

void clearDisplay(void)
{
    for(int i=0; i < 4; i++)
        sevenSegData[i] = 0;
    sevenSegDots = 0;
}

void setSevenSegData(int pos, uint8_t data)
{
    sevenSegData[pos] = data;
}

void setSevenSegDots(uint8_t dots)
{
    sevenSegDots = dots;
}
 
 void runDisplayTick(void)
 {
    if(currentDigit == 0)
        readButtonsEnd();

    selectDigit(-1); // Clear digit when changing
    //output7Seg(digitTo7Seg( (i + d) & 0x0f));
    if(currentDigit < 4)
    {
        output7Seg(sevenSegData[currentDigit]);
    }
    else 
    {
        output7Seg(sevenSegDots & 0x7); // only first 3 bits connected
        readButtonsStart();
    }
    selectDigit(currentDigit);
    
    currentDigit ++;
    if(currentDigit == 5)
        currentDigit = 0;
 }

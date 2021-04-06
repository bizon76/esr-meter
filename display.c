#include "mcc_generated_files/mcc.h"
#include "display.h"

// Map segments to ports, a = bit 0, b = bit 1, ...
void output7Seg(uint8_t segs)
{
    PORTC = 0xff; // Set all high
    TRISC = ~segs;
    /*IO_RC0_TRIS = (segs & 0x01) == 0;
    IO_RC1_TRIS = (segs & 0x02) == 0;
    IO_RC2_TRIS = (segs & 0x04) == 0;
    IO_RC3_TRIS = (segs & 0x08) == 0;
    IO_RC4_TRIS = (segs & 0x10) == 0;
    IO_RC5_TRIS = (segs & 0x20) == 0;
    IO_RC6_TRIS = (segs & 0x40) == 0;
    IO_RC7_TRIS = (segs & 0x80) == 0;*/
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

#define _7SEG_MINUS 0b01000000

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
    0b01010100, //N
    0b01011100, //O
    0b01110011, //P
    0b10000000, //Q *
    0b01010000, //R
    0b01101101, //S
    0b01111000, //T
    0b00011100, //U
    0b10000000, //V *
    0b10000000, //W *
    0b10000000, //X *
    0b10000000, //Y *
    0b01011011, //Z
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

 bool readLeftButton(void)
 {
     return button1_down;
 }
 
 bool readRightButton(void)
 {
     return button2_down;
 }
 
 bool anyButton(void)
 {
     return button1_down || button2_down;
 }
 
void readButtonsStart(void)
{
    // Set weak pull ups on tristate for RC3,RC4
    IO_RC3_WPU = 1;
    IO_RC4_WPU = 1;
}

void readButtonsEnd(void)
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

void setSevenSegData(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
    sevenSegData[0] = d0;
    sevenSegData[1] = d1;
    sevenSegData[2] = d2;
    sevenSegData[3] = d3;
}

void displayText(char c0, char c1, char c2, char c3)
{
    sevenSegDots = 0;
    setSevenSegData(
            charToSevenSeg(c0),
            charToSevenSeg(c1), 
            charToSevenSeg(c2),
            charToSevenSeg(c3));
}

void displayHex(uint16_t value)
{
    setSevenSegData(
            digitTo7Seg( (value >> 12) & 0xf),
            digitTo7Seg( (value >> 8) & 0xf),
            digitTo7Seg( (value >> 4) & 0xf),
            digitTo7Seg( value & 0xf));
}

// 0 <= value <= 9999
// decimalPointPos from left to right, 0 = leftMost
void displayDecimal(int16_t value, uint8_t decimalPointPos)
{
    sevenSegDots = 0;

    bool positive = value >= 0;
    if (!positive)
        value = -value;

    bool showMinus = !positive;

    if (!positive && value > 999)
    {
        if (decimalPointPos > 2)
        {
            // Error negative overflow "- O.F."
            setSevenSegData(_7SEG_MINUS, 0, digitTable[0] | 0x80, digitTable[0xf] | 0x80);
            return;
        }
        decimalPointPos++;
        value /= 10;
    } 
    else if(positive && value > 9999)
    {
        if(decimalPointPos > 2)
        {
            // Error positive overflow "  O.F."
            setSevenSegData(0, 0, digitTable[0] | 0x80, digitTable[0xf] | 0x80);
            return;
        }
        value /= 10;
        decimalPointPos++;
    }

    for (int i = 3; i >= 0; i--)
    {
        int digit = (int)(value % 10);
        uint8_t sevenSeg = digitTo7Seg(digit);
        if (i < decimalPointPos && value == 0)
        {
            if (showMinus)
            {
                sevenSeg = _7SEG_MINUS;
                showMinus = false;
            }
            else
                sevenSeg = 0;
        }
        if (i == decimalPointPos && i != 3)
        {
            if (showMinus && i == 0 && digit == 0)
                sevenSeg = _7SEG_MINUS;
            sevenSeg |= 0x80; // Add decimal point
        }
        sevenSegData[i] = sevenSeg;
        value /= 10;
    }
}

// value in uF
void displayCapacitance(double value) 
{
    if(value < 0)
        value = 0; 
    char unit = ' ';
    uint8_t decimalPointPos;
    uint16_t number;
    if(value < 1)
    {
        number = (uint16_t)(value * 1000);
        unit = 'N';
        decimalPointPos = 4;
    }
    else if(value >= 1 && value < 10)
    {
        number = (uint16_t)(value * 100);
        unit = 'U';
        decimalPointPos = 0;
    }
    else if(value >= 10 && value < 100)
    {
        number = (uint16_t)(value * 10);
        unit = 'U';
        decimalPointPos = 1;
    }
    else if(value >= 100 && value < 1000)
    {
        number = (uint16_t)value;
        unit = 'U';
        decimalPointPos = 4;
    }
    else if(value >= 1000 && value < 10000)
    {
        number = (uint16_t)(value / 10);
        unit = 'M';
        decimalPointPos = 0;
    }
    else if(value >= 10000 && value < 100000)
    {
        number = (uint16_t)(value / 100);
        unit = 'M';
        decimalPointPos = 1;
    }
    else // over range
    {
        sevenSegDots = 0;
        setSevenSegData(0, 0, digitTable[0] | 0x80, digitTable[0xf] | 0x80);
        return;
    }

    for (int i = 2; i >= 0; i--)
    {
        int digit = (int)(number % 10);
        uint8_t sevenSeg = digitTo7Seg(digit);
        if (i < decimalPointPos && number == 0)
        {
            sevenSeg = 0;
        }
        if (i == decimalPointPos)
        {
            sevenSeg |= 0x80; // Add decimal point
        }
        sevenSegData[i] = sevenSeg;
        number /= 10;
    }
    
    if(unit == 'M')
    {
        sevenSegDots = 4;
        sevenSegData[3] = 0;
    }
    else
    {
        sevenSegDots = 0;
        sevenSegData[3] = charToSevenSeg(unit);
    }
}

void clearDisplay(void)
{
    for(int i=0; i < 4; i++)
        sevenSegData[i] = 0;
    sevenSegDots = 0;
}

void setSevenSegPos(int pos, uint8_t data)
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

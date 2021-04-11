#include "mcc_generated_files/mcc.h"
#include "display.h"
#include "esr.h"
#include "acquire.h"

bool findRangeAndMeasureESR(int32_t lowZeroOffset, int32_t highZeroOffset)
{
    // Find range using 16 samples average
    struct doubleSampleData data = fastDoubleSample(_LATA_LATA1_MASK, 16);
    if(data.overRange)
    {
        clearDisplay();
        setSevenSegDots(1);
        return false;
    }
    
    bool lowOhmsRange = data.secondSum < (185 << 4); // About 4.5%
    
    uint8_t pinMask, pointPos;

    if (lowOhmsRange)
    {
        pinMask = _LATA_LATA0_MASK;
        pointPos = 0;
    }
    else
    {
        pinMask = _LATA_LATA1_MASK;
        pointPos = 1;
    }
    
    // Do the measurement with 128 samples
    data = fastDoubleSample(pinMask, 128);

    if(data.overRange)
    {
        clearDisplay();
        setSevenSegDots(1); // Used as power-on led
        return false;
    }
    
    // 7.5 us between samples 
     // estimated 3 us delay from start
     int32_t diff = data.secondSum - data.firstSum;
     int32_t val = data.firstSum - data.startOffsetSum;
     if(diff > 0)
         val -= (diff*5)/15;// (diff>>2) - (diff>>4);

     if(lowOhmsRange)
     {
         val -= lowZeroOffset;
         // 1 count on ADC =  0.0125 ohm
         // 1.25x = 5x/4
         val *= 5; 
         val >>= 2+7;
     }
     else
     {
         val -= highZeroOffset;
         // 1 count on ADC = 0.25 ohm
         // 2.5x = 5x/2
         val *= 5;
         val >>= 1+7;        
     }

     displayDecimal((int16_t)val, pointPos);
     return true;
}

void readEsr(int32_t lowZeroOffset, int32_t highZeroOffset)
{
    while(!anyButton())
    {
        findRangeAndMeasureESR(lowZeroOffset, highZeroOffset);
        __delay_ms(50);
    }
}
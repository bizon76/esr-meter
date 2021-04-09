#include "mcc_generated_files/mcc.h"
#include "display.h"
#include "esr.h"
#include "acquire.h"

void readEsr(int32_t lowZeroOffset, int32_t highZeroOffset)
{
    while(true)
    {
        bool lowOhmsRange = false;

        while(true)
        {
            if(anyButton())
                return;
            
            uint8_t pinMask, pointPos;
            // float calMultiplier;

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

            struct doubleSampleData data = fastDoubleSample(pinMask, 128);

            if(data.overRange)
            {
                clearDisplay();
                setSevenSegDots(1); // Used as power-on led
                break;
            }
            
            if(!lowOhmsRange && data.secondSum < (185 << 7)) // Go to lowOhms range if reading is below about 4.5% of highOhmsRange
            {
                lowOhmsRange = true;
                continue;
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

            //displayHex(offsetSum>>7);

    /*        displayHex(
                    ((firstSum - (diff>>2) - (diff>>3)) >> 7)
                    //(cycleSum>>7) + 
                      );/*/
    //outer:
            //__delay_ms(1);
        }
    }
}
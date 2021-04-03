#include "mcc_generated_files/mcc.h"
#include "resistance.h"
#include "acquire.h"
#include "display.h"

// Measure ohms, uses "long" pulses to allow some inductance
// Returns the sum of the samples, or if max ADC value is detected it returns -1
int32_t takeRawOhmsMeasurement(uint8_t currentSourcePinMask, uint16_t sampleCount)
{
    IO_RA4_LAT = 0; // Turn off discharge mosfet
    LATA &= ~currentSourcePinMask; // turn on current-source

    __delay_us(500); // Wait for current to stabilize (could be an inductor)

    int32_t sum = multiSampleAdc(sampleCount);
    LATA |= currentSourcePinMask; // turn off current-source
    
    if(sum < 0)
    {
        IO_RA4_LAT = 1; // Turn on discharge mosfet
        return -1;
    }

    __delay_ms(3);
    IO_RA4_LAT = 1; // Turn on discharge mosfet
    __delay_ms(1);

    return sum;
}

void readOhms(int32_t lowOhmsRangeZeroOffset, int32_t highOhmsRangeZeroOffset)
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

            pinMask = lowOhmsRange ? _LATA_LATA0_MASK : _LATA_LATA1_MASK;

            int32_t val = takeRawOhmsMeasurement(pinMask, 256);
            if(val < 0)
            {
                clearDisplay();
                setSevenSegDots(1); // Used as power-on led
                break;
            }
            
            if(!lowOhmsRange && val < ((uint32_t)185 << 8)) // Go to lowOhms range if reading is below about 4.5% of highOhmsRange
            {
                lowOhmsRange = true;
                continue;
            }

            if(lowOhmsRange)
            {
                pointPos = 0;
                val -= lowOhmsRangeZeroOffset;
                // 1 count on ADC =  0.0125 ohm
                // 1.25x = 5x/4
                val *= 5; 
                val >>= 2+8;
            }
            else
            {
                pointPos = 1;
                val -= highOhmsRangeZeroOffset;
                // 1 count on ADC = 0.25 ohm
                // 2.5x = 5x/2
                val *= 5;
                val >>= 1+8;
            }
            
            displayDecimal((int16_t)val, pointPos);
            __delay_ms(70);
            
        }
        __delay_ms(70);
    }
}
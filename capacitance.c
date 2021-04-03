#include "mcc_generated_files/mcc.h"
#include "display.h"
#include "acquire.h"

void measureCapacitance(void)
{
    // C = I / (dV/dT)
    
    while(true)
    {
        uint8_t pinMask = _LATA_LATA1_MASK;

        struct doubleSampleData data = fastDoubleSample(pinMask, 128);
        if(data.overRange)
        {
            clearDisplay();
            setSevenSegDots(1); // Used as power-on led
            break;
        }
        
        // 60 cycles between sample 
        //int32_t di= data.secondSum - data.firstSum;
        double diff = (double)(data.secondSum - data.firstSum);
        double dT_us = 60.0 / 8.0;
        double I = 0.0025;
        double dV = diff / 1000.0 / 16.0 / 128.0;
        double C = I / (dV / dT_us);
        
        uint16_t x = (uint16_t)(C * 1000);
        displayDecimal(x, 0);
    }
}

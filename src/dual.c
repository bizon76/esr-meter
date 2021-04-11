#include "mcc_generated_files/mcc.h"
#include "acquire.h"
#include "display.h"
#include "capacitance.h"
#include "esr.h"
#include "dual.h"

void dualMeasureCapAndESR(int32_t lowZeroOffset, int32_t highZeroOffset)
{
    while(!anyButton())
    {
        // Quick check if there is anything connected using low current
        struct doubleSampleData sample = fastDoubleSample(_LATA_LATA1_MASK, 1);
        if(sample.overRange)
        {
            clearDisplay();
            setSevenSegDots(1);
            __delay_ms(50);
            continue;
        }

        // Measure capacitance
        TMR2_Start();
        while(!TMR2_HasOverflowOccured())
        {
            if(anyButton())
            {
                TMR2_Stop();
                return;
            }
            
            findRangeAndMeasureC();
            __delay_ms(50);
        }
        
        // Measure ESR
        TMR2_Start();
        while(!TMR2_HasOverflowOccured())
        {
            if(anyButton())
            {
                TMR2_Stop();
                return;
            }
            findRangeAndMeasureESR(lowZeroOffset, highZeroOffset);
            __delay_ms(50);
        }
    }
    
}
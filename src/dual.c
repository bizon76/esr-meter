#include "mcc_generated_files/mcc.h"
#include "acquire.h"
#include "display.h"
#include "capacitance.h"
#include "esr.h"
#include "dual.h"

void startTimer()
{
    PIR4bits.TMR2IF = 0;
    TMR2_Start();
}




void doDualMeasure(int32_t lowZeroOffset, int32_t highZeroOffset)
{
    while(!anyButton())
    {
        // Quick check if there is anything connected using low current
        struct doubleSampleData sample = fastDoubleSample(_LATA_LATA1_MASK, 1);
        if(sample.overRange)
        {
            clearDisplay();
            setSevenSegDots(1);
            return;
        }

        // Measure capacitance
        startTimer();
        while(!TMR2_HasOverflowOccured())
        {
            if(anyButton() | !findRangeAndMeasureC())
            {
                TMR2_Stop();
                return;
            }
            __delay_ms(50);
        }
        
        // Measure ESR
        startTimer();
        while(!TMR2_HasOverflowOccured())
        {
            if(anyButton() || !findRangeAndMeasureESR(lowZeroOffset, highZeroOffset))
            {
                TMR2_Stop();
                return;
            }
            __delay_ms(50);
        }
    }
}

void dualMeasureCapAndESR(int32_t lowZeroOffset, int32_t highZeroOffset)
{
    while(!anyButton())
    {
        doDualMeasure(lowZeroOffset, highZeroOffset);
        __delay_ms(50);
    }
}
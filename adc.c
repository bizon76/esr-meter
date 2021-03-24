#include "mcc_generated_files/mcc.h"
#include "adc.h"

// Takes [sampleCount] readings of ADC in quick succession and returns summed value
// Exits early with -1 if 0xfff is sampled
int32_t multiSampleAdc(uint16_t sampleCount)
{
    int32_t sum = 0;
    for(int i=0; i < sampleCount; i++)
    {
        ADCON0bits.GO = 1; //Start conversion
        while (ADCON0bits.GO) {} //Wait for conversion done

        uint16_t adcResult = readAdc();
        if(adcResult == 0xfff)
            return -1;

        sum += adcResult;
    }
    return sum;
}
#include "mcc_generated_files/mcc.h"
#include "display.h"
#include "acquire.h"


struct capRange
{
    int8_t index;
    uint8_t burstLength;
    uint16_t burstSpacing;
    uint8_t repeat;
    bool useLowCurrent;
};

struct capRange getRangeByIndex(int8_t index)
{
    // TODO: Maybe use an array of structs instead
    switch(index)
    {
        case  0: return (struct capRange) { 0,  1,   1, 16, true};
        case  1: return (struct capRange) { 1,  2,   2,  8, true };
        case  2: return (struct capRange) { 2,  4,   4,  4, true };
        case  3: return (struct capRange) { 3,  8,   8,  2, true };
        case  4: return (struct capRange) { 4, 16,  16,  1, true };
        case  5: return (struct capRange) { 5, 16,  32,  1, true };
        case  6: return (struct capRange) { 6, 16,  64,  1, true };
        case  7: return (struct capRange) { 7, 16, 128,  1, true };
        case  8: return (struct capRange) { 8, 16, 256,  1, true };
        case  9: return (struct capRange) { 9, 16, 512,  1, true };
        case 10: return (struct capRange) {10, 16,  16,  1, false};
        case 11: return (struct capRange) {11, 16,  32,  1, false};
        case 12: return (struct capRange) {12, 16,  64,  1, false};
        case 13: return (struct capRange) {13, 16, 128,  1, false};
        case 14: return (struct capRange) {14, 16, 256,  1, false};
        case 15: return (struct capRange) {15, 16, 512,  1, false};
        default: return (struct capRange) {-1,  0,   0,  0,  true}; 
    }
}

struct doubleSampleData measureRange(struct capRange range, uint8_t repeats)
{
    uint8_t currentSourcePinMask = range.useLowCurrent ? _LATA_LATA1_MASK : _LATA_LATA0_MASK;
    struct doubleSampleData res = {0};

    for(uint8_t r = repeats; r > 0; r--)
    {
        for(uint8_t s = range.repeat; s > 0; s--)
        {
            struct doubleSampleData data;
            if(range.burstLength == range.burstSpacing)
            {
                data = sampleSlope(range.burstLength, currentSourcePinMask);
            }
            else
            {
                const uint16_t instrPerSample = 0x3b;
                data = sampleSlopeWithDelay(range.burstLength, currentSourcePinMask, range.burstSpacing * instrPerSample);
            }
            if(data.overRange)
            {
                res.overRange = true;
                return res;
            }

            res.firstSum += data.firstSum;
            res.secondSum += data.secondSum;
        }
    }
    return res;
}

void findRangeAndMeasureC()
{
    int8_t rangeIndex = 0;
    struct capRange range = getRangeByIndex(rangeIndex);
    struct doubleSampleData data;
    
    // Try to find a good measurement range...
    while(true)
    {
        if(anyButton())
            return;
        data = measureRange(range, 1);
        if(data.overRange)
        {
            if(rangeIndex > 0)
            {
                // go back one range, this one overflowed for some reason
                rangeIndex--;
                range = getRangeByIndex(rangeIndex);
                break;
            }
            // If we get overflow on first range we can't do anything more, 
            // Show the "dot" and exit
            clearDisplay();
            setSevenSegDots(1);
            return;
        }
        struct capRange nextRange = getRangeByIndex(rangeIndex + 1);
        if(nextRange.index < 0)
            break;
    
        // check if next range would be inside measurement bounds
        int32_t diff = data.secondSum - data.firstSum;
        int32_t deltaPerSample = (diff<<8) / range.burstSpacing; // shift up by 8 for more accuracy
        
        int32_t estimatedMax = data.firstSum + (((nextRange.burstSpacing +(nextRange.burstLength >> 1)) * deltaPerSample) >> 8);
        
        // Special case if we switch from low to high current range
        if(range.useLowCurrent && !nextRange.useLowCurrent)
            estimatedMax *= 20; // Current is 20x higher

        estimatedMax >>= 4; // We sampled 16 times, shift that down to get to 12-bit range
        if(estimatedMax > 0xc00) // above 3/4 of max, stop here
            break;

        range = nextRange;
        rangeIndex = nextRange.index;
    }
    
    // Do the real measurement, with 16x averaging this time
    data = measureRange(range, 16);
    
    double diff = (double)(data.secondSum - data.firstSum);
    // 59 cycles per sample, 8 cycles per us
    double dT_us = (59.0 / 8.0) * range.burstSpacing;
    double I = range.useLowCurrent ? 0.0025 : 0.05;
    
    // 1000 mV/V, 16x amplification, 256 total samples
    double dV = diff / 1000.0 / 16.0 / 256.0;
    if(dV > 0)
    {
        double C_uF = I / (dV / dT_us);
        bool isUncertain = dV < 0.025; // Less than 10% of range used ?
        displayCapacitance(C_uF, isUncertain);
    }
    else
        displayCapacitance(1e9, false); // overflow
}

void measureCapacitance(void)
{
    while(!anyButton())
    {
        findRangeAndMeasureC();
        __delay_ms(50);
    }
    // C = I / (dV/dT)    
}
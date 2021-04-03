/* 
 * File:   adc.h
 * Author: Erik
 *
 * Created on March 23, 2021, 10:05 PM
 */

#ifndef ADC_H
#define	ADC_H

#ifdef	__cplusplus
extern "C" {
#endif

#define readAdc() ((uint16_t)((ADRESH << 8) + ADRESL));
#define readAdcPrev() ((uint16_t)((ADPREVH << 8) + ADPREVL));

struct doubleSampleData
{
    int32_t startOffsetSum;
    int32_t firstSum;
    int32_t secondSum;
    int32_t cyclesSum;
    bool overRange;
};

int32_t multiSampleAdc(uint16_t sampleCount);
struct doubleSampleData fastDoubleSample(uint8_t currentSourcePinMask, uint16_t sampleCount);
struct doubleSampleData doubleBurstSample(uint8_t currentSourcePinMask, uint8_t burstLength);
bool burstSampleSum(uint8_t burstLength, int24_t* aggregate);
#ifdef	__cplusplus
}
#endif

#endif	/* ADC_H */


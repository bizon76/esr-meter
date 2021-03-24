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

int32_t multiSampleAdc(uint16_t sampleCount);
        
#ifdef	__cplusplus
}
#endif

#endif	/* ADC_H */


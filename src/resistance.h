/* 
 * File:   resistance.h
 * Author: Erik
 *
 * Created on March 23, 2021, 10:03 PM
 */

#ifndef RESISTANCE_H
#define	RESISTANCE_H

#ifdef	__cplusplus
extern "C" {
#endif

void readOhms(int32_t lowOhmsRangeZeroOffset, int32_t highOhmsRangeZeroOffset);
int32_t takeRawOhmsMeasurement(uint8_t currentSourcePinMask, uint16_t sampleCount);

#ifdef	__cplusplus
}
#endif

#endif	/* RESISTANCE_H */


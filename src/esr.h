/* 
 * File:   esr.h
 * Author: Erik
 *
 * Created on March 23, 2021, 8:25 PM
 */

#ifndef ESR_H
#define	ESR_H

#ifdef	__cplusplus
extern "C" {
#endif

void readEsr(int32_t lowOhmsRangeZeroOffset, int32_t highOhmsRangeZeroOffset);
void findRangeAndMeasureESR(int32_t lowZeroOffset, int32_t highZeroOffset);

#ifdef	__cplusplus
}
#endif

#endif	/* ESR_H */


#include "mcc_generated_files/mcc.h"
#include "acquire.h"
#include "display.h"

#define waitForAdc()  __asm("MOVLB 0x2 \n BTFSC ADCON0, 0x0 \n GOTO $-1");
#define waitForAdcAndRestart()  __asm("MOVLB 0x2 \n BTFSC ADCON0, 0x0 \n GOTO $-1 \n BSF ADCON0, 0x0");
#define startTimer()     __asm("MOVLB 0x4 \n BSF T1CON, 0x0");
#define stopTimer()      __asm("MOVLB 0x4 \n BCF T1CON, 0x0");
#define waitForTimerOverFlow() __asm("MOVLB 0xE \n BTFSS PIR4, 0x0 \n GOTO $-1");


void initAdc()
{
    ADCON2bits.MD = 1; // Accumulate mode
    
    ADCC_DefineSetPoint(0);
    ADCC_SetUpperThreshold(0xffe);
            
    ADCON3bits.ADCALC = 1; // ERR = ADRES - ADSTPT = ADRES - 0 = ADRES
    ADCON3bits.ADTMD = 0b110; // ERR > UTH (upper threshold)
    ADCON3bits.SOI = 1; // Clear GO if error threshold reached
    
    //Use if(ADSTATbits.UTHR) To check if conversion was greater than upper threshold
}

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



void waitForDischarge(void)
{
    __delay_us(200);
    uint16_t lastVal = 0x7777;
    for(int k=0; k < 30; k++)
    {
        ADCON0bits.GO = 1; //Start conversion
        while (ADCON0bits.GO) {} //Wait for conversion done
        uint16_t val = readAdc();

        lastVal = val;

        // Zero reading means discharge done
        if(val == 0)
            break;

        // Allow some residual voltage after waiting about 1 ms
        // This is because large capacitors might not have time to completely discharge
        // Offset compensation should be able to deal with it
        if(k >= 8 && val <= 10) 
            break;

        __delay_us(90);
    }
}



// Turns on current source and samples the analog signal twice in quick succession
// Then turns on discharge mosfet and waits until any analog signal reaches zero (or close to zero)
// The process is repeated "sampleCount" times
struct doubleSampleData fastDoubleSample(uint8_t currentSourcePinMask, uint16_t sampleCount)
{
    struct doubleSampleData res = {0};

    for(uint16_t i = 0; i < sampleCount; i++)
    {
        // Note 0x3e instructions between samples
        TMR1H = 0;
        TMR1L = 0;

        // Begin time critical section
        INTERRUPT_GlobalInterruptDisable();

        // Convert before starting and get the offset
        // For large caps there might not be enough time to discharge completely
        // Also it will sync against ADC clock
        __asm("MOVLB 0x2");
        __asm("BSF ADCON0, 0x0");
        __asm("BTFSC ADCON0, 0x0");
        __asm("GOTO $-1");
        uint16_t startOffset = readAdc();

        IO_RA4_LAT = 0; // Turn off discharge mosfet
        LATA &= ~currentSourcePinMask; // turn on current-source

        __delay_us(2);

    //    ADCON0bits.GO = 1; //Start conversion
        //T1CONbits.TMR1ON = 1; // Turn on timer
    //    while (ADCON0bits.GO) {} //Wait for conversion done
        __asm("MOVLB 0x2");
        __asm("BSF ADCON0, 0x0");
        __asm("MOVLB 0x4");
        __asm("BSF T1CON, 0x0");
        __asm("MOVLB 0x2");
        __asm("BTFSC ADCON0, 0x0");
        __asm("GOTO $-1");
        //ADCON0bits.GO = 1; //Start conversion
        //T1CONbits.TMR1ON = 0; // Turn off timer

        __asm("MOVLB 0x2");
        __asm("BSF ADCON0, 0x0");
        __asm("MOVLB 0x4");
        __asm("BCF T1CON, 0x0");

        // delay 12 instruction, should be enough for AD to have done sample
        // testing shows that just 2 instructions is enough after setting GO=1
        __asm("REPT 12 \n NOP \n ENDM");

        LATA |= currentSourcePinMask; // Turn off current source
        IO_RA4_LAT = 1; // Turn on discharge mosfet

        while (ADCON0bits.GO) {} //Wait for conversion done

        // End time critical section
        INTERRUPT_GlobalInterruptEnable();

        res.firstSum += readAdcPrev();
        uint16_t second = readAdc();

        if(second == 0xfff)
        {
            res.overRange = true;
            waitForDischarge();
            return res;
        }
        
        res.startOffsetSum += startOffset;
        res.secondSum += second;
        res.cyclesSum += (uint16_t)(TMR1H<<8) | TMR1L;

        waitForDischarge();
    }
    return res;
}

/*struct doubleSampleData doubleBurstSampleWithDelay(uint8_t currentSourcePinMask, uint8_t burstLength, uint16_t burstSpacingCycles)
{
    int16_t timerVal = -(int16_t)burstSpacingCycles;
    TMR1H = (timerVal >> 8);
    TMR1L = timerVal;
    struct doubleSampleData res = {0};

    INTERRUPT_GlobalInterruptDisable();
    IO_RA4_LAT = 0; // Turn off discharge mosfet
    LATA &= ~currentSourcePinMask; // turn on current-source
    __delay_us(2);
    startTimer();
    res.firstSum = burstSampleSum(burstLength);
    if(res.firstSum < 0)
    {
        res.overRange = true;
    }
    else
    {
        if(TMR1H < 255)
        {
            // More than 255 clock cycles left, so we can enable interrupts
            INTERRUPT_GlobalInterruptEnable();

            while(TMR1H < 255){}
            // Run the last 256 cycles with interrupts disabled for accurate timing
            INTERRUPT_GlobalInterruptDisable();
        }

        waitForTimerOverFlow();
        res.secondSum = burstSampleSum(burstLength);
        if(res.secondSum < 0)
            res.overRange = true;
    }
    
    stopTimer();
    PIR4bits.TMR1IF = 0; // Clear timer overflow
    LATA |= currentSourcePinMask; // turn off current-source
    IO_RA4_LAT = 1; // Turn on discharge mosfet
    INTERRUPT_GlobalInterruptEnable();
    waitForDischarge();
    return res;
}*/


struct doubleSampleData doubleBurstSample(uint8_t currentSourcePinMask, uint8_t burstLength)
{
    TMR1H = 0;
    TMR1L = 0;
    struct doubleSampleData res = {0};
    int24_t firstSum, secondSum;
    IO_RA4_LAT = 0; // Turn off discharge mosfet
    LATA &= ~currentSourcePinMask; // turn on current-source

    INTERRUPT_GlobalInterruptDisable();
    __delay_us(2);
    startTimer();
    if(burstSampleSum(burstLength, &firstSum))
    {
        stopTimer();
        if(!burstSampleSum(burstLength, &secondSum))
            res.overRange = true;
    }
    else
    {
        stopTimer();
        res.overRange = true;
    }

    LATA |= currentSourcePinMask; // turn off current-source
    IO_RA4_LAT = 1; // Turn on discharge mosfet
    INTERRUPT_GlobalInterruptEnable();
    res.firstSum += firstSum;
    res.secondSum += secondSum;
    displayHex((uint16_t)(TMR1H<<8) | TMR1L);

    waitForDischarge();
    return res;
}

// Samples many times in a row and sums the result. 
// Does calculations while ADC is running to speed up process. 
// Exits early with -1 if 0xfff is sampled
bool burstSampleSum(uint8_t burstLength, int24_t* aggregate)
{
    // qwerty
    ADCON0bits.GO = 1; //Start conversion
    //int24_t aggregate = 0; // Declare and zero variable after conversion starts, faster...
    while(true)
    {
        burstLength --;
        if(burstLength == 0)
        {
            waitForAdc();
            uint16_t sample = readAdc();
            if(sample == 0x1fff)
                return false;
            *aggregate += sample;
            return true;
        }
        _nop(); // Sync with AD
        waitForAdcAndRestart();

        uint16_t sample = readAdc();
        if(sample == 0x1fff)
            return false;
        *aggregate += sample;
    }
}
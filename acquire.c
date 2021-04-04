#include "mcc_generated_files/mcc.h"
#include "acquire.h"
#include "display.h"

#define waitForAdc()  __asm("MOVLB 0x2 \n BTFSC ADCON0, 0x0 \n GOTO $-1");
#define waitForAdcAndRestart()  __asm("MOVLB 0x2 \n BTFSC ADCON0, 0x0 \n GOTO $-1 \n BSF ADCON0, 0x0");
#define startTimer()     __asm("MOVLB 0x4 \n BSF T1CON, 0x0");
#define stopTimer()      __asm("MOVLB 0x4 \n BCF T1CON, 0x0");
#define waitForTimerOverFlow() __asm("MOVLB 0xE \n BTFSS PIR4, 0x0 \n GOTO $-1");

struct doubleSampleData sampleResult;


void initAdc(void)
{
    ADCON2bits.MD = 1; // Accumulate mode
    
    ADCC_DefineSetPoint(0);
    ADCC_SetUpperThreshold(0xfff);
            
    ADCON3bits.ADCALC = 1; // ERR = ADRES - ADSTPT = ADRES
    //ADCON3bits.ADTMD = 0b110; // Stop condition: Stop if ERR > UTH (upper threshold)
    //ADCON3bits.SOI = 1; // Set it to clear GO stop condition reached
    
    //Use if(ADSTATbits.UTHR) To check if conversion was greater than upper threshold
}

int32_t multiSampleAdc2(uint16_t sampleCount)
{
    int32_t sum = 0;
    ADCON0bits.GO = 0; 
    ADCON0bits.CONT = 0;
    
    //INTERRUPT_GlobalInterruptDisable();
    for(int i=sampleCount; i > 0; i-=16)
    {
         // Clear ACC, OV, CNT
        //ADCON2bits.ACLR = 1;
        //while(ADCON2bits.ACLR){};
        
        ADACCU = 0; 
        ADACCH = 0; 
        ADACCL = 0;
        
        uint8_t repeat = i > 16 ? 16 : (uint8_t)i;
        ADCNT = 128-1;//repeat;

        for(uint8_t j=0; j < repeat; j++)
        {
            ADCON0bits.GO = 1; 

            while(ADCON0bits.GO){}
            if(ADSTATbits.UTHR) // Was there a 0xfff reading? 
                return -1;

        }
        uint24_t adcSum =  ADCC_GetAccumulatorValue(); //
        //ADCON0bits.GO = 0; 
        
        
        
        sum += adcSum;
        
    }
    //INTERRUPT_GlobalInterruptEnable();
    return sum;
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
        res.instructionsDelta = TMR1_ReadTimer();

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

// Sample "burstLenght" times and put sum in firstSum
// Then sample "burstLenght" times and put sum in secondSum
// NOP instructions are for timing, each sample should be equally spaced with 58 instructions between
struct doubleSampleData sampleSlope(uint8_t burstLength, uint8_t currentSourcePinMask)
{
    stopTimer();
    TMR1H = 0;
    TMR1L = 0;
    burstLength --;
    uint16_t sample;
    struct doubleSampleData res = {0};
    int24_t firstSum, secondSum;
    IO_RA4_LAT = 0; // Turn off discharge mosfet
    LATA &= ~currentSourcePinMask; // turn on current-source

    INTERRUPT_GlobalInterruptDisable();
    __delay_us(2);
    ADCON0bits.GO = 1; //Start conversion
    
    //startTimer();
    _nop(); _nop();
    if(burstLength > 0)
    {
        uint8_t x = burstLength;
        _nop();
        while(true)
        {
            waitForAdcAndRestart();

            sample = readAdc();
            if(ADSTATbits.UTHR) // Was greater than upper threshold ?if(sample == 0x1fff)
                goto overrange;
            firstSum += sample;
            _nop();
            x--;
            if(x == 0)
                break;
        }
    }
    _nop();
    _nop();
    waitForAdcAndRestart();
    startTimer();
    sample = readAdc();
    if(ADSTATbits.UTHR) // Was greater than upper threshold ?if(sample == 0x0fff)
        goto overrange;
    firstSum += sample;

    if(burstLength > 0)
    {
        uint8_t x = burstLength;
        _nop();
        while(true)
        {
            waitForAdcAndRestart();
            sample = readAdc();
            if(ADSTATbits.UTHR) // Was greater than upper threshold ?if(sample == 0x1fff)
                goto overrange;
            secondSum += sample;
            _nop();
            x--;
            if(x == 0)
                break;
        }
    }
    _nop();
    waitForAdc();
    _nop();
    stopTimer();
    sample = readAdc();
    if(ADSTATbits.UTHR) // Was greater than upper threshold ?if(sample == 0x0fff)
        goto overrange;
    secondSum += sample;

    LATA |= currentSourcePinMask; // turn off current-source
    IO_RA4_LAT = 1; // Turn on discharge mosfet    
    INTERRUPT_GlobalInterruptEnable();
    res.overRange = false;
    res.firstSum = firstSum;
    res.secondSum = secondSum;
    res.instructionsDelta = TMR1_ReadTimer() - 2;
    return res;
overrange:
    LATA |= currentSourcePinMask; // turn off current-source
    IO_RA4_LAT = 1; // Turn on discharge mosfet    
    stopTimer();
    INTERRUPT_GlobalInterruptEnable();
    res.overRange = true;
    return res;
}
#include "mcc_generated_files/mcc.h"
#include "display.h"
#include "esr.h"

#define readAdc() ((uint16_t)((ADRESH << 8) + ADRESL));
#define readAdcPrev() ((uint16_t)((ADPREVH << 8) + ADPREVL));


struct sampleData
{
    uint16_t startOffset;
    uint16_t first;
    uint16_t second;
    uint16_t cycles;
};

struct sampleData takeRawESRMeasurement(uint8_t currentSourcePinMask, uint8_t k)
{
    // Note 0x3e instructions between samples
    TMR1H = 0;
    TMR1L = 0;
    
    struct sampleData res;

    // Begin time critical section
    INTERRUPT_GlobalInterruptDisable();
    
    // Convert before starting and get the offset
    // For large caps there might not be enough time to discharge completely
    // Also it will sync against ADC clock
    __asm("MOVLB 0x2");
    __asm("BSF ADCON0, 0x0");
    __asm("BTFSC ADCON0, 0x0");
    __asm("GOTO $-1");
    res.startOffset = readAdc();
    
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


    res.first = readAdcPrev();
    res.second = readAdc();
    res.cycles = (uint16_t)(TMR1H<<8) | TMR1L;
    
//    uint16_t timerVal = (uint16_t)(TMR1H<<8) | TMR1L;
//    displayHex(timerVal);
    
    __delay_us(200);
    uint16_t lastVal = 0x7777;
    for(int i=0; i < 30; i++)
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
        if(i >= 8 && val <= 10) 
            break;
        
        __delay_us(90);
    }
    //displayHex(lastVal);
    return res;
}

void readEsr(void)
{
    while(true)
    {
        for(int k=0; k < 16; k++)
        {
        
        for(int x=0; x < 300; x++)    
        {
        
        int32_t offsetSum = 0;
        int32_t firstSum = 0;
        int32_t secondSum = 0;
        int32_t cycleSum = 0;

        bool lowRange = false;
        uint8_t pinMask, pointPos;
        // float calMultiplier;

        if (lowRange)
        {
            pinMask = _LATA_LATA1_MASK;
            pointPos = 1;
        }
        else
        {
            pinMask = _LATA_LATA0_MASK;
            pointPos = 0;
        }
        
        for(int i=0; i < 128; i++)
        {
            struct sampleData data = takeRawESRMeasurement(pinMask, k);

            /*if(pair.second == 0xfff)
            {
                clearDisplay();
                setSevenSegDots(1); // Used as power-on led
                goto outer;
            }*/

            offsetSum += data.startOffset;
            firstSum += data.first;
            secondSum += data.second;
            cycleSum += data.cycles;
        }
        
        // 7.5 us between samples 
        // estimated 3 us delay from start
        int32_t diff = secondSum - firstSum;
        
        int32_t val = firstSum - (diff*5)/15 - offsetSum;// (diff>>2) - (diff>>4);
        //int32_t val = secondSum;
        
        if(lowRange)
        {
            // 1 count on ADC = 0.25 ohm
            // 2.5x = 5x/2
            val *= 5;
            val >>= 1+7;
        }
        else
        {
            // 1 count on ADC =  0.0125 ohm
            // 1.25x = 5x/4
            val *= 5; 
            val >>= 2+7;
        }

        val -= 73;// temp zeroing
        
        if(val >= 10000)
        {
            val /= 10;
            pointPos++;
        }

        displayDecimal((int16_t)val, pointPos);
    
        //displayHex(offsetSum>>7);
        
/*        displayHex(
                ((firstSum - (diff>>2) - (diff>>3)) >> 7)
                //(cycleSum>>7) + 
                  );/*/
        }
        }
//outer:
        //__delay_ms(1);
    }
}
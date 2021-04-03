/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.6
        Device            :  PIC16F18446
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"
#include "display.h"
#include "esr.h"
#include "acquire.h"
#include "resistance.h"
#include "capacitance.h"

void interruptHandler(void)
{
    runDisplayTick();
}

const double _batVoltageCal = 4.785/4.81;


// Reads battery voltage in 10s of millivolts
uint16_t readBatteryVoltage(double calibrationFactor)
{
    IO_RA4_SetDigitalInput();
    IO_RA4_SetAnalogMode();
    ADPCH = 0x04;  // Set ADC input to RA4 pin
    __delay_ms(1); // Wait for things to settle
    
    int32_t adcAvg = multiSampleAdc(256);
    
    adcAvg *=3; // Compensate for 1/3 voltage divider
    adcAvg = (uint32_t)(adcAvg * calibrationFactor) >> 8;
    
    uint16_t voltage = (uint16_t)adcAvg;
    
    // Round and make it 10s of mV
    voltage += 5;
    voltage /= 10;

    return voltage;
}

void initMeasurements(void)
{
    ADPCH = 5; // Set ADC channel to RA5

    // Setup discharge mosfet pin
    IO_RA4_SetDigitalMode();
    IO_RA4_SetDigitalOutput();
    IO_RA4_SetPushPull();
    
    IO_RA4_SetHigh();
}


void testSignal(void)
{
    INTERRUPT_GlobalInterruptDisable();

    while(true)
    {
        IO_RA4_LAT = 0;
        IO_RA0_LAT = 0;

        __delay_us(20);

        IO_RA0_LAT = 1;
        IO_RA4_LAT = 1;

        __delay_us(180);
    }
}

int32_t lowOhmsZeroOffset = 0, highOhmsZeroOffset = 0;
int32_t lowEsrZeroOffset = 0, highEsrZeroOffset = 0;

void ZeroMeter(void)
{
    int32_t lowZero = takeRawOhmsMeasurement(_LATA_LATA0_MASK, 1024);
    int32_t highZero = takeRawOhmsMeasurement(_LATA_LATA1_MASK, 1024);
    
    struct doubleSampleData lowEsrZero = fastDoubleSample(_LATA_LATA0_MASK, 256);
    struct doubleSampleData highEsrZero = fastDoubleSample(_LATA_LATA1_MASK, 256);
    
    if(lowZero < 0 || highZero < 0)
    {
        lowOhmsZeroOffset = 0;
        highOhmsZeroOffset = 0;
        lowEsrZeroOffset = 0;
        highEsrZeroOffset = 0;
        return;
    }
    // Add a bit of rounding (-0.25) to prevent some flipping between 0 and -1
    lowOhmsZeroOffset = (lowZero >> 2) - 64;
    highOhmsZeroOffset = (highZero >> 2) - 64;
    
    lowEsrZeroOffset = (lowEsrZero.firstSum >> 1) - 32;
    highEsrZeroOffset = (highEsrZero.firstSum >> 1) - 32;
}


void adcc()
{
    IO_RA4_LAT = 0; // Turn off discharge mosfet
    LATA1 = 0; // turn on weak current-source

    __delay_us(2);
    
    // Clear ACC, OV, CNT
    ADCON2bits.ACLR = 1;
    while(ADCON2bits.ACLR){};
    
    ADCON2bits.MD = 1; // Accumulate mode
    
    // Set upper threshold = 0xffe
    ADUTHL = 0xfe;
    ADUTHH = 0x0f;
    // Setpoint = 0
    ADSTPTH = 0x00;
    ADSTPTL = 0x00;
    
    ADCON3bits.ADCALC = 1; // ERR = ADRES - ADSTPT = ADRES
    ADCON3bits.ADTMD = 0b110; // Stop condition: Stop if ERR > UTH (upper threshold)
    ADCON3bits.SOI = 1; // Set it to clear GO stop condition reached
    
    ADCON0bits.CONT = 1;
        T1CONbits.T1RD16 = 1;
    __asm("MOVLB 0x4");
    __asm("BCF T1CON, 0x0");
    TMR1H = 0;
    TMR1L = 0;
    ADCNT = 128-4;
    ADCON0bits.GO = 1; 
    __asm("MOVLB 0x4");
    __asm("BSF T1CON, 0x0");

    while((int8_t)ADCNT >= 0 && ADCON0bits.GO){} // Wait for conversion counter to reach 128
    __asm("MOVLB 0x4");
    __asm("BCF T1CON, 0x0");
    
    ADCON0bits.GO = 0; 
    
    uint16_t err = ((uint16_t)ADERRH << 8) | ADERRL;
    
    if(ADSTATbits.UTHR) // Was greater than upper threshold ?
        displayHex((ADACCU <<8) | ADACCH );
    uint8_t lo = TMR1L;
    uint16_t tim = lo + (TMR1H<<8);
    displayHex(tim);
    displayHex(TMR1_ReadTimer());


    LATA1 = 1; // turn off weak current-source

    // 1 -> 3d
    // 2 -> 75
    // 3 -> ad
    // 4 -> e5
    // 0x38 (56) cycles per conversion, 7us
}

/*
                         Main application
 */
void main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    
    // Wait for fixed voltage reference
    while(!FVR_IsOutputReady());
    
    TMR0_SetInterruptHandler(interruptHandler);

    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    TMR0_StartTimer();

    displayText('B', 'A', 'T' | 0x80, ' ');
    __delay_ms(800);
        
        //setSevenSegDots( (readButton1() ? 1 : 0) + (readButton2() ? 2 : 0));
    
    for(int i=0; i < 12; i++)
    {
        displayDecimal(readBatteryVoltage(_batVoltageCal), 1);
        __delay_ms(150);
    }
     
    
    initMeasurements();
    /*clearDisplay();

    while(1)
    {
        //setSevenSegDots( (readButton1() ? 1 : 0) + (readButton2() ? 2 : 0) + 4);
        setSevenSegDots(1); // Used as power-on led
        __delay_ms(50);
    }*/
    
    
    int mode = 0;
    // Main loop
    while(true)
    {
        break;
        if(readRightButton())
        {
            displayText('Z', 'E', 'R', 'O' | 0x80);
            while(readRightButton());
            
            __delay_ms(500);
            
            ZeroMeter();
        }
        
        if(readLeftButton())
        {
            // Go to next mode
            mode++;
            if(mode == 3)
                mode = 0;
            
            switch(mode)
            {
                case 0: 
                    displayText('E', 'S', 'R' | 0x80 , 0);
                    break;
                case 1:
                    displayText('R', 'E', 'S' | 0x80 , 0);
                    break;
                case 2:
                    displayText('C', 'A', 'P' | 0x80 , 0);
                    break;
            }


            while(readLeftButton());
            __delay_ms(300);
            
            // Delay while checking button
            for(int i=0; i < 700; i++)
            {    
                if(readLeftButton())
                    break;
                __delay_ms(1);
            }
        }
        
        switch(mode)
        {
            case 0: 
                readEsr(lowEsrZeroOffset, highEsrZeroOffset);
                break;
                
            case 1: 
                readOhms(lowOhmsZeroOffset, highOhmsZeroOffset);
                break;
                
            case 2:
                measureCapacitance();
                break;
        }
    }
    
    //readEsr();
    //testSignal();
    //burstSampleSum(4);
    //doubleBurstSample(0, 0);
        INTERRUPT_GlobalInterruptDisable();

    
    __asm("MOVLB 0x4");
    __asm("BCF T1CON, 0x0");
    TMR1H = 0;
    TMR1L = 0;
    __asm("MOVLB 0x4");
    __asm("BSF T1CON, 0x0");
    ADCON2bits.ACLR = 1;
    __asm("MOVLB 0x4");
    while(ADCON2bits.ACLR){};
    __asm("BCF T1CON, 0x0");
    
   uint16_t tim = TMR1L + (TMR1H<<8);
   displayHex(TMR1_ReadTimer());


    
    adcc();
    INTERRUPT_GlobalInterruptEnable();
    while(true) {};
}
/**
 End of File
*/
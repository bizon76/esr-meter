#include "mcc_generated_files/mcc.h"
#include "display.h"
#include "esr.h"
#include "acquire.h"
#include "resistance.h"
#include "capacitance.h"
#include "dual.h"

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

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    TMR0_StartTimer();

    displayText('B', 'A', 'T' | 0x80, ' ');
    __delay_ms(800);
        
    initAdc();
    for(int i=0; i < 12; i++)
    {
        displayDecimal(readBatteryVoltage(_batVoltageCal), 1);
        __delay_ms(155);
    }
     
    
    initMeasurements();
    
    int mode = 0;
    // Main loop
    while(true)
    {
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
            if(mode == 4)
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
                case 3:
                    displayText('D', 'U', 'A', 'L');
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
                
            case 3:
                dualMeasureCapAndESR(lowEsrZeroOffset, highEsrZeroOffset);
                break;
        }
    }
}
/**
 End of File
*/
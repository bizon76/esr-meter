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

#define readAdc() ((uint16_t)((ADRESH << 8) + ADRESL));

void interruptHandler(void){
    // add your TMR0 interrupt custom code
    // or set custom function using TMR0_SetInterruptHandler()
    
    runDisplayTick();
}

const double _batVoltageCal = 4.785/4.81;


void readBatteryVoltage()
{
    IO_RA4_SetDigitalInput();
    IO_RA4_SetAnalogMode();
    ADPCH = 0x04;           //Set ADC input to RA4 pin
    __delay_ms(1); // Wait for things to settle
    
    uint32_t adcAvg = 0;
    
    const int readings = 256;
    for(int i = 0; i < readings; i++)
    {
        ADCON0bits.GO = 1; //Start conversion      
        while (ADCON0bits.GO) {} //Wait for conversion done

        uint16_t adcResult = readAdc();//((uint16_t)((ADRESH << 8) + ADRESL));
        adcAvg += adcResult;
    }
    adcAvg *=3; // real voltage is 3 times higher
    adcAvg /= readings;
    
    adcAvg = (uint32_t)(adcAvg * _batVoltageCal);
    
    uint16_t milliVolts = (uint16_t)adcAvg;
    
    // Round and make it 3 digits range
    milliVolts += 5;
    milliVolts /= 10;
    
    displayDecimal(milliVolts, 1);
}

void initMeasurements(void)
{
    ADPCH = 5; // Set adc channel to RA5

    // Setup discharge mosfet pin
    IO_RA4_SetDigitalMode();
    IO_RA4_SetDigitalOutput();
    IO_RA4_SetPushPull();
    
    IO_RA4_SetHigh();
}

// Takes [sampleCount] readings of adc in quick succession and returns summed value
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
        {
            return -1;
        }
        sum += adcResult;
    }
    return sum;
}

int32_t takeOhmsMeasurement(void)
{
    IO_RA4_LAT = 0; // Turn off discharge mosfet
    IO_RA1_LAT = 0; // turn on medium current-source

    __delay_ms(1); // Wait for current to stabilise (could be an inductor)

    int32_t sum = multiSampleAdc(64);
    
    if(sum < 0)
    {
        IO_RA1_LAT = 1; // turn off medium current-source
        IO_RA4_LAT = 1; // Turn on discharge mosfet
        return -1;
    }

    IO_RA1_LAT = 1; // turn off medium current-source
    __delay_ms(3);
    IO_RA4_LAT = 1; // Turn on discharge mosfet
    __delay_ms(1);

    // 1 count on ADC =  0.0125 ohm
    // 1.25x = 5x/4
    sum *= 5; 
    sum >>= 2+6;

    return sum;
}


void readOhms(void)
{
    while(true)
    {
        int32_t val = takeOhmsMeasurement();
        if(val < 0)
        {
            clearDisplay();
            setSevenSegDots(1); // Used as power-on led
            __delay_ms(50);
            continue;
        }

        displayDecimal((int16_t)val, 1);

        __delay_ms(50);
    }
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

    //ADCC_GetSingleConversion(RA5);
    
    TMR0_SetInterruptHandler(interruptHandler);

    
    
    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    TMR0_StartTimer();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();

    
    /*while (1)
    {
        //IO_RC5_Toggle();
        //LATC++;
        for(int i=0; i < 16; i++)
        {
            for(int k = 0; k < 100; k++)
            for(int d=0; d < 4; d++)
            {
                
                selectDigit(-1); // Clear digit when changing
                //output7Seg(digitTo7Seg( (i + d) & 0x0f));
                output7Seg(sevenSegData[d]);
                selectDigit(d);
                __delay_us(400);
            }
        }
        // Add your application code
    }*/
       
    displayText('B', 'A', 'T' | 0x80, ' ');
    __delay_ms(800);
        
        //setSevenSegDots( (readButton1() ? 1 : 0) + (readButton2() ? 2 : 0));
    
    for(int i=0; i < 10; i++)
    {
        readBatteryVoltage();
        __delay_ms(150);
    }
     
       
    /*while(true)
    {
    int16_t vals[] = { 0, 1, 12, 103, 5093 };

    for(int j=0; j < 5; j++)
    {
        for (uint8_t dp = 0; dp <= 3; dp++)
        {
            displayDecimal(vals[j], dp);
            __delay_ms(1000);
            displayDecimal(-vals[j], dp);
            __delay_ms(1000);
        }
    }
    }*/
    
    
    initMeasurements();
    /*clearDisplay();

    while(1)
    {
        //setSevenSegDots( (readButton1() ? 1 : 0) + (readButton2() ? 2 : 0) + 4);
        setSevenSegDots(1); // Used as power-on led
        __delay_ms(50);
    }*/
    
    readOhms();
}
/**
 End of File
*/
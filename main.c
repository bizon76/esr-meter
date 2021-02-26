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

#define byte unsigned char



// Map segments to ports, a = bit 0, b = bit 1, ...
void output7Seg(byte segs)
{
    PORTC = 0xff; // Set all high
    IO_RC0_TRIS = (segs & 0x01) == 0;
    IO_RC1_TRIS = (segs & 0x02) == 0;
    IO_RC2_TRIS = (segs & 0x04) == 0;
    IO_RC3_TRIS = (segs & 0x08) == 0;
    IO_RC4_TRIS = (segs & 0x10) == 0;
    IO_RC5_TRIS = (segs & 0x20) == 0;
    IO_RC6_TRIS = (segs & 0x40) == 0;
    IO_RC7_TRIS = (segs & 0x80) == 0;
}

void selectDigit(int digit)
{
    PORTB = 0; // Set all low
    TRISB = 0xff; // Set all to input first
    IO_RA2_PORT = 0;
    IO_RA2_TRIS = 1;
    if(digit < 0)
        return;
    IO_RB4_TRIS = digit != 0;
    IO_RB5_TRIS = digit != 1;
    IO_RB6_TRIS = digit != 2;
    IO_RB7_TRIS = digit != 3;
    IO_RA2_TRIS = digit != 4;
}

const byte digitTable[] = {
    0b00111111, //0
    0b00000110, //1
    0b01011011, //2
    0b01001111, //3
    0b01100110, //4
    0b01101101, //5
    0b01111101, //6
    0b00000111, //7
    0b01111111, //8
    0b01101111, //9
    0b01110111, //A
    0b01111100, //b
    0b00111001, //C
    0b01011110, //d
    0b01111001, //E
    0b01110001, //F
};

byte digitTo7Seg(int digit)
{
    if(digit > 15 || digit < 0)
        return 0x80; // dp
    return digitTable[digit];
}



 byte sevenSegData[] = {0,0,0,0};
 byte sevenSegDots = 0;
 byte digits[] = {0,0,0,0};
 int currentDigit = 0;
 bool button1_down = false, button2_down = false;

void readButtonsStart()
{
    // Set weak pull ups on tristate for RC3,RC4
    IO_RC3_WPU = 1;
    IO_RC4_WPU = 1;
}

void readButtonsEnd()
{
    button1_down = !IO_RC3_PORT;
    button2_down = !IO_RC4_PORT;
    // Remove weak pull ups on tristate for RC3,RC4
    IO_RC3_WPU = 0;
    IO_RC4_WPU = 0;
}

 
 void runDisplayTick()
 {
    if(currentDigit == 0)
        readButtonsEnd();

    selectDigit(-1); // Clear digit when changing
    //output7Seg(digitTo7Seg( (i + d) & 0x0f));
    if(currentDigit < 4)
    {
        output7Seg(sevenSegData[currentDigit]);
    }
    else 
    {
        output7Seg(sevenSegDots & 0x7); // only first 3 bits connected
        readButtonsStart();
    }
    selectDigit(currentDigit);
    
    currentDigit ++;
    if(currentDigit == 5)
        currentDigit = 0;
 }
 
void interruptHandler(void){
    // add your TMR0 interrupt custom code
    // or set custom function using TMR0_SetInterruptHandler()
    
    runDisplayTick();
}


void ReadBatteryVoltage()
{
    IO_RA4_TRIS = 1;
    IO_RA4_SetAnalogMode();
    
    //////
    ADPCH = 0x04;  
    __delay_ms(1);
    
    
    // Conversion finished, return the result
    
//Setup ADC                        
    ADCON0bits.FM = 1;      //right justify   
    ADCON0bits.CS = 1;      //FRC Clock           
    ADPCH = 0x04;           //RA0 is Analog channel            
    TRISAbits.TRISA4 = 1;   //Set RA0 to input        
    ANSELAbits.ANSA4 = 1; //Set RA0 to analog          
    //ADCON0bits.ON = 1;      //Turn ADC On     
    //__delay_ms(1);
    ADCON0bits.GO = 1;     //Start conversion      
    while (ADCON0bits.GO); //Wait for conversion done

    /////
    uint16_t adcResult = ((uint16_t)((ADRESH << 8) + ADRESL));
    
    uint16_t milliVolts = adcResult * 3; // real voltage is 3 times higher
    milliVolts += 5;
    milliVolts /= 10;
    for(int i = 3; i >= 0; i--)
    {
        byte digit = (byte)(milliVolts % 10);
        milliVolts /= 10;
        byte sevenSeg = digitTo7Seg(digit);
        if(i == 1)
            sevenSeg |= 0x80; // Add decimal point
        if(i == 0 && digit == 0)
            sevenSeg = 0;
        sevenSegData[i] = sevenSeg;
    }
    
    __delay_ms(200);
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
       
        
    while(1)
    {
        sevenSegDots = (button1_down ? 1 : 0) + (button2_down ? 2 : 0);
        ReadBatteryVoltage();
    }
    /*while(1)
    {
        for(int a = 0; a < 16; a++)
        {
            sevenSegData[0] = digitTo7Seg(a);
            sevenSegData[1] = digitTo7Seg( (a + 10) & 0x0f );
            sevenSegData[2] = digitTo7Seg( a ^ 9 );
            sevenSegData[3] = digitTo7Seg( a ^ 1 );
            sevenSegDots = (button1_down ? 1 : 0) + (button2_down ? 2 : 0);
            __delay_ms(400);
        }
    }*/
}
/**
 End of File
*/
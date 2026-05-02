/*
;---------------------------------------------------------------
; Title: ADC_LCD_Accelerometer_With_IOC_Interrupt
;---------------------------------------------------------------
; Program Details:
;   This program uses a PIC18F47K42 to read an analog accelerometer
;   and display the motion/orientation result on a 16x2 LCD.
;
;   The LCD is connected in 8-bit mode using PORTB for data and
;   RD0/RD1 for control. The accelerometer X, Y, and Z outputs are
;   connected to RA0, RA1, and RA2 as analog ADC inputs.
;
;   The program displays one of four states on the LCD:
;   Shake, Tilt_Left, Tilt_Right, or Flat. The second LCD line
;   displays the positive X-axis acceleration in m/s2.
;
;   RC2 is used as an interrupt-on-change button input. When the
;   button is pressed, the program enters a HALT state. During HALT,
;   the red LED on RC3 blinks and the LCD displays a 10-second
;   countdown. When the countdown reaches zero, the PIC resets.
;
; Inputs:
;   - RA0 : Accelerometer X-OUT analog input
;   - RA1 : Accelerometer Y-OUT analog input
;   - RA2 : Accelerometer Z-OUT analog input
;   - RC2 : IOC interrupt button input
;
; Outputs:
;   - PORTB : LCD data bus D0-D7
;   - RD0   : LCD RS control pin
;   - RD1   : LCD EN control pin
;   - RC3   : Red LED halt indicator
;
; Setup:
;   - PIC18F47K42
;   - 16x2 LCD connected in 8-bit mode
;   - Analog accelerometer connected to RA0, RA1, and RA2
;   - Button connected from RC2 to GND
;   - Red LED connected to RC3 through a resistor
;
; Date:
;   April 27, 2026
;
; File Dependencies / Libraries:
;   - xc.h
;   - pic18f47k42.h
;   - stdio.h
;   - string.h
;   - stdlib.h
;
; Compiler:
;   XC8 v2.40
;
; Author:
;   Antonio Kassis
;
; Versions:
;   V1.0 : Original version
;---------------------------------------------------------------
*/

#include <xc.h>
#include "pic18f47k42.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ---------------------------------------------------------------
// Configuration Bits
// ---------------------------------------------------------------
#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_1MHZ
#pragma config CLKOUTEN = OFF
#pragma config PR1WAY = ON
#pragma config CSWEN = ON
#pragma config FCMEN = OFF
#pragma config MCLRE = EXTMCLR
#pragma config PWRTS = PWRT_OFF
#pragma config MVECEN = ON
#pragma config IVT1WAY = ON
#pragma config LPBOREN = OFF
#pragma config BOREN = SBORDIS
#pragma config BORV = VBOR_2P45
#pragma config ZCD = OFF
#pragma config PPS1WAY = ON
#pragma config STVREN = ON
#pragma config DEBUG = OFF
#pragma config XINST = OFF
#pragma config WDTCPS = WDTCPS_31
#pragma config WDTE = OFF
#pragma config WDTCWS = WDTCWS_7
#pragma config WDTCCS = SC
#pragma config BBSIZE = BBSIZE_512
#pragma config BBEN = OFF
#pragma config SAFEN = OFF
#pragma config WRTAPP = OFF
#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF
#pragma config WRTSAF = OFF
#pragma config LVP = ON
#pragma config CP = OFF

// ---------------------------------------------------------------
// Constants and Pin Definitions
// ---------------------------------------------------------------
#define _XTAL_FREQ 1000000UL

#define VREF_MV 3300
#define ZERO_MV 1650
#define SENS_MV 330
#define SHAKE_LIMIT 140
#define TILT_LIMIT 35

#define LCD_RS LATDbits.LATD0
#define LCD_EN LATDbits.LATD1
#define LCD_PORT LATB

#define RED_LED LATCbits.LATC3

// ---------------------------------------------------------------
// Global Variables
// ---------------------------------------------------------------
volatile unsigned char stopFlag = 0;
unsigned int xZero = 0;

// ---------------------------------------------------------------
// Function Prototypes
// ---------------------------------------------------------------
void lcdInit(void);
void lcdCmd(unsigned char cmd);
void lcdChar(unsigned char dat);
void lcdText(const char *msg);
void lcdTextAt(unsigned char row, unsigned char pos, const char *msg);
void lcdClear(void);

void adcInit(void);
unsigned int readADC(unsigned char channel);
unsigned int adcToMv(unsigned int adc);
signed int mvToG(unsigned int mv);
signed int gToMs2(signed int gVal);
void formatNum(char *buffer, signed int value);

void iocInit(void);
void haltReset(void);

// ---------------------------------------------------------------
// Interrupt Service Routine
// RC2 button interrupt sets stopFlag.
// ---------------------------------------------------------------
void __interrupt(irq(IRQ_IOC), base(0x4008)) IOC_ISR(void)
{
    if(IOCCFbits.IOCCF2 == 1)
    {
        stopFlag = 1;
        IOCCFbits.IOCCF2 = 0;
    }

    PIR0bits.IOCIF = 0;
}

// ---------------------------------------------------------------
// Main Program
// ---------------------------------------------------------------
void main(void)
{
    unsigned int xAdc, yAdc, zAdc;
    unsigned int xMv, yMv, zMv;
    unsigned int oldX = 0, oldY = 0, oldZ = 0;

    signed int xG;
    signed int yG;
    signed int zG;
    signed int xMs2;

    unsigned int moveAmount;
    char topLine[17];
    char botLine[17];
    char number[10];

    // Configure LCD ports as digital outputs.
    ANSELB = 0x00;
    ANSELD = 0x00;
    ANSELC = 0x00;

    TRISB = 0x00;
    TRISD = 0x00;

    // Configure interrupt button and halt LED.
    TRISCbits.TRISC2 = 1;
    TRISCbits.TRISC3 = 0;

    // Clear all output latches.
    LATB = 0x00;
    LATD = 0x00;
    LATCbits.LATC3 = 0;

    // Initialize peripherals.
    lcdInit();
    adcInit();
    iocInit();

    lcdClear();

    // Main loop reads accelerometer and updates LCD.
    while(1)
    {
        // If button interrupt occurred, enter halt state.
        if(stopFlag == 1)
        {
            haltReset();
        }

        // Read X, Y, and Z accelerometer ADC values.
        xAdc = readADC(0x00);
        yAdc = readADC(0x01);
        zAdc = readADC(0x02);

        // Convert ADC values to millivolts.
        xMv = adcToMv(xAdc);
        yMv = adcToMv(yAdc);
        zMv = adcToMv(zAdc);

        // Convert voltage readings to acceleration estimates.
        xG = (signed int)(((signed int)xMv - (signed int)xZero) * 100 / SENS_MV);
        yG = mvToG(yMv);
        zG = mvToG(zMv);

        // Display only positive acceleration.
        xMs2 = abs(gToMs2(xG));

        // Detect shaking by comparing current ADC readings to old readings.
        moveAmount = abs((int)xAdc - (int)oldX)
                   + abs((int)yAdc - (int)oldY)
                   + abs((int)zAdc - (int)oldZ);

        oldX = xAdc;
        oldY = yAdc;
        oldZ = zAdc;

        // Decide accelerometer state.
        if(moveAmount > SHAKE_LIMIT)
            strcpy(topLine, "Shake           ");
        else if(xG > TILT_LIMIT)
            strcpy(topLine, "Tilt_Left       ");
        else if(xG < -TILT_LIMIT)
            strcpy(topLine, "Tilt_Right      ");
        else
            strcpy(topLine, "Flat            ");

        // Build second LCD line.
        strcpy(botLine, "X:");
        formatNum(number, xMs2);
        strcat(botLine, number);
        strcat(botLine, " m/s2   ");

        // Print LCD output.
        lcdTextAt(1, 0, topLine);
        lcdTextAt(2, 0, botLine);

        __delay_ms(300);
    }
}

// ---------------------------------------------------------------
// IOC Initialization
// RC2 is the interrupt button. RC3 is the red LED.
// ---------------------------------------------------------------
void iocInit(void)
{
    // RC2 button input with weak pull-up.
    TRISCbits.TRISC2 = 1;
    ANSELCbits.ANSELC2 = 0;
    WPUCbits.WPUC2 = 1;

    // RC3 red LED output.
    TRISCbits.TRISC3 = 0;
    ANSELCbits.ANSELC3 = 0;
    LATCbits.LATC3 = 0;

    // Interrupt-on-change for RC2 falling edge.
    IOCCPbits.IOCCP2 = 0;
    IOCCNbits.IOCCN2 = 1;
    IOCCFbits.IOCCF2 = 0;

    // Enable IOC interrupt.
    PIR0bits.IOCIF = 0;
    PIE0bits.IOCIE = 1;

    // Enable global interrupts.
    INTCON0bits.IPEN = 1;
    INTCON0bits.GIEH = 1;
    INTCON0bits.GIEL = 1;
}

// ---------------------------------------------------------------
// Halt State
// Blinks LED, displays countdown, then resets PIC.
// ---------------------------------------------------------------
void haltReset(void)
{
    unsigned char sec;
    unsigned char i;
    char msg[17];

    lcdClear();
    lcdTextAt(1, 0, "HALT State      ");

    // Count down from 10 seconds.
    for(sec = 10; sec > 0; sec--)
    {
        sprintf(msg, "Reset in %2u sec ", sec);
        lcdTextAt(2, 0, msg);

        // Blink LED for one second total.
        for(i = 0; i < 2; i++)
        {
            RED_LED = 1;
            __delay_ms(250);

            RED_LED = 0;
            __delay_ms(250);
        }
    }

    // Show zero before reset.
    lcdTextAt(2, 0, "Reset in  0 sec ");
    __delay_ms(300);

    RED_LED = 0;

    RESET();
}

// ---------------------------------------------------------------
// ADC Initialization
// RA0, RA1, and RA2 are analog inputs.
// ---------------------------------------------------------------
void adcInit(void)
{
    // Set accelerometer pins as inputs.
    TRISAbits.TRISA0 = 1;
    TRISAbits.TRISA1 = 1;
    TRISAbits.TRISA2 = 1;

    // Enable analog function on RA0, RA1, and RA2.
    ANSELAbits.ANSELA0 = 1;
    ANSELAbits.ANSELA1 = 1;
    ANSELAbits.ANSELA2 = 1;

    // ADC voltage references are VDD and VSS.
    ADREFbits.NREF = 0;
    ADREFbits.PREF = 0;

    // ADC timing setup.
    ADCLK = 0x00;
    ADPREL = 0x00;
    ADPREH = 0x00;
    ADACQL = 0x20;
    ADACQH = 0x00;

    // Clear ADC result registers.
    ADRESH = 0x00;
    ADRESL = 0x00;

    // Turn ADC on.
    ADCON0 = 0xA4;

    // Calibrate X-axis zero position.
    lcdClear();
    lcdTextAt(1, 0, "Calibrating...");
    __delay_ms(1000);

    xZero = adcToMv(readADC(0x00));

    lcdClear();
}

// ---------------------------------------------------------------
// Read ADC Channel
// Selects channel, starts conversion, and returns result.
// ---------------------------------------------------------------
unsigned int readADC(unsigned char channel)
{
    unsigned int result;

    ADPCH = channel;
    __delay_ms(2);

    ADCON0 |= 0x01;
    while(ADCON0 & 0x01);

    result = ((unsigned int)ADRESH << 8) | ADRESL;

    return result;
}

// ---------------------------------------------------------------
// Convert ADC Count to Millivolts
// ---------------------------------------------------------------
unsigned int adcToMv(unsigned int adc)
{
    return (unsigned int)(((unsigned long)adc * VREF_MV) / 4095);
}

// ---------------------------------------------------------------
// Convert Millivolts to Hundredths of g
// ---------------------------------------------------------------
signed int mvToG(unsigned int mv)
{
    signed int diff;

    diff = (signed int)mv - ZERO_MV;

    return (signed int)((diff * 100) / SENS_MV);
}

// ---------------------------------------------------------------
// Convert Hundredths of g to Hundredths of m/s2
// ---------------------------------------------------------------
signed int gToMs2(signed int gVal)
{
    return (signed int)((gVal * 981) / 100);
}

// ---------------------------------------------------------------
// Format Number
// Converts hundredths value into x.xx format.
// ---------------------------------------------------------------
void formatNum(char *buffer, signed int value)
{
    signed int whole;
    signed int decimal;

    if(value < 0)
        value = -value;

    whole = value / 100;
    decimal = value % 100;

    sprintf(buffer, "%d.%02d", whole, decimal);
}

// ---------------------------------------------------------------
// LCD Initialization
// Initializes 16x2 LCD in 8-bit mode.
// ---------------------------------------------------------------
void lcdInit(void)
{
    __delay_ms(500);

    lcdCmd(0x38);
    __delay_ms(20);

    lcdCmd(0x38);
    __delay_ms(20);

    lcdCmd(0x38);
    __delay_ms(20);

    lcdCmd(0x0C);
    __delay_ms(20);

    lcdCmd(0x06);
    __delay_ms(20);

    lcdCmd(0x01);
    __delay_ms(20);
}

// ---------------------------------------------------------------
// Clear LCD
// ---------------------------------------------------------------
void lcdClear(void)
{
    lcdCmd(0x01);
    __delay_ms(20);
}

// ---------------------------------------------------------------
// Send LCD Command
// ---------------------------------------------------------------
void lcdCmd(unsigned char cmd)
{
    LCD_PORT = cmd;

    LCD_RS = 0;
    __delay_ms(5);

    LCD_EN = 1;
    __delay_ms(5);
    LCD_EN = 0;

    __delay_ms(5);
}

// ---------------------------------------------------------------
// Send LCD Character
// ---------------------------------------------------------------
void lcdChar(unsigned char dat)
{
    LCD_PORT = dat;

    LCD_RS = 1;
    __delay_ms(5);

    LCD_EN = 1;
    __delay_ms(5);
    LCD_EN = 0;

    __delay_ms(5);
}

// ---------------------------------------------------------------
// Print String on LCD
// ---------------------------------------------------------------
void lcdText(const char *msg)
{
    while(*msg != '\0')
    {
        lcdChar(*msg);
        msg++;
    }
}

// ---------------------------------------------------------------
// Print String at LCD Row and Position
// Row 1 starts at 0x80. Row 2 starts at 0xC0.
// ---------------------------------------------------------------
void lcdTextAt(unsigned char row, unsigned char pos, const char *msg)
{
    unsigned char location;

    if(row == 1)
        location = 0x80 + pos;
    else
        location = 0xC0 + pos;

    lcdCmd(location);
    lcdText(msg);
}
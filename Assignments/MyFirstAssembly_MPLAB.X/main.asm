//-----------------------------
// Title: My First Assembly Program
//-----------------------------
// Purpose: This program measures the temperature from a sensor and references
//	    it with an inputted decimal value from a keypad. The program compares
//	    the reference value with the temperature sensor. If it's greater 
//	    than the reference, the cooling system will turn on. If it's less
//	    than the reference, the heating system will turn on. If it's equal,
//	    than turn off both systems.
// Dependencies: MyConfigFile.inc
// Compiler: v6.30
// Author: Antonio Kassis
// OUTPUTS: The outputs of the program are voltages that turn on LEDs. 
//	    They signify the heating and cooling system.
// INPUTS:  The inputs of the program are decimal values. 
//	    They are connected to the keypad and temperature sensor.
// Versions:
//  	V1.0: 3/9/2026 - First version 
//-----------------------------

#include ".\MyConfigFile.inc"
#include <xc.inc>

;----------------
; PROGRAM INPUTS
;----------------
; The DEFINE directive is used to create macros or symbolic names for values.
; It is more flexible and can be used to define complex expressions or sequences 
; of instructions.
; It is processed by the preprocessor before the assembly begins.

#define  measuredTempInput 	-5 ; this is the input value
#define  refTempInput		15 ; this is the input value

;---------------------
; Definitions
;---------------------
#define SWITCH    LATD,2  
#define LED0      PORTD,0
#define LED1	  PORTD,1
    
 
;---------------------
; Program Constants
;---------------------
;
; The EQU (Equals) directive is used to assign a constant value to a symbolic
; name or label.
; It is simpler and is typically used for straightforward assignments.
; It directly substitutes the defined value into the code during the assembly 
; process.
    
REG10    equ     10h   // in HEX
REG11    equ     11h
REG01    equ     1h
   
refTemp		equ	0x20
measuredTemp	equ	0x21
contReg		equ	0x22
		
refTempCLN	equ	0x23
measuredTempCLN equ	0x24
	
measValU equ 0x72   // Measured input in decimal
measValH equ 0x71
measValL equ 0x70
 
refValU  equ 0x62   // Reference input in decimal
refValH  equ 0x61
refValL  equ 0x60
  


	
    PSECT absdata,abs,ovrld
	
	ORG	    0x20
    
	MOVLW  0x0C	    // Initialize PORTD.0 and PORTD.1
	MOVWF  TRISD, 0
	MOVLW  0x00
	MOVWF  PORTD, 0
    
	MOVLW  refTempInput	    // Initialize and store values into reg 20,21,22
	MOVWF  refTemp, 0
	MOVWF  refTempCLN, 0
	MOVLW  measuredTempInput
	MOVWF  measuredTemp, 0
	MOVLW  0x00
	MOVWF  contReg, 0
	MOVWF  REG10, 0
	MOVWF  REG11, 0
	
	BTFSS  measuredTemp, 7, 0
	GOTO   GREAT
	GOTO   NEGATIVES
	
	
	ORG    0x350
NEGATIVES:   NOP
	NEGF   measuredTemp, 0
	MOVLW  0x01
	MOVWF  contReg, 0
	MOVFF  measuredTemp, measuredTempCLN
	GOTO   LIGHT
    
	ORG    0x50
GREAT:	MOVLW  refTempInput
	CPFSGT measuredTemp, 0
	GOTO   LESS
	GOTO   COOL
	
	ORG    0x100
LESS:	MOVLW  refTempInput
	CPFSLT measuredTemp, 0
	GOTO   EQUAL
	GOTO   HEAT
	
	ORG    0x150
EQUAL:	MOVLW  0x00
	MOVWF  contReg, 0
	GOTO   LIGHT
	
	ORG    0x200
HEAT:	MOVLW  0x01
	MOVWF  contReg, 0
	GOTO   LIGHT
	
	ORG    0x250
COOL:	MOVLW  0x02
	MOVWF  contReg, 0
	GOTO   LIGHT
	
	ORG    0x300
LIGHT:  MOVFF  contReg, PORTD
	GOTO   CONVDEC1
    
	ORG    0x400
CONVDEC1:MOVLW  0x0A
	CPFSEQ refTempCLN, 0
	CPFSLT refTempCLN, 0
	GOTO   SUB1
	GOTO   ONES1
SUB1:	INCF   REG10, 1, 0
	SUBWF  refTempCLN, 1, 0
	GOTO   CONVDEC1
ONES1:	MOVFF  REG10, refValH
	MOVFF  refTempCLN, refValL

CONVDEC2:MOVLW  0x0A
	CPFSEQ measuredTempCLN, 0
	CPFSLT measuredTempCLN, 0
	GOTO   SUB2
	GOTO   ONES2
SUB2:	INCF   REG11, 1, 0
	SUBWF  measuredTempCLN, 1, 0
	GOTO   CONVDEC2
ONES2:  MOVFF  REG11, measValH
	MOVFF  measuredTempCLN, measValL
	GOTO   FINISH
	
	
	
	
	ORG    0x500
FINISH:	NOP
	END
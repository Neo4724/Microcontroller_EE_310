;---------------------
; Title: Assignment6B_SevenSegment
;---------------------
; Program Details:
;   The purpose of this program is to count up if ButtonA(RB1) is pressed, count
;   down if ButtonB(RB0) is pressed, and reset back to zero if both are pressed.
;   The range is from 0 to F, and it can go from F to 0.
;    
; Inputs: RB0(Decrement) & RB1(Increment)
; Outputs: RD0-7(7-Segment Outputs)
; Setup: The Curiosity Board

; Date: March 24, 2026
; File Dependencies / Libraries: It is required to include the
;   AssemblyConfig.inc in the Header Folder
; Compiler: xc8, 2.4
; Author: Antonio Kassis
; Versions:
;       V1.0: Original


;---------------------
; Initialization
;---------------------
#include "./AssemblyConfig.inc"
#include <xc.inc>
    
;---------------------
; Program Inputs
;---------------------
Inner_loop  EQU 100 // Values for loop to give a delay between 0.2 to 1 second
Outer_loop  EQU 100 
Upper_loop  EQU 8

;---------------------
; Program Constants
;---------------------
#define ButtonA  PORTB,1
#define ButtonB  PORTB,0
#define sevenSeg PORTD

SEG0  EQU 0x10		;7-Segment zero
SEGF  EQU 0x1F		;7-Segment f
AND1  EQU 0x20		;Holds bit for ANDED portion

REG21 EQU 0x21		;Both reg21, reg22, reg23 are used in the loopDelay
REG22 EQU 0x22
REG23 EQU 0x23
 
;---------------------
; Setup & Main Program
;---------------------  
 
    PSECT absdata,abs,ovrld 
 
    ORG 0x20
    
    BANKSEL	PORTB ;
    CLRF	PORTB ;Init PORTB
    BANKSEL	LATB ;Data Latch
    CLRF	LATB ;
    BANKSEL	ANSELB ;
    CLRF	ANSELB ;digital I/O
    BANKSEL	TRISB ;
    MOVLW	0b00000011 ;
    MOVWF	TRISB ;
    
    BANKSEL	PORTD ;
    CLRF	PORTD ;Init PORTA
    BANKSEL	LATD ;Data Latch
    CLRF	LATD ;
    BANKSEL	ANSELD ;
    CLRF	ANSELD ;digital I/O
    BANKSEL	TRISD ;
    MOVLW	0b00000000 ;Set RD[7:1] as outputs
    MOVWF	TRISD ;and set RD0 as ouput
    
    LFSR 0, SEG0    ;Initializing FSR0L to SEG0
    
    MOVLW 0x3F	    ;Setting reg10 as 7-Segment 0
    MOVWF SEG0, 0
    MOVLW 0x06	    ;Setting reg11 as 7-Segment 1
    MOVWF 0x11, 0
    MOVLW 0x5B	    ;Setting reg12 as 7-Segment 2
    MOVWF 0x12, 0
    MOVLW 0x4F	    ;Setting reg13 as 7-Segment 3
    MOVWF 0x13, 0
    MOVLW 0x66	    ;Setting reg14 as 7-Segment 4
    MOVWF 0x14, 0
    MOVLW 0x6D	    ;Setting reg15 as 7-Segment 5
    MOVWF 0x15, 0
    MOVLW 0x7D	    ;Setting reg16 as 7-Segment 6
    MOVWF 0x16, 0
    MOVLW 0x07	    ;Setting reg17 as 7-Segment 7
    MOVWF 0x17, 0
    MOVLW 0x7F	    ;Setting reg18 as 7-Segment 8
    MOVWF 0x18, 0
    MOVLW 0x6F	    ;Setting reg19 as 7-Segment 9
    MOVWF 0x19, 0
    MOVLW 0x77	    ;Setting reg1A as 7-Segment A
    MOVWF 0x1A, 0
    MOVLW 0x7C	    ;Setting reg1B as 7-Segment B
    MOVWF 0x1B, 0
    MOVLW 0x39	    ;Setting reg1C as 7-Segment C
    MOVWF 0x1C, 0
    MOVLW 0x5E	    ;Setting reg1D as 7-Segment D
    MOVWF 0x1D, 0
    MOVLW 0x79	    ;Setting reg1E as 7-Segment E
    MOVWF 0x1E, 0
    MOVLW 0x71	    ;Setting reg1F as 7-Segment F
    MOVWF SEGF, 0
    
    MOVF  INDF0, W	;Initially loads FSR with value 0
    MOVWF sevenSeg, 0	;Displays FSR value to the 7-segment
    
    
    
    
MainLoop:
    BTFSS   ButtonA	;Checks if ButtonA is pressed
    GOTO    CheckButton
    BTFSS   ButtonB	;Checks if ButtonB is pressed
    GOTO    CheckButton
    
    GOTO    Zero	
    
Zero:
    LFSR 0, SEG0	;Loads FSR with value of 0
    GOTO    DisplaySeg

Max:
    LFSR 0, SEGF	;Loads FSR with value of F
    GOTO    DisplaySeg
    
    
Increment:
    MOVF INDF0, W
    CPFSEQ  SEGF, 0	;Checks if FSR is at value F, if so it sets FSR to 
    GOTO Increase	;   value 0. If not, it increments FSR by one.
    GOTO Zero
Increase:
    INCF FSR0L, F
    GOTO DisplaySeg
   
    
Decrement:
    MOVF INDF0, W
    CPFSEQ  SEG0, 0	;Checks if FSR is at value 0, if so it sets FSR to 
    GOTO Decrease	;   value F. If not, it decrements FSR by one.
    GOTO Max
Decrease:
    DECF FSR0L, F
    GOTO DisplaySeg
    
    
CheckButton:
    BTFSC   ButtonA, 0	;Checks if ButtonA is not pressed, goes to increment if it is 
    GOTO    Increment
    
    BTFSC   ButtonB, 0	;Checks if ButtonB is not pressed, goes to decrement if it is
    GOTO    Decrement
    
    GOTO    MainLoop	;Loop continues indefinately
    
    
DisplaySeg:
    MOVF  INDF0, W	;Reads FSR value and sends it to 7-segment
    MOVWF sevenSeg, 0
    CALL  loopDelay	;Delays program for 0.2 to 1 second
    GOTO  MainLoop
   
    
// Delay Function    
loopDelay: 
    MOVLW       Inner_loop
    MOVWF       REG21, 0
    MOVLW       Outer_loop
    MOVWF       REG22, 0
    MOVLW	Upper_loop
    MOVWF	REG23, 0
_loop1:
    DECF        REG21, 1, 0 // inner loop
    BNZ         _loop1
    MOVLW       Inner_loop ; Re-initialize the inner loop for when the outer loop decrements.
    MOVWF       REG21, 0
    DECF        REG22, 1, 0 // outer loop
    BNZ        _loop1
    MOVLW       Outer_loop ; Re-initialize the outer loop for when the upper loop decrements.
    MOVWF       REG22, 0
    DECF        REG23, 1, 0 // upper loop
    BNZ        _loop1
    RETURN
    
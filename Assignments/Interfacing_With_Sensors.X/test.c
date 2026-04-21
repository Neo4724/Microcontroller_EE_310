#include <xc.h>
#define _XTAL_FREQ 4000000

const unsigned char segCode[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

void main(void)
{
    ANSELB = 0x00;
    ANSELD = 0x00;

    TRISBbits.TRISB1 = 1;   // PR1
    TRISBbits.TRISB2 = 1;   // PR2
    TRISD = 0x00;

    WPUBbits.WPUB1 = 1;
    WPUBbits.WPUB2 = 1;

    while(1)
    {
        if (PORTBbits.RB1 == 0)
            LATD = segCode[1];
        else if (PORTBbits.RB2 == 0)
            LATD = segCode[2];
        else
            LATD = segCode[0];
    }
}
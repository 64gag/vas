/*
 * Author: Pedro Aguiar
 */

#define DEBUG 0

#include "config.h"
#include <delays.h>
#include "can.h"
#if DEBUG
    #include <stdio.h>
#endif

#define UART_BUFFER 8
unsigned char filters[2] = {CAN_ID_BRAKE, CAN_ID_ACCEL};
unsigned char c= 0, i = 0;

unsigned char uart_string[UART_BUFFER];
unsigned char uart_byte;
unsigned char u = 3;

void main(void) {
    OSCCON = 0b00000010 | FREQ_8M;
    while (!OSCCONbits.HFIOFS);   /* Wait for oscillator to stabilize */

    /* GPIO configurations*/
    ANCON1 = 0;     /* Completely digital! */
    ANCON0 = 0;
    TRISA = 0x00;
    TRISB = 0x88;   /* Set RB7 and RB3 (RX2 and CANRX) */
    TRISC = 0x00;
    LATA = 0;       /* Datasheet says unused pins should be cleared outputs */
    LATB = 0;
    LATC = 0;

    /* UART configurations */
    TXSTA2bits.TX9=0;
    TXSTA2bits.TXEN = 1;
    TXSTA2bits.SYNC=0;
    TXSTA2bits.SENDB = 0;
    TXSTA2bits.BRGH=1;
    RCSTA2bits.SPEN=1;
    RCSTA2bits.RX9=0;
    RCSTA2bits.CREN=1;
    BAUDCON2bits.TXCKP = 0;
    BAUDCON2bits.BRG16 = 0;
    BAUDCON2bits.WUE = 0; /* Could be 1? */
    BAUDCON2bits.ABDEN = 0;
    SPBRG2 = 12;
    SPBRGH2 = 0;

    /* Interrupts */
    RXB0IF = 0;             /* CAN module interrupts */
    RXB1IF = 0;
    TXB0IF = 0;
    PIE5bits.RXB0IE = 0;
    PIE5bits.RXB1IE = 0;
    TXBIEbits.TXB0IE = 1;

    PIR3bits.RC2IF = 0;     /* EUSART receive*/
    PIE3bits.RC2IE = 1;

    INTCONbits.PEIE = 1;    /* Peripheral interrupt enable */
    INTCONbits.GIE = 1;     /* Global interrupt enable */

    LATAbits.LA0 = 1;

    /* Initialize CAN module */
    can_init(CAN_MODE_NORMAL, 0xff);

    /* Send CAN with: */
    TXB0CON = 0x03;
    TXB0SIDL = 0x00;
    TXB0SIDH = CAN_ID_BRAKE;
    TXB0DLC = 1;
    TXB0D0 = 0x00;

    #if DEBUG
        printf("Reached the infinite loop\r\n");
    #endif

    LATAbits.LA1 = 1;
    
    while(1){
        TXB0SIDH = filters[c];
        TXB0CON |= 0x08;

        c ^= 1;
        for(i = 0; i < 10; i++){
            __delay_ms(50);
        }
        while(TXB0CON & 0x08);
        LATAbits.LA2 ^= 1;
    }
}

void interrupt isr (void){
    if (PIR3bits.RC2IF) {
        uart_byte = RCREG2;

        if(RCSTA2 & 0x06){        /* Framing or overrun error */
            RCSTA2bits.CREN=0;    /* Clear errors and do nothing */
            RCSTA2bits.CREN=1;
        }else{
            if(uart_byte < 'a' || uart_byte > 'z'){
                u = 0;
            }else{
                uart_string[u++] = uart_byte;
            }

            if(u == 2){
                TXB0SIDH = uart_string[0];
                TXB0D0 = uart_string[1];
                TXB0CON |= 0x08;
                u = 0;
            }
            LATAbits.LA2 = 1;
        }
        PIR3bits.RC2IF = 0;
    }

    if(TXB0IF){
        #if DEBUG
            printf("byte 0x%02X sent with SIDH 0x%02X\r\n", TXB0D0, TXB0SIDH);
        #endif
        LATAbits.LA2 = 0;
        TXB0IF = 0;
    }
}

#if DEBUG
void putch(char data) {
   while(!TX2IF){
       continue;
   }

   TXREG2 = data;
}
#endif
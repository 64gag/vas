/*
 * Author: Pedro Aguiar
 */

#define DEBUG 0
#define UART

#include "config.h"
#include <delays.h>
#include "can.h"
#if DEBUG
    #include <stdio.h>
#endif

unsigned char * can_msg_registers[] = {(unsigned char *)&TXB0SIDH, (unsigned char*)&TXB0D0};
unsigned char uart_byte;
unsigned char u = 0;

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
    #ifdef UART
        /* UART configurations */
        TXSTA2bits.TX9 = 0;
        TXSTA2bits.TXEN = 1;
        TXSTA2bits.SYNC = 0;
        TXSTA2bits.SENDB = 0;
        TXSTA2bits.BRGH = 1;
        RCSTA2bits.SPEN = 1;
        RCSTA2bits.RX9 = 0;
        RCSTA2bits.CREN = 1;
        BAUDCON2bits.TXCKP = 0;
        BAUDCON2bits.BRG16 = 0;
        BAUDCON2bits.WUE = 0; /* Could be 1? */
        BAUDCON2bits.ABDEN = 0;
        SPBRG2 = 51;
        SPBRGH2 = 0;
    #endif

    /* Interrupts */
    RXB0IF = 0;             /* CAN module interrupts */
    RXB1IF = 0;
    TXB0IF = 0;
    PIE5bits.RXB0IE = 1;
    PIE5bits.RXB1IE = 1;
    TXBIEbits.TXB0IE = 1;

    #ifdef UART
        PIR3bits.RC2IF = 0;     /* EUSART receive*/
        PIE3bits.RC2IE = 1;
        PIR3bits.TX2IF = 0;     /* EUSART transmit*/
        PIE3bits.TX2IE = 0;
    #endif

    INTCONbits.PEIE = 1;    /* Peripheral interrupt enable */
    INTCONbits.GIE = 1;     /* Global interrupt enable */

    /* Initialize CAN module */
    can_init(CAN_MODE_NORMAL, 0xff);

    /* Send CAN with: */
    TXB0CON = 0x03;
    TXB0SIDL = 0x00;
    TXB0DLC = 1;

    #if DEBUG
        printf("Reached the infinite loop\r\n");
    #endif

    while(1);
}

void interrupt isr (void){
    if (PIR3bits.RC2IF) {
        uart_byte = RCREG2;
        
        if(RCSTA2 & 0x06){        /* Framing or overrun error */
            RCSTA2bits.CREN=0;    /* Clear errors and do nothing */
            RCSTA2bits.CREN=1;
        }else{
            if(uart_byte == 0xff){
                u = 0;
            }else{
                *can_msg_registers[u++] = uart_byte;
            }

            if(u == 2){
                TXB0CON |= 0x08;
            }
        }
        PIR3bits.RC2IF = 0;
    }

    if(TXB0IF){
        #if DEBUG
            printf("byte 0x%02X sent with SIDH 0x%02X\r\n", TXB0D0, TXB0SIDH);
        #endif
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
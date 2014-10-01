/*
 * Author: Pedro Aguiar
 */

#include "config.h"
#include <delays.h>
#include "ecan.h"

/* HMI code */
#define MASK_CNTRL      0x00
#define MASK_BRAKE      0x40
#define MASK_ACCEL      0x50
#define MASK_STEER_L    0x60
#define MASK_STEER_H    0x70

#define UART_BUFFER    6

unsigned char uart_byte[UART_BUFFER];
unsigned char steer_byte;
unsigned char valid_data[UART_BUFFER] = {0};
unsigned char steer_valid = 0;

unsigned char uart_tmp;
unsigned char can_byte;

unsigned char u = 0, i;

void main(void) {
    OSCCON = 0b00000010 | FREQ_8M;
    while (!OSCCONbits.HFIOFS);   /* Wait for oscillator to stabilize */

    /* GPIO configurations*/
    ANCON1 = 0;     /* Completely digital! */
    ANCON0 = 0;
    TRISA = 0x00;
    TRISB = 0x88;   /* Set RB7 (RX2) */
    TRISC = 0x00;
    LATA = 0;       /* Datasheet says unused pins should be cleared outputs */
    LATB = 0;
    LATC = 0;

    /* UART configurations */
    TXSTA2bits.TX9=0;
    TXSTA2bits.TXEN = 0;
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
    PIR3bits.RC2IF = 0;     /* EUSART receive*/
    PIE3bits.RC2IE = 1;
    INTCONbits.PEIE = 1;    /* Peripheral interrupt enable */
    INTCONbits.GIE = 1;     /* Global interrupt enable */

    ECANInitialize();
    
    while(1){
        if(steer_valid == 0x03){
                while(!ECANSendMessage(MASK_STEER_L, &steer_byte, 1, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_3));
                steer_valid = 0;
        }
        for(i = 0; i < UART_BUFFER; i++){
            if(valid_data[i]){
                can_byte = uart_byte[i] & 0x0f;
                while(!ECANSendMessage(uart_byte[i] & 0xf0, &can_byte, 1, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_3));
                valid_data[i] = 0;
            }
        }
    }
}

#pragma code isr=0x08
#pragma interrupt ISR
void ISR (void){
    if (PIR3bits.RC2IF) {
        uart_tmp = RCREG2;

        if(RCSTA2 & 0x06){        /* Framing or overrun error */
            RCSTA2bits.CREN=0;    /* Clear errors and do nothing */
            RCSTA2bits.CREN=1;
        }else{
            if((uart_tmp & MASK_STEER_L) == MASK_STEER_L){
                if(uart_tmp & 0x10){ /* High nibble */
                    steer_byte &= 0x0f;             /* Clear high nibble */
                    steer_byte |= (uart_tmp << 4);  /* Set the new value */
                    steer_valid |= 0x02;            /* Mark it as valid */
                }else{               /* Low nibble */
                    steer_byte &= 0xf0;             /* Clear low nibble */
                    steer_byte |= (uart_tmp & 0x0f);/* Set the new value */
                    steer_valid |= 0x01;            /* Mark it as valid */
                }
            }else{ /* Not a steering value, buffer it */
                uart_byte[u] = uart_tmp;
                valid_data[u] = 1;
                if(++u >= UART_BUFFER){
                    u = 0;
                }
            }
        }
        PIR3bits.RC2IF = 0;
    }
}
#pragma code

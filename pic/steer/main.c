/*
 * File:   main.c
 * Author: paguiar
 *
 *
 * Created on April 16, 2014, 12:14 PM
 */

#include "config.h"
#include <delays.h>


/* * * * * * * * * * * MACROS * * * * * * * * * * */

/* Shaft encoder macros */
#define SHH_MASK    0x07
#define SHM_MASK    0x01
#define SHL_MASK    0x0f
#define SHH_SHIFT   5
#define SHM_SHIFT   4
#define SHL_SHIFT   0
#define SHH_PORT    PORTC
#define SHM_PORT    PORTB
#define SHL_PORT    PORTA
#define SHAFT_POSITION() ((SHH_PORT & SHH_MASK) << SHH_SHIFT | \
			  (SHM_PORT & SHM_MASK) << SHM_SHIFT | \
			  (SHL_PORT & SHL_MASK))

/* HMI code */
#define MASK_CNTRL 0x00
#define MASK_BRAKE 0x40
#define MASK_STEER 0x60
#define MASK_ACCEL 0x70

/* PWM stages */
#define PWM_VAR1    0x1
#define PWM_VAR2    0x2
#define PWM_LOW     0x4
#define PWM_ZERO    0x2 /* Initial state is VAR2 */

#define HMI_ZERO 0

#define H_PIN_INA LATAbits.LATA6
#define H_PIN_INB LATAbits.LATA7
#define H_PIN_ENA LATBbits.LATB5
#define H_PIN_ENB LATBbits.LATB6

#define setDirection(d) { H_PIN_INA = ina_table[d]; H_PIN_INB = inb_table[d]; }
#define setSpeed(s) { CCPR1L = speed_table[s]; }
#define bridgeEnable() {H_PIN_ENA = 1; H_PIN_ENB = 1;}
#define bridgeDisable() {H_PIN_ENA = 0; H_PIN_ENB = 0;}

enum { H_DIR_B2VCC = 0,
       H_DIR_B2GND,
       H_DIR_LEFT,
       H_DIR_RIGHT,
       H_DIR_COUNT };


/* * * * * * * * * * * LOOKUP TABLES * * * * * * * * * * */

#pragma idata gpr5
/* Table with the 128 possible shaft positions, zero means error (or undefined position?) */
unsigned char shaft[256] = { 0x00, 0x38, 0x28, 0x37, 0x18, 0x00, 0x27, 0x34, 0x08, 0x39, 0x00, 0x00, 0x17, 0x00, 0x24, 0x0D, \
                             0x78, 0x00, 0x29, 0x36, 0x00, 0x00, 0x00, 0x35, 0x07, 0x00, 0x00, 0x00, 0x14, 0x13, 0x7D, 0x12, \
                             0x68, 0x69, 0x00, 0x00, 0x19, 0x6A, 0x26, 0x00, 0x00, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x25, 0x0E, \
                             0x77, 0x76, 0x00, 0x00, 0x00, 0x6B, 0x00, 0x00, 0x04, 0x00, 0x03, 0x00, 0x6D, 0x6C, 0x02, 0x01, \
                             0x58, 0x00, 0x59, 0x00, 0x00, 0x00, 0x00, 0x33, 0x09, 0x0A, 0x5A, 0x00, 0x16, 0x0B, 0x00, 0x0C, \
                             0x00, 0x00, 0x2A, 0x2B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x7E, 0x7F, \
                             0x67, 0x00, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x00, \
                             0x74, 0x75, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x5D, 0x5E, 0x5C, 0x00, 0x72, 0x5F, 0x71, 0x00, \
                             0x48, 0x47, 0x00, 0x44, 0x49, 0x00, 0x00, 0x1D, 0x00, 0x46, 0x00, 0x45, 0x00, 0x00, 0x23, 0x22, \
                             0x79, 0x00, 0x7A, 0x00, 0x4A, 0x00, 0x00, 0x1E, 0x06, 0x00, 0x7B, 0x00, 0x00, 0x00, 0x7C, 0x11, \
                             0x00, 0x00, 0x00, 0x43, 0x1A, 0x00, 0x1B, 0x1C, 0x00, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, \
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x6E, 0x00, 0x6F, 0x10, \
                             0x57, 0x54, 0x00, 0x2D, 0x56, 0x55, 0x00, 0x32, 0x00, 0x00, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x21, \
                             0x00, 0x53, 0x00, 0x2C, 0x4B, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, \
                             0x64, 0x3D, 0x65, 0x42, 0x00, 0x3E, 0x00, 0x31, 0x63, 0x3C, 0x00, 0x2F, 0x00, 0x00, 0x00, 0x30, \
                             0x4D, 0x52, 0x4E, 0x41, 0x4C, 0x3F, 0x00, 0x40, 0x62, 0x51, 0x4F, 0x50, 0x61, 0x60, 0x70, 0x00 };
#pragma idata

/* Map table from line angle to encoder position */
unsigned char linemap[182] = { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, \
                               0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7E, 0x7D, 0x7C, 0x7B, 0x7A, 0x79, 0x78, 0x77, 0x76, \
                               0x75, 0x74, 0x73, 0x72, 0x71, 0x70, 0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x69, 0x68, 0x67, \
                               0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5F, 0x5E, 0x5D, 0x5C, 0x5B, 0x5A, 0x59, 0x58, \
                               0x57, 0x56, 0x55, 0x54, 0x53, 0x52, 0x51, 0x50, 0x4F, 0x4E, 0x4D, 0x4C, 0x4B, 0x4A, 0x49, \
                               0x48, 0x47, 0x46, 0x45, 0x44, 0x44, 0x43, 0x43, 0x42, 0x42, 0x41, 0x41, 0x40, 0x40, 0x40, \
                               0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x3E, 0x3E, 0x3D, 0x3D, 0x3C, 0x3C, 0x3B, 0x3A, 0x39, \
                               0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30, 0x2F, 0x2E, 0x2D, 0x2C, 0x2B, 0x2A, \
                               0x29, 0x28, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20, 0x1F, 0x1E, 0x1D, 0x1C, 0x1B, \
                               0x1A, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, \
                               0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, \
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

unsigned char ina_table[] = {1, 0, 1, 0};
unsigned char inb_table[] = {1, 0, 0, 1};
unsigned char speed_table[] = { 0x00, 0x5a, 0x66, 0x72, 0x7e, 0x8a, 0x96, 0xa2, \
                                0xae, 0xba, 0xc5, 0xd0, 0xdb, 0xe6, 0xf1, 0xfc };


/* * * * * * * * * * * GLOBALS * * * * * * * * * * */

unsigned char speed = 1;
unsigned char setpoint = 64; /* Left = 12, center = 64, right = 115 */
unsigned char uart_byte;

void main(void)
{
    OSCCON = 0b00000010 | FREQ_8M;
    while (!OSCCONbits.HFIOFS);   /* Wait for oscillator to stabilize */
    
    /* GPIO configurations*/
    ANCON0 = 0x10;  /* AN4 is used*/
    ANCON1 = 0;
    TRISA = 0x2f;   /* AN4 and encoder */
    TRISB = 0x81;   /* Set RB7 (RX2) and encoder */
    TRISC = 0x07;   /* Encoder */
    LATA = 0;       /* Clear all bits */
    LATB = 0;
    LATC = 0;

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
    SPBRG2 = 12;
    SPBRGH2 = 0;

    /* Configure CCP1 and its timer */
    PR2 = 0xff;             /* Value at which it interrupts */
    TMR2 = 0x00;            /* Clear timer */
    CCPTMRSbits.C1TSEL = 0; /* Use TMR2 */
    PSTR1CON = 0x11;        /* Update at next PWM, use PA1 */
    CCPR1L = 0xaf;          /* Duty registers (MIN DUTY(@lab) = 362 {CCPR1L = 0x5a}, MAX DUTY = 1010 {CCPR1L = 0xfc}) */
    CCP1CON = 0x2c;         /* Single output, <5:4>LSbits to 0b10, P1A active high */
    T2CON = 0x05;           /* Postscale = 0, Prescale = 16 */

    /* Interrupts */
    PIR3bits.RC2IF = 0;     /* EUSART receive*/
    PIE3bits.RC2IE = 1;
    PIR3bits.TX2IF = 0;     /* EUSART transmit*/
    PIE3bits.TX2IE = 1;
    INTCONbits.PEIE = 1;    /* Peripheral interrupt enable */
    INTCONbits.GIE = 1;     /* Global interrupt enable */

    setSpeed(speed);
    setDirection(H_DIR_LEFT);
    bridgeEnable();
    while(shaft[SHAFT_POSITION()] > 30); /* While not at left */

    while(speed < 16){
        setSpeed(speed++);
        setDirection(H_DIR_B2GND);
        Delay10KTCYx(10);
        setDirection(H_DIR_RIGHT);
        while(shaft[SHAFT_POSITION()] < 97); /* While not at right */
        setDirection(H_DIR_B2GND);
        Delay10KTCYx(10);
        setDirection(H_DIR_LEFT);
        while(shaft[SHAFT_POSITION()] > 30); /* While not at left */
    }

    setSpeed(0);
    setDirection(H_DIR_B2GND);
    while(1);
    bridgeDisable();
}

#pragma code isr=0x08
#pragma interrupt ISR
void ISR(void){
    if (PIR3bits.RC2IF) {
        if(RCSTA2 & 0x06){        /* Framing or overrun error */
            uart_byte = RCREG2;      /* Clear errors and do nothing */
            RCSTA2bits.CREN=0;
            RCSTA2bits.CREN=1;
        }else{
            uart_byte = RCREG2;
            if((uart_byte & 0xf0) == MASK_STEER){     /* Device ID @ MSNibble */

            }else if((uart_byte & 0xf0) == MASK_CNTRL){
                if((uart_byte & 0x0f) == 0x00){ /* Reset signal received */
                    setpoint = 64;
                }
            }
        }
        PIR3bits.RC2IF = 0;
    }
}
#pragma code

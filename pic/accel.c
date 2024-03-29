/*
 * Author: Pedro Aguiar
 */

#define DEBUG 0
//#define UART

#include "config.h"
#include <delays.h>
#include "can.h"
#if DEBUG
    #include <stdio.h>
#endif

/* PWM stages */
#define PWM_VAR1    0x1
#define PWM_VAR2    0x2
#define PWM_LOW     0x4
#define PWM_ZERO    0x2     /* Initial state is VAR2 */
#define HMI_ZERO    0x00    /* Initial "hmi_state" */

/* Globals */
unsigned char hmi_state = HMI_ZERO;
unsigned char hmi_state_buffer = HMI_ZERO;
unsigned char pwm_step = PWM_ZERO;
#ifdef UART
    unsigned char uart_byte;
#endif
                                  /*  LV1  CONV1  LV2  CONV2 */
unsigned char duty_table[64]   =   { 0xae, 0x0c, 0x00, 0x0c, \
                                     0xa9, 0x2c, 0x00, 0x0c, \
/* Lines (16) for each HMI state */  0xa4, 0x3c, 0x00, 0x0c, \
/* Then pairs for VAR1/2 stages  */  0xa0, 0x0c, 0x00, 0x0c, \
/* Pairs are: CCPR4L, CCP4CON    */  0x9b, 0x1c, 0x00, 0x0c, \
/* So each line is:              */  0x96, 0x2c, 0x00, 0x0c, \
/* LV1, CONV1, LV2, CONV2        */  0x91, 0x3c, 0x00, 0x0c, \
                                     0x8d, 0x0c, 0x00, 0x0c, \
                                     0x88, 0x1c, 0x00, 0x0c, \
                                     0x83, 0x2c, 0x00, 0x0c, \
                                     0x7e, 0x3c, 0x00, 0x0c, \
                                     0x7a, 0x0c, 0x00, 0x0c, \
                                     0x75, 0x1c, 0x00, 0x0c, \
                                     0x70, 0x2c, 0x00, 0x0c, \
                                     0x6b, 0x3c, 0x00, 0x0c, \
                                     0x66, 0x3c, 0x00, 0x0c };

void main(void) {
    OSCCON = 0b00000010 | FREQ_8M;
    while (!OSCCONbits.HFIOFS);   /* Wait for oscillator to stabilize */

    /* GPIO configurations*/
    ANCON1 = 0;     /* Completely digital */
    ANCON0 = 0;
    TRISA = 0x00;
    TRISB = 0x88;   /* Set RB7 and RB3 (RX2 and CANRX) */
    TRISC = 0x00;
    LATA = 0;       /* Datasheet says unused pins should be cleared outputs */
    LATB = 0;
    LATC = 0;

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

    /* Configure CCP4 and its timer as PWM_LOW stage*/
    PR2 = 0xff;             /* Value at which it interrupts */
    TMR2 = 0x00;            /* Clear timer */
    CCPTMRSbits.C4TSEL = 0; /* Use TMR2 */
    CCPR4L = 0;             /* Duty registers */
    CCP4CON = 0x0c;         /* PWM mode */
    T2CON = 0x3e;           /* Postscale = 8, Prescale = 16 */

    /* Interrupts */
    PIR1bits.TMR2IF = 1;    /* TMR2 (trigger IRQ to set up PWM) */
    PIE1bits.TMR2IE = 1;

    RXB0IF = 0;             /* CAN module interrupts */
    RXB1IF = 0;
    TXB0IF = 0;
    PIE5bits.RXB0IE = 1;
    PIE5bits.RXB1IE = 1;
    TXBIEbits.TXB0IE = 0;
    
    #ifdef UART
        PIR3bits.RC2IF = 0;     /* EUSART receive*/
        PIE3bits.RC2IE = 1;
        PIR3bits.TX2IF = 0;     /* EUSART transmit*/
        PIE3bits.TX2IE = 0;
    #endif

    INTCONbits.PEIE = 1;    /* Peripheral interrupt enable */
    INTCONbits.GIE = 1;     /* Global interrupt enable */

    /* Initialize CAN module */
    can_init(CAN_MODE_NORMAL, CAN_ID_ACCEL);

    #if DEBUG
        printf("Reached the infinite loop\r\n");
    #endif
    
    while(1);
}

void interrupt isr (void){
    if(PIR1bits.TMR2IF){
      switch(pwm_step){
          case PWM_VAR1:
              /* * * * Setup PWM_LOW * * * */
              CCPR4L = 0;                                 /* Duty registers */
              CCP4CON = 0x0c;
              T2CON = 0x3e;                               /* Postscale = 8 */
              pwm_step = PWM_VAR2;                        /* Which is next? */
              break;
          case PWM_VAR2: 
              /* * * * Setup PWM_VAR1 * * * */
              hmi_state = hmi_state_buffer;
              CCPR4L = duty_table[hmi_state << 2];        /* Duty registers */
              CCP4CON = duty_table[(hmi_state << 2) + 1];
              T2CON = 0x06;                               /* Postscale = 1 */
              pwm_step = PWM_LOW;                         /* Which is next? */
              break;
          default: /* PWM_LOW */
              /* * * * Setup PWM_VAR2 * * * */
              CCPR4L = duty_table[(hmi_state << 2) + 2];  /* Duty registers */
              CCP4CON = duty_table[(hmi_state << 2) + 3];
              T2CON = 0x06;                               /* Postscale = 1 */
              pwm_step = PWM_VAR1;                        /* Which is next? */
              break;
      }
      PIR1bits.TMR2IF = 0;
    }

    #ifdef UART
        if (PIR3bits.RC2IF) {
            uart_byte = RCREG2;
            TXREG2 = uart_byte;

            if(RCSTA2 & 0x06){          /* Framing or overrun error */
                RCSTA2bits.CREN=0;      /* Clear errors and do nothing */
                RCSTA2bits.CREN=1;
            }else{
                #if DEBUG
                    printf("Received byte: %x at RCREG2\r\n", uart_byte);
                #endif
            }
            PIR3bits.RC2IF = 0;
        }
    #endif
    
    if(TXB0IF){
        TXB0IF = 0;
    }

    /* Command to this node received */
    if (RXB0IF && RXB0FUL){
        if(RXB0OVFL){
                RXB0OVFL = 0;
        }

        #if DEBUG
            printf("Received byte: %x with ID: %x at RXB0\r\n", RXB0D0, RXB0SIDH);
        #endif

        hmi_state_buffer = RXB0D0; /* Update the state to the received byte */

        /* Done reading, clear flags*/
        RXB0FUL = 0;
        RXB0IF = 0;
    }

    /* RESET signal received */
    if (RXB1IF && RXB1FUL){
        if (RXB1OVFL){
                RXB1OVFL = 0;
        }

        #if DEBUG
            printf("Received byte: %x with ID: %x at RXB1\r\n", RXB1D0, RXB1SIDH);
        #endif

        hmi_state_buffer = HMI_ZERO; /* Reset to initial state */

        /* Done reading, clear flags*/
        RXB1FUL = 0;
        RXB1IF = 0;
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
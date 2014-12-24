#include <xc.h>
#include "can.h"

//8Mhz and 125Kb: (1,4,3,2,2)
//void CANInitialize( SJW, BRP, PHSEG1, PHSEG2, PROPSEG)
void can_init(unsigned char mode, unsigned char id)
{
	/* Change into configuration mode */
	CANCON &= 0x1f;
	CANCON |= CAN_MODE_CONFIG;
        while((CANSTAT & CAN_MODE_MASK) != CAN_MODE_CONFIG);

	/* Set bit rates */
	BRGCON1 = 0b00000011; /* SJW: 1, BRP: 4 */
	BRGCON2 = 0b10010001; /* SEG2 Programmable, read once, SEG1: 3, PRSEG: 2 */
	BRGCON3 = 0b10000001; /* Wake up mode enable, PH2 = 2 */

	CIOCON = 0b00100000;  /* Drives and capture modes */
	ECANCON &= 0x3f;      /* Mode 0 */

	RXB0CON = 0b00100000; /* Receive only standard, overflow will not write to buffer 1 */
	RXB1CON = 0b00100000; /* Receive as per EXIDEN */

	/* Will only accept standard messages */
	RXF0SIDL = 0x00;
	RXF1SIDL = 0x00;
	RXF2SIDL = 0x00;
	RXF3SIDL = 0x00;
	RXF4SIDL = 0x00;
	RXF5SIDL = 0x00;

	RXF0SIDH = id;      /* RXB0 */
	RXF1SIDH = 0xff;

        RXF2SIDH = CAN_ID_RESET;    /* RXB1 */
	RXF3SIDH = 0xff;
	RXF4SIDH = 0xff;
	RXF5SIDH = 0xff;

	/* Use the full most significant register as mask*/
	RXM0SIDL = 0b00000000; /* EXIDEN unimplemented */
	RXM1SIDL = 0x00000000; /* EXIDEN unimplemented */
	RXM0SIDH = 0xff;
	RXM1SIDH = 0xff;

	/* Change into loopback mode */
	CANCON &= 0x1f;
	CANCON |= mode;
	while((CANSTAT & CAN_MODE_MASK) != mode);
}
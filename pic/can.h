#ifndef _CAN_H
#define _CAN_H

/* HMI codes for CAN IDs */
#define CAN_ID_STEER 'a'
#define CAN_ID_RESET 'b'
#define CAN_ID_BRAKE 'c'
#define CAN_ID_ACCEL 'd'

#define CAN_MODE_NORMAL 0x00
#define CAN_MODE_LOOPBACK 0x40
#define CAN_MODE_CONFIG 0x80
#define CAN_MODE_MASK 0xe0

void can_init(unsigned char mode, unsigned char id);

#endif

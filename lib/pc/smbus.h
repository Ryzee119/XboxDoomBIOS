#ifndef SMBUS_H
#define SMBUS_H

#include <stdint.h>

#define SMBUS_STATUS     (SMBUS_IO_BASE + 0x00)
#define SMBUS_CONTROL    (SMBUS_IO_BASE + 0x02)
#define SMBUS_ADDRESS    (SMBUS_IO_BASE + 0x04)
#define SMBUS_DATA       (SMBUS_IO_BASE + 0x06)
#define SMBUS_COMMAND    (SMBUS_IO_BASE + 0x08)
#define SMBUS_BLOCK_DATA (SMBUS_IO_BASE + 0x09)

#define SMBUS_STATUS_ABORT          0x0001
#define SMBUS_STATUS_COLLISION      0x0002
#define SMBUS_STATUS_PROTOCOL_ERROR 0x0004
#define SMBUS_STATUS_BUSY           0x0008
#define SMBUS_STATUS_CYCLE_COMPLETE 0x0010
#define SMBUS_STATUS_TIMEOUT        0x0020
#define SMBUS_STATUS_ERROR          (SMBUS_STATUS_ABORT | SMBUS_STATUS_COLLISION | SMBUS_STATUS_PROTOCOL_ERROR | SMBUS_STATUS_TIMEOUT)

#define SMBUS_CONTROL_TRANSFER_TYPE_BLOCK 0x05
#define SMBUS_CONTROL_TRANSFER_TYPE_WORD  0x03
#define SMBUS_CONTROL_TRANSFER_TYPE_BYTE  0x02
#define SMBUS_CONTROL_TRANSFER_TYPE_ZERO  0x01

#define SMBUS_CONTROL_START         0x08
#define SMBUS_CONTROL_USE_INTERRUPT 0x10
#define SMBUS_CONTROL_ABORT         0x20

#define SMBUS_WRITE 0
#define SMBUS_READ  1

#define SMBUS_RETURN_ERROR -1

void smbus_init(uint16_t io_base);
int8_t smbus_poke(uint8_t address);
int8_t smbus_input_byte(uint8_t address, uint8_t reg, uint8_t *data);
int8_t smbus_input_word(uint8_t address, uint8_t reg, uint16_t *data);
int8_t smbus_input_dword(uint8_t address, uint8_t reg, uint32_t *data);
int8_t smbus_output_byte(uint8_t address, uint8_t reg, uint8_t data);
int8_t smbus_output_word(uint8_t address, uint8_t reg, uint16_t data);
int8_t smbus_output_dword(uint8_t address, uint8_t reg, uint32_t data);

#endif

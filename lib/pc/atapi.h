// SPDX-License-Identifier: CC0-1.0

#ifndef ATAPI_H
#define ATAPI_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct
{
    uint8_t opcode;
    uint8_t reserved_1 : 5;
    uint8_t lun : 3;
    uint8_t page_code; ///< defined in SFF8090i, V6
    uint8_t reserved_3;
    uint8_t allocation_length;
    uint8_t reserved_5;
    uint8_t reserved_6;
    uint8_t reserved_7;
    uint8_t reserved_8;
    uint8_t reserved_9;
    uint8_t reserved_10;
    uint8_t reserved_11;
} atapi_inquiry_cmd_t;

typedef struct
{
    uint8_t opcode;
    uint8_t reserved_1;
    uint8_t reserved_2;
    uint8_t reserved_3;
    uint8_t allocation_length;
    uint8_t reserved_5;
    uint8_t reserved_6;
    uint8_t reserved_7;
    uint8_t reserved_8;
    uint8_t reserved_9;
    uint8_t reserved_10;
    uint8_t reserved_11;
} atapi_request_sense_cmd_t;

// read and write 12
typedef struct
{
    uint8_t opcode;
    uint8_t flags;
    uint32_t lba;
    uint32_t transfer_length;
    uint8_t reserved_10;
    uint8_t control;
} atapi_read12_cmd_t, atapi_write12_cmd_t;

typedef struct
{
    uint8_t opcode;
    uint8_t reserved_1 : 5;
    uint8_t lun : 3;
    uint8_t reserved_2;
    uint8_t reserved_3;
    uint8_t reserved_4;
    uint8_t reserved_5;
    uint8_t reserved_6;
    uint8_t allocation_length_hi;
    uint8_t allocation_length_lo;
    uint8_t reserved_9;
    uint8_t reserved_10;
    uint8_t reserved_11;
} atapi_read_format_capacity_cmd_t;

// read capacity command
typedef struct
{
    uint8_t opcode;
    uint8_t reserved_1 : 5;
    uint8_t lun : 3;
    uint32_t lba;
    uint8_t reserved_6;
    uint8_t reserved_7;
    uint8_t reserved_8;
    uint8_t control;
    uint8_t reserved_10;
    uint8_t reserved_11;
} atapi_read_capacity_cmd_t;

typedef struct
{
    uint8_t opcode;
    uint8_t reserved_1 : 5;
    uint8_t lun : 3;
    uint8_t page_code : 6;
    uint8_t page_control : 2;
    uint8_t reserved_3;
    uint8_t reserved_4;
    uint8_t reserved_5;
    uint8_t reserved_6;
    uint8_t parameter_list_length_hi;
    uint8_t parameter_list_length_lo;
    uint8_t reserved_9;
    uint8_t reserved_10;
    uint8_t reserved_11;
} atapi_mode_sense_cmd_t;

typedef struct
{
    uint8_t error_code : 7;
    uint8_t valid : 1;
    uint8_t reserved_1;
    uint8_t sense_key : 4;
    uint8_t reserved_2 : 1;
    uint8_t Vendor_specifc_1 : 3;
    uint8_t vendor_specific_3;
    uint8_t vendor_specific_4;
    uint8_t vendor_specific_5;
    uint8_t vendor_specific_6;
    uint8_t addnl_sense_length; ///< n - 7
    uint8_t vendor_specific_8;
    uint8_t vendor_specific_9;
    uint8_t vendor_specific_10;
    uint8_t vendor_specific_11;
    uint8_t addnl_sense_code;            ///< mandatory
    uint8_t addnl_sense_code_qualifier;  ///< mandatory
    uint8_t field_replaceable_unit_code; ///< optional
    uint8_t sense_key_specific_15 : 7;
    uint8_t sksv : 1;
    uint8_t sense_key_specific_16;
    uint8_t sense_key_specific_17;
} atapi_mode_sense_response_t;

typedef struct
{
    uint32_t last_lba;
    uint32_t block_size;
} atapi_read_capacity_response_t;

typedef struct
{
    uint8_t reserved_0;
    uint8_t reserved_1;
    uint8_t reserved_2;
    uint8_t capacity_length;
    uint8_t last_lba_3;
    uint8_t last_lba_2;
    uint8_t last_lba_1;
    uint8_t last_lba_0;
    uint8_t descriptor_code : 2;
    uint8_t reserved_9 : 6;
    uint8_t block_size_2;
    uint8_t block_size_1;
    uint8_t block_size_0;
} atapi_read_format_capacity_response_t;

#pragma pack(pop)
#define ATAPI_CMD_SIZE 12

#define ATA_CMD_ATAPI_RESET                  0x08
#define ATAPI_CMD_SOFT_RESET                  0x08
#define ATAPI_CMD_REQUEST_SENSE               0x03
#define ATAPI_CMD_INQUIRY                     0x12
#define ATAPI_CMD_READ_FORMAT_CAPACITY        0x23
#define ATAPI_CMD_READ_CAPACITY               0x25
#define ATAPI_CMD_READ_10                     0x28
#define ATAPI_CMD_WRITE_10                    0x2A
#define ATAPI_CMD_READ_12                     0xA8
#define ATAPI_CMD_WRITE_12                    0xAA
#define ATAPI_CMD_ATAPI_SEEK                  0x2B
#define ATAPI_CMD_WRITE_AND_VERIFY            0x2E
#define ATAPI_CMD_VERIFY                      0x2F
#define ATAPI_CMD_READ_12                     0xA8
#define ATAPI_CMD_WRITE_12                    0xAA
#define ATAPI_CMD_START_STOP_UNIT             0x1B
#define ATAPI_CMD_PREVENT_ALLOW_MEDIA_REMOVAL 0x1E
#define ATAPI_CMD_MODE_SELECT                 0x55
#define ATAPI_CMD_MODE_SENSE                  0x5A

#endif
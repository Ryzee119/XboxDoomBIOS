// SPDX-License-Identifier: CC0-1.0

#ifndef _DMA_8237_H_
#define _DMA_8237_H_

/* DMA Controller Registers (I/O port offsets) */
#define DMA_CHAN0_ADDR(base)  (base + 0x00) // Address register for channel 0
#define DMA_CHAN0_COUNT(base) (base + 0x01) // Count register for channel 0
#define DMA_CHAN1_ADDR(base)  (base + 0x02) // Address register for channel 1
#define DMA_CHAN1_COUNT(base) (base + 0x03) // Count register for channel 1
#define DMA_CHAN2_ADDR(base)  (base + 0x04) // Address register for channel 2
#define DMA_CHAN2_COUNT(base) (base + 0x05) // Count register for channel 2
#define DMA_CHAN3_ADDR(base)  (base + 0x06) // Address register for channel 3
#define DMA_CHAN3_COUNT(base) (base + 0x07) // Count register for channel 3

#define DMA_STATUS_REG(base)     (base + 0x08) // Status register
#define DMA_COMMAND_REG(base)    (base + 0x08) // Command register
#define DMA_REQUEST_REG(base)    (base + 0x09) // Request register
#define DMA_MASK_REG(base)       (base + 0x0A) // Mask register
#define DMA_MODE_REG(base)       (base + 0x0B) // Mode register
#define DMA_CLEAR_FF_REG(base)   (base + 0x0C) // Clear first/last flip-flop
#define DMA_MASTER_CLEAR(base)   (base + 0x0D) // Master clear
#define DMA_CLEAR_MASK_REG(base) (base + 0x0E) // Clear mask register
#define DMA_WRITE_REG(base)      (base + 0x0F) // Write mask register

/* DMA Mode Register Bits */
#define DMA_MODE_CHANNEL_0 0x00 // Select channel 0
#define DMA_MODE_CHANNEL_1 0x01 // Select channel 1
#define DMA_MODE_CHANNEL_2 0x02 // Select channel 2
#define DMA_MODE_CHANNEL_3 0x03 // Select channel 3

#define DMA_MODE_READ   0x04 // Read transfer mode
#define DMA_MODE_WRITE  0x08 // Write transfer mode
#define DMA_MODE_VERIFY 0x0C // Verify transfer mode

#define DMA_MODE_SINGLE 0x40 // Single transfer mode
#define DMA_MODE_BLOCK  0x80 // Block transfer mode
#define DMA_MODE_DEMAND 0xC0 // Demand transfer mode
#define DMA_MODE_AUTO   0x10 // Auto-initialize mode

/* DMA Status Register Bits */
#define DMA_STATUS_TC0 0x01 // Terminal count for channel 0
#define DMA_STATUS_TC1 0x02 // Terminal count for channel 1
#define DMA_STATUS_TC2 0x04 // Terminal count for channel 2
#define DMA_STATUS_TC3 0x08 // Terminal count for channel 3

/* DMA Command Register Bits */
#define DMA_CMD_MEM_TO_MEM  0x01 // Memory-to-memory transfer
#define DMA_CMD_CHAN_0_HOLD 0x02 // Channel 0 hold
#define DMA_CMD_ENABLE      0x04 // Enable controller
#define DMA_CMD_PRIORITY    0x10 // Fixed priority
#define DMA_CMD_COMPRESS    0x20 // Address compression

/* DMA Mask Register Bits */
#define DMA_MASK_CHANNEL_0 0x01 // Mask channel 0
#define DMA_MASK_CHANNEL_1 0x02 // Mask channel 1
#define DMA_MASK_CHANNEL_2 0x04 // Mask channel 2
#define DMA_MASK_CHANNEL_3 0x08 // Mask channel 3

/* DMA Request Register Bits */
#define DMA_REQUEST_CHAN_0 0x01 // Request channel 0
#define DMA_REQUEST_CHAN_1 0x02 // Request channel 1
#define DMA_REQUEST_CHAN_2 0x04 // Request channel 2
#define DMA_REQUEST_CHAN_3 0x08 // Request channel 3

#endif /* _DMA_8237_H_ */

// SPDX-License-Identifier: CC0-1.0

#ifndef PIC8259_H
#define PIC8259_H

/* Initialization Command Word 1 (ICW1) */
#define ICW1_ICW4      0x01 /* ICW4 needed */
#define ICW1_SINGLE    0x02 /* Single IRQ mode */
#define ICW1_INTERVAL4 0x04 /* Call for 4 byte interval */
#define ICW1_LEVEL     0x08 /* Level triggered mode */
#define ICW1_INIT      0x10 /* Initialization */

/* Initialization Command Word 3 (ICW3) */
#define ICW3_MASTER_SLAVE 0x04 /* Master controls slave */
#define ICW3_SLAVE_2      0x02 /* Slave connected to IRQ2 */

/* Initialization Command Word 4 (ICW4) */
#define ICW4_8086_MODE  0x01 /* 8086/88 mode */
#define ICW4_AUTO_EOI   0x02 /* Auto End of Interrupt */
#define ICW4_BUF_SLAVE  0x08 /* Buffered mode slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode master */
#define ICW4_SFNM       0x10 /* Special Fully Nested Mode */

/* Operation Command Word 1 (OCW1) */
#define OCW1_MASK_ALL  0xFF /* Mask all interrupts */
#define OCW1_MASK_IRQ0 0xFE /* Mask IRQ0 */
#define OCW1_MASK_IRQ1 0xFD /* Mask IRQ1 */
#define OCW1_MASK_IRQ2 0xFB /* Mask IRQ2 */
#define OCW1_MASK_IRQ3 0xF7 /* Mask IRQ3 */
#define OCW1_MASK_IRQ4 0xEF /* Mask IRQ4 */
#define OCW1_MASK_IRQ5 0xDF /* Mask IRQ5 */
#define OCW1_MASK_IRQ6 0xBF /* Mask IRQ6 */
#define OCW1_MASK_IRQ7 0x7F /* Mask IRQ7 */

/* Operation Command Word 2 (OCW2) */
#define OCW2_EOI      0x20 /* End of Interrupt */
#define OCW2_SPEC_EOI 0x40 /* Specific EOI */
#define OCW2_ROTATE   0x80 /* Rotate priority */

/* Operation Command Word 3 (OCW3) */
#define OCW3_READ_IRR 0x0A /* Read Interrupt Request Register */
#define OCW3_READ_ISR 0x0B /* Read Interrupt Service Register */
#define OCW3_RIS      0x0C /* Read In-Service Register */

/* Ports for Master and Slave PICs */
#define PIC1_COMMAND 0x20 /* Master PIC command port */
#define PIC1_DATA    0x21 /* Master PIC data port */
#define PIC2_COMMAND 0xA0 /* Slave PIC command port */
#define PIC2_DATA    0xA1 /* Slave PIC data port */

// EOI
#define PIC_EOI 0x20

void pic8259_irq_enable(uint8_t data_port, uint8_t irq);
void pic8259_irq_disable(uint8_t data_port, uint8_t irq);

#endif /* _PIC8259_H */

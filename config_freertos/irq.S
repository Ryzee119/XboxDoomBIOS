.file "FreeRTOSIRQ.S"

/* The correcly handle IRQs with FreeRTOS, the IRQ need to wrap portFREERTOS_INTERRUPT_ENTRY and portINTERRUPT_EPILOGUE (or Exit)
   when calling the user handler. This file provide FreeRTOS friendly IRQs for  all the interrupts in the system.
*/

#include "FreeRTOSConfig.h"
#include "freertos_kernel/portable/GCC/IA32_flat/ISR_Support.h"

// create 3 dwords to store EFLAGS, CS, and EIP
.section .data
exception_frame: .long 0, 0, 0, 0
eax_store: .long 0

.text

.macro EXCEPTION_WRAPPER name, function, error_code_present
    .align 4
    .global \name
    .func \name
    \name :
        cld

        movl eax_store, %eax // Store without changing stack as we're about to clobber it

        // Stack layout looks like:
        // (TOP) *ERROR CODE (0x00), EIP (0x04), CS (0x08), EFLAGS (0x0C)
        // *ERROR CODE is not present for all exceptions so have to handle that

        .if \error_code_present == "Y"
            // If error code is present, store it then remove from stack
            movl 0x00(%esp), %eax
            movl %eax, exception_frame
            addl $4, %esp
        .else
            movl $0, exception_frame // Otherwise set to 0
        .endif

        // Stack layout looks like:
        // (TOP) EIP (0x00), CS (0x04), EFLAGS (0x08)

        movl 0x00(%esp), %eax
        movl %eax, exception_frame + 4 // EIP

        movl 0x04(%esp), %eax
        movl %eax, exception_frame + 8 // CS

        movl 0x08(%esp), %eax
        movl %eax, exception_frame + 12 // EFLAGS

        // Restore eax
        movl eax_store, %eax

        // Call the exception handler function
        pusha
        pushl exception_frame + 12 // EFLAGS
        pushl exception_frame + 8 // CS
        pushl exception_frame + 4 // EIP
        pushl exception_frame + 0 // ERROR CODE
        call \function
        addl $16, %esp
        popa

        hlt
        iret
    .endfunc
.endm

.macro INTERRUPT_WRAPPER name, function, source
    .align 4
    .global \name
    .func \name
    \name :
        portFREERTOS_INTERRUPT_ENTRY
        cld
        call \function
        portINTERRUPT_EPILOGUE

        .if \source == "pic2"
            movb $0x20, %al
            outb %al, $0xA0
        .endif

        movb $0x20, %al
        outb %al, $0x20

        popa
        iret
    .endfunc
.endm

// http://wiki.osdev.org/Exceptions
EXCEPTION_WRAPPER freertos_exception0, exception_handler0, "N"
EXCEPTION_WRAPPER freertos_exception1, exception_handler1, "N"
EXCEPTION_WRAPPER freertos_exception2, exception_handler2, "N"
EXCEPTION_WRAPPER freertos_exception3, exception_handler3, "N"
EXCEPTION_WRAPPER freertos_exception4, exception_handler4, "N"
EXCEPTION_WRAPPER freertos_exception5, exception_handler5, "N"
EXCEPTION_WRAPPER freertos_exception6, exception_handler6, "N"
EXCEPTION_WRAPPER freertos_exception7, exception_handler7, "N"
EXCEPTION_WRAPPER freertos_exception8, exception_handler8, "Y"
EXCEPTION_WRAPPER freertos_exception9, exception_handler9, "N"
EXCEPTION_WRAPPER freertos_exception10, exception_handler10, "Y"
EXCEPTION_WRAPPER freertos_exception11, exception_handler11, "Y"
EXCEPTION_WRAPPER freertos_exception12, exception_handler12, "Y"
EXCEPTION_WRAPPER freertos_exception13, exception_handler13, "Y"
EXCEPTION_WRAPPER freertos_exception14, exception_handler14, "Y"
EXCEPTION_WRAPPER freertos_exception15, exception_handler15, "N"
EXCEPTION_WRAPPER freertos_exception16, exception_handler16, "N"

INTERRUPT_WRAPPER freertos_usb0_interrupt, usb0_handler, "pic1"
INTERRUPT_WRAPPER freertos_gpu_interrupt, gpu_handler, "pic1"
INTERRUPT_WRAPPER freertos_nic_interrupt, nic_handler, "pic1"
INTERRUPT_WRAPPER freertos_apu_interrupt, apu_handler, "pic1"
INTERRUPT_WRAPPER freertos_aci_interrupt, aci_handler, "pic1"
INTERRUPT_WRAPPER freertos_usb1_interrupt, usb1_handler, "pic2"
INTERRUPT_WRAPPER freertos_ide_interrupt, ide_handler, "pic2"
INTERRUPT_WRAPPER freertos_smc_interrupt, smc_handler, "pic2"

.end

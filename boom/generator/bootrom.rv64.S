.section .text.start, "ax", @progbits
.globl _start
_start:
    # Jump to DRAM base address.
    li t0, 0x80000000
    jalr t0

    # Store return value at MMIO base address. (Indicates end of test.)
    li t0, 0x60000000
    sw a0, 0(t0)

    # Hang.
1:  wfi
    j 1b

.section .text.hang, "ax", @progbits
.globl _hang
_hang:
	j _start

.section .text.hang80, "ax", @progbits
.globl _hang80
_hang80:
	j _hang

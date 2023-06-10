_start:
    # Jump to DRAM base address.
    li t0, 0x80000000
    call t0

    # Store return value at MMIO base address. (Indicates end of test.)
    li t0, 0x60000000
    sw a0, 0(t0)

    # Hang.
1:  wfi
    j 1b

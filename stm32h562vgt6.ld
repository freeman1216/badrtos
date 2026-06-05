MEMORY {
  ROM(rx) : ORIGIN = 0x08000000, LENGTH = 1024k
  RAM(rwx) : ORIGIN = 0x20000000, LENGTH = 640k
}

__estack     = ORIGIN(RAM) + LENGTH(RAM);       /* stack points to end of SRAM */

SECTIONS {
    .text : ALIGN(4)
    {   KEEP(*(.ivt))
        *(.text)
        . = ALIGN(4);
        __etext = .;
    } > ROM

    .kernel_bss (NOLOAD) : ALIGN(32)
    {
        __kernel_bss = .;
        *(.kernel_bss)
        __ekernel_bss = .;
   
    } > RAM

    __rkernel_data = LOADADDR(.kernel_data);

	.kernel_data : ALIGN(4) 
	{
		__kernel_data = .;
		*(.kernel_data)
		__ekernel_data = .;
	} > RAM AT > ROM

	.static_stacks : ALIGN(32)
	{
		__static_stacks = .;
        *(.static_stacks)
        __estatic_stacks = .;
	}

    .heap : ALIGN(32)
    {
        __heap = .;
        *(.kheap)
    } > RAM

    __rdata = LOADADDR(.data); /* data placed in rom */

    .data : ALIGN(4)
    {   
        __data = .;
        *(.data)
        . = ALIGN(4);
        __edata = .;
    } > RAM AT > ROM
    
    .bss : ALIGN(4)
    {
        __bss = .;
        *(.bss)
        . = ALIGN(32);
        __ebss = .;
    } > RAM 
    
    __rramfunc = LOADADDR(.ramfunc); /* data placed in rom */

    .ramfunc :ALIGN(4)
    {   
        __ramfunc = .;
        *(.ramfunc)
        . = ALIGN(4);
        __eramfunc = .;
        
    } >RAM AT > ROM
    
    .dma_buffs (NOLOAD) :ALIGN(32)
    {
        __dma_buffs = .; 
        *(.dma_buffs)
        . = ALIGN(32);
        __edma_buffs = .;
    } > RAM

    .init_array :ALIGN(4)
    {  
        __init_array = .;
       *(.init_array)
       . = ALIGN(4);
        __einit_array = .;
    } > ROM

    .rodata : ALIGN(4)
    {
        *(.rodata)
        . = ALIGN(4);
        __erodata = .;
    } > ROM

} 

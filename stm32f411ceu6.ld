MEMORY
{
    ROM (rx) : ORIGIN = 0x8000000, LENGTH  = 512k
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH  = 128k
}
__eram = ORIGIN(RAM) + LENGTH(RAM);
__estack = __eram;



SECTIONS
{   
    .text : ALIGN(4)
    {   KEEP(*(.ivt))
        *(.text)
        . = ALIGN(4);
        __etext = .;
    } > ROM

    .kernel_bss : ALIGN(4)
    {
        __kernel_bss = .;
        *(.kernel_bss)
        . = ALIGN(32);
        __ekernel_bss = .;
        __static_stacks = .;
        *(.static_stacks)
        . = ALIGN(32);
        __estatic_stacks = .;
    } > RAM
    

    .heap : ALIGN(32)
    {
      *(.kheap)
      *(.uheap)
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
        . = ALIGN(4);
        __ebss = .;
    } > RAM 
    

    __rramfunc = LOADADDR(.ramfunc); /* data placed in rom */

    .ramfunc :ALIGN(4)
    {   
        __ramfunc = .;
        *(.ramfunc)
        *(.text.memmove)
        . = ALIGN(4);
        __eramfunc = .;
        
    } >RAM AT > ROM

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

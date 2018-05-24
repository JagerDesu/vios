ENTRY(_start)

STARTUP(startup.o)

MEMORY
{
 EXEC (rx)      : ORIGIN = 0x08000000, LENGTH = 0x10000 /*64K*/
 RAM (rw)       : ORIGIN = 0x20000000, LENGTH = 0x02000 /*8K*/
}

SECTIONS
{
 .text : { * (.text) } > EXEC
 . = ALIGN(4);
 .data : {
	. = ALIGN(8);
	*(.data)
	. = ALIGN(8);
	. = . + 0x1000; /* 4kB of stack memory */
	stack_top = .;
 } > RAM
 .bss : { *(.bss COMMON) }
}

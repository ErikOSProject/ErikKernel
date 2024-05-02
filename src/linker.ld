ENTRY(kernel_main)

SECTIONS
{
	. = 0xfffffffff8000000;
	_kernel_start = .;
	.text : {
		*(.text)
	}
	.data ALIGN(4096) : {
		*(.data)
	}
	.rodata ALIGN(4096) : {
		*(.rodata)
	}
	.bss ALIGN(4096) : {
		*(.bss)
	}
	_kernel_end = .;
}
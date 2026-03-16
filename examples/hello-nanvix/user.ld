/*
 * Custom linker script for hello-nanvix (wasmtime on Nanvix).
 * Based on nanvix/build/user/linker/x86/user.ld with additional sections
 * (.ctors, .got.plt) placed into existing segments to prevent overlapping
 * LOAD segments on the same page, which the Nanvix kernel ELF loader rejects.
 */

OUTPUT_FORMAT("elf32-i386")
ENTRY(_do_start)

PAGE_SIZE = 0x1000;
BASE_ADDR = 0x40000000;

SECTIONS
{
	. = BASE_ADDR;

	/* Text section. */
	.text : ALIGN(PAGE_SIZE)
	{
		*(.crt0*)
		*(.text*)
	}

	/* Initialized data section — padded to page boundary so that .bss
	   (NOBITS) starts on the next page and doesn't share a LOAD segment page. */
	.data : ALIGN(PAGE_SIZE)
	{
		*(.ctors*)
		*(.data*)
		*(.got.plt)
		*(.got)
		. = ALIGN(PAGE_SIZE);
	}

	/* Uninitialized data section. */
	.bss : ALIGN(PAGE_SIZE)
	{
		*(.bss*)
		*(COMMON)
	}

	/* Read-only data section. */
	.rodata : ALIGN(PAGE_SIZE)
	{
		*(.rodata*)
	}

	/* Dynamic symbol table. */
	.dynsym : ALIGN(4)
	{
		PROVIDE(__dynsym_start = .);
		*(.dynsym)
		PROVIDE(__dynsym_end = .);
	}

	/* Dynamic string table. */
	.dynstr : ALIGN(4)
	{
		PROVIDE(__dynstr_start = .);
		*(.dynstr)
		PROVIDE(__dynstr_end = .);
	}

	/* Symbol hash table. */
	.hash : ALIGN(4)
	{
		*(.hash)
		*(.gnu.hash)
	}

	/* Exception handling frame header. */
	.eh_frame_hdr : ALIGN(4)
	{
		PROVIDE(__eh_frame_hdr_start = .);
		KEEP(*(.eh_frame_hdr))
		PROVIDE(__eh_frame_hdr_end = .);
	}

	/* Exception handling frames. */
	.eh_frame : ALIGN(4)
	{
		PROVIDE(__eh_frame_start = .);
		KEEP(*(.eh_frame))
		PROVIDE(__eh_frame_end = .);
	}

	/* Thread-local storage. */
	.tls : ALIGN(PAGE_SIZE)
	{
		__TLS_START = .;
		*(.tdata)
		*(.tbss)
		__TLS_END = .;
	}

	.tls_init ADDR(.tls) : AT(ADDR(.tls))
	{
		FILL(0x00000000);
		. = __TLS_END;
	}

	/DISCARD/ :
	{
		*(.comment)
		*(.note)
	}
}

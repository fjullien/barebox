/*
 * Copyright (c) 2001 William L. Pitts
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 */

#include <common.h>
#include <command.h>
#include <linux/ctype.h>
#include <net.h>
#include <elf.h>
#include <filetype.h>
#include <getopt.h>

static unsigned long load_elf_image_phdr(unsigned long addr);
static unsigned long load_elf_image_shdr(unsigned long addr);

/* Allow ports to override the default behavior */
__attribute__((weak))
unsigned long do_bootelf_exec(ulong (*entry)(int, char * const[]),
			       int argc, char * const argv[])
{
	unsigned long ret;

	/*
	 * QNX images require the data cache is disabled.
	 * Data cache is already flushed, so just turn it off.
	 */
/*	int dcache = dcache_status();
	if (dcache)
		dcache_disable();*/

	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
	ret = entry(argc, argv);

/*	if (dcache)
		dcache_enable();*/

	return ret;
}

/* ======================================================================
 * Determine if a valid ELF image exists at the given memory location.
 * First looks at the ELF header magic field, the makes sure that it is
 * executable and makes sure that it is for a PowerPC.
 * ====================================================================== */
int valid_elf_image(unsigned long addr)
{
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)addr;		/* Elf header structure pointer */

	if (file_detect_type((void *)addr, sizeof(Elf32_Ehdr)) != filetype_elf) {
		printf("## No elf image at address 0x%08lx\n", addr);
		return 0;
	}

	if (ehdr->e_type != ET_EXEC) {
		printf("## Not a 32-bit elf image at address 0x%08lx\n", addr);
		return 0;
	}

	return 1;
}

/* ======================================================================
 * Interpreter command to boot an arbitrary ELF image from memory.
 * ====================================================================== */
int do_bootelf(int argc, char *argv[])
{
	unsigned long addr;		/* Address of the ELF image     */
	unsigned long rc;		/* Return value from user code  */
	char *sload, *saddr;
	const char *elf_file = NULL;
	int opt;

	while ((opt = getopt(argc, argv, "sp")) > 0) {
		switch(opt) {
		case 's':
			break;
		case 'p':
			break;
		default:
			break;
		}
	}

	if (optind != argc)
		elf_file = argv[optind];

	if (!elf_file || !*elf_file) {
		printf("no elf file given\n");
		goto err_out;
	}

/*
	sload = saddr = NULL;
	if (argc == 3) {
		sload = argv[1];
		saddr = argv[2];
	} else if (argc == 2) {
		if (argv[1][0] == '-')
			sload = argv[1];
		else
			saddr = argv[1];
	}

	if (saddr)
		addr = simple_strtoul(saddr, NULL, 16);
	else
		addr = load_addr;

	if (!valid_elf_image(addr))
		return 1;

	if (sload && sload[1] == 'p')
		addr = load_elf_image_phdr(addr);
	else
		addr = load_elf_image_shdr(addr);

	printf("## Starting application at 0x%08lx ...\n", addr);
*/
	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
	rc = do_bootelf_exec((void *)addr, argc - 1, argv + 1);
	if (rc != 0)
		rcode = 1;

	printf("## Application terminated, rc = 0x%lx\n", rc);
	return rcode;
}

/* ======================================================================
 * A very simple elf loader, assumes the image is valid, returns the
 * entry point address.
 * ====================================================================== */
static unsigned long load_elf_image_phdr(unsigned long addr)
{
	Elf32_Ehdr *ehdr;		/* Elf header structure pointer     */
	Elf32_Phdr *phdr;		/* Program header structure pointer */
	int i;

	ehdr = (Elf32_Ehdr *) addr;
	phdr = (Elf32_Phdr *) (addr + ehdr->e_phoff);

	/* Load each program header */
	for (i = 0; i < ehdr->e_phnum; ++i) {
		void *dst = (void *)(uintptr_t) phdr->p_paddr;
		void *src = (void *) addr + phdr->p_offset;
		debug("Loading phdr %i to 0x%p (%i bytes)\n",
			i, dst, phdr->p_filesz);
		if (phdr->p_filesz)
			memcpy(dst, src, phdr->p_filesz);
		if (phdr->p_filesz != phdr->p_memsz)
			memset(dst + phdr->p_filesz, 0x00,
				phdr->p_memsz - phdr->p_filesz);
		flush_cache((unsigned long)dst, phdr->p_filesz);
		++phdr;
	}

	return ehdr->e_entry;
}

static unsigned long load_elf_image_shdr(unsigned long addr)
{
	Elf32_Ehdr *ehdr;		/* Elf header structure pointer     */
	Elf32_Shdr *shdr;		/* Section header structure pointer */
	unsigned char *strtab = 0;	/* String table pointer             */
	unsigned char *image;		/* Binary image pointer             */
	int i;				/* Loop counter                     */

	/* -------------------------------------------------- */

	ehdr = (Elf32_Ehdr *) addr;

	/* Find the section header string table for output info */
	shdr = (Elf32_Shdr *) (addr + ehdr->e_shoff +
			       (ehdr->e_shstrndx * sizeof(Elf32_Shdr)));

	if (shdr->sh_type == SHT_STRTAB)
		strtab = (unsigned char *) (addr + shdr->sh_offset);

	/* Load each appropriate section */
	for (i = 0; i < ehdr->e_shnum; ++i) {
		shdr = (Elf32_Shdr *) (addr + ehdr->e_shoff +
				       (i * sizeof(Elf32_Shdr)));

		if (!(shdr->sh_flags & SHF_ALLOC)
		   || shdr->sh_addr == 0 || shdr->sh_size == 0) {
			continue;
		}

		if (strtab) {
			debug("%sing %s @ 0x%08lx (%ld bytes)\n",
				(shdr->sh_type == SHT_NOBITS) ?
					"Clear" : "Load",
				&strtab[shdr->sh_name],
				(unsigned long) shdr->sh_addr,
				(long) shdr->sh_size);
		}

		if (shdr->sh_type == SHT_NOBITS) {
			memset((void *)(uintptr_t) shdr->sh_addr, 0,
				shdr->sh_size);
		} else {
			image = (unsigned char *) addr + shdr->sh_offset;
			memcpy((void *)(uintptr_t) shdr->sh_addr,
				(const void *) image,
				shdr->sh_size);
		}
		flush_cache(shdr->sh_addr, shdr->sh_size);
	}

	return ehdr->e_entry;
}

/* ====================================================================== */

/*
BAREBOX_CMD_HELP_START(bootelf)
BAREBOX_CMD_HELP_TEXT("This is for booting based on scripts. Unlike the bootm command which")
BAREBOX_CMD_HELP_END
*/
BAREBOX_CMD_START(bootelf)
	.cmd	= do_bootelf,
	BAREBOX_CMD_DESC("Boot from an ELF image in memory")
	BAREBOX_CMD_OPTS("[-ps] [BOOTSRC...]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	//BAREBOX_CMD_HELP(cmd_boot_help)
BAREBOX_CMD_END

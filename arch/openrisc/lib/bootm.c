/*
 * barebox - bootm.c
 *
 * (C) Copyright 2011 - Franck JULLIEN <elec4fun@gmail.com>
 *
 * (C) Copyright 2003, Psyent Corporation <www.psyent.com>
 * Scott McNutt <smcnutt@psyent.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <image.h>
#include <environment.h>
#include <init.h>
#include <boot.h>
#include <errno.h>
#include <sizes.h>
#include <asm/cache.h>

static int do_bootm_linux(struct image_data *idata)
{
	unsigned long mem_free;
	void (*kernel) (unsigned int);
	int ret;

	ret = bootm_load_os(idata, idata->os_address);
	if (ret)
		return ret;

	kernel = (void *)(idata->os_address + idata->os_entry);

	/*
	 * put oftree/initrd close behind compressed kernel image to avoid
	 * placing it outside of the kernels lowmem.
	 */
	mem_free = PAGE_ALIGN(idata->os_res->end + SZ_1M);

	ret = bootm_load_devicetree(idata, mem_free);
	if (ret)
		return ret;

	if (dcache_status() || icache_status())
		flush_cache((ulong)kernel, max(checkdcache(), checkicache()));

	if (bootm_verbose(idata)) {
		printf("\nStarting kernel at 0x%p", kernel);
		if (idata->oftree)
			printf(", oftree at 0x%p", idata->oftree);
		printf("...\n");
	}

	/*
	 * Linux Kernel Parameters (passing device tree):
	 * r3: pointer to the fdt
	 */
	kernel((unsigned int)idata->oftree);
	/* does not return */

	return 1;
}

static struct image_handler handler = {
	.name = "OpenRISC Linux",
	.bootm = do_bootm_linux,
	.filetype = filetype_uimage,
	.ih_os = IH_OS_LINUX,
};

int openrisc_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(openrisc_register_image_handler);


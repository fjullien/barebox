#include <common.h>
#include <init.h>
#include <driver.h>
#include <partition.h>
#include <ns16550.h>
#include <sizes.h>
#include <fs.h>

static struct NS16550_plat serial_plat = {
	.clock = OPENRISC_SOPC_UART_FREQ,
	.shift = 0,
};

static int neek_devices_init(void)
{
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, OPENRISC_SOPC_CFI_BASE, SZ_16M, 0);

	devfs_add_partition("nor0", 0x00000,  SZ_128K,       DEVFS_PARTITION_FIXED, "env0");
	devfs_add_partition("nor0", 0x20000,  SZ_1M,         DEVFS_PARTITION_FIXED, "fpga0");
	devfs_add_partition("nor0", 0x120000, SZ_512K,       DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x1A0000, SZ_8M + SZ_4M, DEVFS_PARTITION_FIXED, "linux");
	devfs_add_partition("nor0", 0xDA0000, 0,             DEVFS_PARTITION_FIXED, "elf");

#ifdef CONFIG_DRIVER_NET_ETHOC
	add_generic_device("ethoc", DEVICE_ID_DYNAMIC, NULL,
			   OPENRISC_SOPC_ETHOC_BASE, 0x1000,
			   IORESOURCE_MEM, NULL);
#endif

	return 0;
}

device_initcall(neek_devices_init);

static int neek_console_init(void)
{
	barebox_set_model("neek");
	barebox_set_hostname("or1k");

	/* Register the serial port */
	add_ns16550_device(DEVICE_ID_DYNAMIC, OPENRISC_SOPC_UART_BASE, 1024,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT, &serial_plat);

	return 0;
}

console_initcall(neek_console_init);

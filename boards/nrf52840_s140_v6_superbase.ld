/* SuperBase: S140 6.1.1, extra filesystem, Bluefruit DFU peer handoff. */
SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x26000, LENGTH = 0xD4000 - 0x26000
  /* The bootloader owns 0x20007F7C (double reset) and 0x20007F80..8000
   * (bonded-peer handoff). Neither globals, heap nor task stacks may use it.
   * Match the bootloader's 0x20008000 application-RAM boundary. */
  RAM (rwx) : ORIGIN = 0x20008000, LENGTH = 0x20040000 - 0x20008000
}

SECTIONS
{
  . = ALIGN(4);
  .svc_data :
  {
    PROVIDE(__start_svc_data = .);
    KEEP(*(.svc_data))
    PROVIDE(__stop_svc_data = .);
  } > RAM

  .fs_data :
  {
    PROVIDE(__start_fs_data = .);
    KEEP(*(.fs_data))
    PROVIDE(__stop_fs_data = .);
  } > RAM
} INSERT AFTER .data;

INCLUDE "nrf52_common.ld"
ASSERT(__data_start__ >= 0x20008000, "SuperBase RAM overlaps bootloader DFU handoff")

Progress Map
------------

| Status                           | POST Code Value |
| ------                           | --------------- |
| DXE_CORE_STARTED                 | 0x60            |
| DXE_SBRUN_INIT                   | 0x62            |
| DXE_NB_HB_INIT                   | 0x68            |
| DXE_NB_INIT                      | 0x69            |
| DXE_NB_SMM_INIT                  | 0x6A            |
| DXE_SB_INIT                      | 0x70            |
| DXE_SB_SMM_INIT                  | 0x71            |
| DXE_SB_DEVICES_INIT              | 0x72            |
| DXE_BDS_STARTED                  | 0x90            |
| DXE_PCI_BUS_BEGIN                | 0x92            |
| DXE_PCI_BUS_HPC_INIT             | 0x93            |
| DXE_PCI_BUS_ENUM                 | 0x94            |
| DXE_PCI_BUS_REQUEST_RESOURCES    | 0x95            |
| DXE_PCI_BUS_ASSIGN_RESOURCES     | 0x96            |
| DXE_CON_OUT_CONNECT              | 0x97            |
| DXE_CON_IN_CONNECT               | 0x98            |
| DXE_SIO_INIT                     | 0x99            |
| DXE_USB_BEGIN                    | 0x9A            |
| DXE_USB_RESET                    | 0x9B            |
| DXE_USB_DETECT                   | 0x9C            |
| DXE_USB_ENABLE                   | 0x9D            |
| DXE_IDE_BEGIN                    | 0xA1            |
| DXE_IDE_RESET                    | 0xA2            |
| DXE_IDE_DETECT                   | 0xA3            |
| DXE_IDE_ENABLE                   | 0xA4            |
| DXE_SCSI_BEGIN                   | 0xA5            |
| DXE_SCSI_RESET                   | 0xA6            |
| DXE_SCSI_DETECT                  | 0xA7            |
| DXE_SCSI_ENABLE                  | 0xA8            |
| DXE_SETUP_START                  | 0xAB            |
| DXE_SETUP_INPUT_WAIT             | 0xAC            |
| DXE_READY_TO_BOOT                | 0xAD            |
| DXE_LEGACY_BOOT                  | 0xAE            |
| DXE_EXIT_BOOT_SERVICES           | 0xAF            |
| RT_SET_VIRTUAL_ADDRESS_MAP_BEGIN | 0xB0            |
| RT_SET_VIRTUAL_ADDRESS_MAP_END   | 0xB1            |
| DXE_LEGACY_OPROM_INIT            | 0xB2            |
| DXE_RESET_SYSTEM                 | 0xB3            |
| DXE_USB_HOTPLUG                  | 0xB4            |
| DXE_PCI_BUS_HOTPLUG              | 0xB5            |

Error Map
---------

| Status                          | POST Code Value |
| ------                          | --------------- |
| DXE_CPU_SELF_TEST_FAILED        | 0x58            |
| DXE_NB_ERROR                    | 0xD1            |
| DXE_SB_ERROR                    | 0xD2            |
| DXE_ARCH_PROTOCOL_NOT_AVAILABLE | 0xD3            |
| DXE_PCI_BUS_OUT_OF_RESOURCES    | 0xD4            |
| DXE_LEGACY_OPROM_NO_SPACE       | 0xD5            |
| DXE_NO_CON_OUT                  | 0xD6            |
| DXE_NO_CON_IN                   | 0xD7            |
| DXE_INVALID_PASSWORD            | 0xD8            |
| DXE_BOOT_OPTION_LOAD_ERROR      | 0xD9            |
| DXE_BOOT_OPTION_FAILED          | 0xDA            |
| DXE_FLASH_UPDATE_FAILED         | 0xDB            |
| DXE_RESET_NOT_AVAILABLE         | 0xDC            |

# RenesasFirmwarePD720202

RenesasFirmwarePD720202 is a firmware loader for Renesas
uPD720201/uPD720202 family USB 3.0 controllers.

It provides the functionality to upload the firmware required for
certain extension cards before they can used.

## Usage

 * First, a firmware image for the chipset is required.
   The final release 2.0.2.6 (K2026090.mem) can be downloaded from
[https://www.renesas.com/en/products/interface/usb-switches-hubs/upd720202-usb-30-host-controller#documents](https://www.renesas.com/en/products/interface/usb-switches-hubs/upd720202-usb-30-host-controller#documents)
   after registering for a free account.
 * The firmware image is uploaded into the chipset.
   This process is non-persistent, the chipset RAM is cleared when the power supply is removed.

To use this driver, update the platform .dsc file:
```
  #
  # Renesas PD720202 XHCI firmware uploader
  #
  Drivers/OptionRomPkg/RenesasFirmwarePD720202/RenesasFirmwarePD720202.inf
```
Then update the platform .fdf file:
```
  #
  # Renesas PD720202 XHCI firmware uploader, requires firmware image
  # in directory $(WORKSPACE)
  #
  INF Drivers/OptionRomPkg/RenesasFirmwarePD720202/RenesasFirmwarePD720202.inf
  FILE FREEFORM = A059EBC4-D73D-4279-81BF-E4A89308B923 {
    SECTION RAW = $(WORKSPACE)/K2026090.mem
  }
```

## Some technical details

The uPD72020x USB 3.0 chipset familiy supports two modes of operation.
Either the firmware is stored in an external EEPROM chip and downloaded
into the chipset at boot time, or it must be uploaded into the chipset
by the operating system / driver.
The second option always works and overrides any firmware stored into
the EEPROM of the card.

The story behind the upd72020x-load tool and more technical details are
discussed in a [blog post](https://mjott.de/blog/881-renesas-usb-3-0-controllers-vs-linux/).

## Acknowledgements

This document is mostly based on README.md from https://github.com/markusj/upd72020x-load.

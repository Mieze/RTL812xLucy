# RTL812xLucy

A new macOS driver for the Realtek RTL812x family of 2.5GBit Ethernet Controllers

**Key Features of RTL812xLucy**

* Supports all versions of Realtek's RTL8125 2.5GBit Ethernet Controllers:
  - RTL8125A
  - RTL8125B
  - RTL8125BP
  - RTL8125CP
  - RTL8125D
* Support for AppleVTD (Tahoe included), but also works without it.
* Support for TCP/IPv4, UDP/IPv4, TCP/IPv6 and UDP/IPv6 checksum offload.
* Supports jumbo frames up to 4076 bytes.
* Fully optimized for Catalina. Note that older versions of macOS might not support 2.5GB Ethernet.
* Support for Energy Efficient Ethernet (EEE).
* The driver is published under GPLv2.

**Current Status**

* The driver has been successfully tested with Tahoe, Sequoia and Monterey but should work fine with Catalina and above.
* Due to performance problems under Tahoe TCP segmentation offload (TSO) is currently disabled but will be added back ASAP. As a result, CPU load is a little bit higher than usal during packet transmission (tx activity).
* VLAN and WoL support are implemented but untested and may not work as expected.
* Support for the RTL8126 series of 5 Gbit Ethernet controllers will be added after the aforementioned problems have been fixed.

**A word on AppleVTD**

Although RTL812x supports AppleVTD, there is no guarantee that your mainboard also does. In case you are unsure if you need AppleVTD, leave it disabled and you'll be on the safe side. When you enable AppleVTD and experience one of the following issues, it's most likely that your board doesn't support AppleVTD:

- Kernel Panics.
- The machine suddenly reboots, freezes and/or the fans speed up.
- No network connection at all.
- The link status keeps going up and down.
- Very low connection throughput.

**What can you do to resolve the issue?**
- Check your board's DMAR table and see if there are any reserved memory regions in it.
- If there are reserved memory regions, you might want to patch your DMAR removing these regions. If it resolves the issue, congratulations! Be careful, because the board's manufacturer did add these regions with intention. Removing them may produce unexpected results too, like the problems described above.
- Otherwise you have to keep AppleVTD disabled, because it is incompatible with your board and there is no way to make it compatible.

**Installation**
- Use OpenCore to inject the driver.
<img width="683" height="145" alt="Bildschirmfoto 2026-02-06 um 20 33 23" src="https://github.com/user-attachments/assets/3a75f549-1ba2-4059-8bb4-d89b8b8b06ed" />

**Contributions**

If you find my projects useful, please consider to buy me a cup of coffee: https://buymeacoffee.com/mieze

Thank you for your support! Your contribution helps me to continue development.


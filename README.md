# 88w8897 Emulator
A Windows emulator for 88w8897 USB mode.

The goal of this project is not so much to achieve a full fledged emulation of Marvell's 88w8897, but rather to emulate or replay a few important parts. We want to interact with mwlu97w8x64.sys (C:\Windows\System32\DriverStore\FileRepository\mwlu97w8x64.inf_amd64_23bc3dc6d91eebdc\mwlu97w8x64.sys) enough to replicate a few worthwhile crashes. 

AFAIK, the 8897 has only been put into production in USB mode for the MS Surface 1 and 2. In these instances, the 8897 is onboard BGA meaning it doesn't take the typical shape of a USB device, but behaves like one. More importantly, the Xbox One makes use of an 8897 via a non-standard USB connector. I will post the pinout for this at a later time. Once wired to a standard USB connector, the traffic from the wifi card on the Xbox One can be monitored, modified, or used on any of the system's USB ports. It is believed that the Xbox One's version of the driver resides in host. I have certainly not seen it in SRA or ERA. Pull the symbols for the driver and check for yourself -- you'll run into a few Xbox references.

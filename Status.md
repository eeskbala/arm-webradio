## Status ##
| **Hardware** |
|:-------------|
| Display                           | working (S65-L2F50, S65-LPH88, S65-LS020, MIO283QT-1, MIO283QT-2) |
| Rotary Encoder                    | working (2 and 4 step encoder) |
| IR Receiver (RC5)                 | working      |
| SD-Card                           | working      |
| VS1033c/VS1033d/VS1053b           | working      |
| F-RAM (FM25Vxx)                   | working      |
| **Ethernet Stack** |
| Protocols                         | ARP, IP, IP-MultiCast, ICMP, TCP, UDP |
| DHCP Client                       | working      |
| DNS Client                        | working      |
| NTP Client                        | working      |
| HTTP Server                       | working      |
| FTP Server                        | -            |
| UPnP Device                       | working      |
| **FAT Stack** |
| ELM ChaN's FatFs with LFN         | working      |
| **Features** |
| Play Shoutcast/Icecast streams    | working      |
| Play RTSP streams                 | working      |
| Play from SMB/CIFS Shares         | -            |
| Play from Memory Card             | working      |
| Load Settings from Memory Card    | working      |
| Change Settings via Menu          | working      |
| Change Settings via HTTP          | working      |
| Control via HTTP                  | Station control works |
| Control via UPnP                  | Volume control works  |
| Alarm Clock                       | working      |
| Bootloader                        | working      |


### Tested microSD-Cards ###
| **SD-Card Vendor** | **Capacity** | **v0.08** | **v0.09** | **v0.10** | **trunk** |
|:-------------------|:-------------|:----------|:----------|:----------|:----------|
|   Kingston         |    2 GiB     | works     |  fails    |  works    |  works    |
|   Platinum         |    1 GiB     |           |           |  works    |  works    |
|   Platinum         |    2 GiB     |           |           |  works    |  works    |
|   SanDisk          |  512 MiB     | works     |  works    |  works    |  works    |
|   SanDisk          |    2 GiB     | works     |  works    |  works    |  works    |

(This list will be expanded whenever we will check a new card. If your SD card is not listed, please drop us a note if it works or not.)
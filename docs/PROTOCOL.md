# Communication Protocol

## Overview

The server client communicates with the STM32 display via USB CDC serial at 115200 baud.

## Data Format

Data is sent as key-value pairs, one per line, terminated by an END marker.

```
key1=value1
key2=value2
...
END
```

## Available Keys

| Key | Description | Example |
|-----|-------------|--------|
| ip | Server IP address | 192.168.1.100 |
| up | System uptime | 5d 12h 30m |
| load_1 | Load average (1 min) | 0.52 |
| load_5 | Load average (5 min) | 0.48 |
| load_15 | Load average (15 min) | 0.45 |
| cpu_pct | CPU usage percentage | 45 |
| cpu_temp | CPU temperature (C) | 52 |
| ram_used | RAM used (MB) | 2048 |
| ram_total | RAM total (MB) | 8192 |
| ram_pct | RAM usage percentage | 25 |
| swap_used | Swap used (MB) | 128 |
| swap_total | Swap total (MB) | 2048 |
| swap_pct | Swap usage percentage | 6 |
| disk_used | Disk used (GB) | 120 |
| disk_total | Disk total (GB) | 500 |
| disk_pct | Disk usage percentage | 24 |
| net_rx | Network RX (KB/s) | 1500 |
| net_tx | Network TX (KB/s) | 800 |
| disk_read | Disk read (MB/s) | 1.5 |
| disk_write | Disk write (MB/s) | 0.8 |
| proc_count | Process count | 245 |
| smb_clients | SMB connected clients | 3 |
| conn_in | Incoming TCP connections | 12 |

## Timing

- Default update interval: 2 seconds
- Small delay (10ms) between each key-value pair
- END marker triggers display refresh

## Example Session

```
ip=192.168.1.100
up=5d 12h 30m
load_1=0.52
load_5=0.48
load_15=0.45
cpu_pct=45
cpu_temp=52
ram_used=2048
ram_total=8192
ram_pct=25
swap_used=128
swap_total=2048
swap_pct=6
disk_used=120
disk_total=500
disk_pct=24
net_rx=1500
net_tx=800
disk_read=1.5
disk_write=0.8
proc_count=245
smb_clients=3
conn_in=12
END
```

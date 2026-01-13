#!/usr/bin/env python3
import serial
import time

s = serial.Serial("/dev/ttyACM0", 115200, timeout=1)
print("Porta aberta:", s.name)
time.sleep(0.5)

dados = [
    "ip=192.168.1.100",
    "up=1d 2h 30m",
    "cpu_pct=45",
    "cpu_temp=52",
    "ram_used=2048",
    "ram_total=8192",
    "ram_pct=25",
    "swap_used=128",
    "swap_total=2048",
    "swap_pct=6",
    "disk_used=120",
    "disk_total=180",
    "disk_pct=67",
    "net_rx=1500",
    "net_tx=800",
    "load_1=1.25",
    "load_5=0.98",
    "load_15=0.75",
    "END"
]

for d in dados:
    s.write((d + "\n").encode())
    time.sleep(0.01)

print("Dados enviados!")
s.close()

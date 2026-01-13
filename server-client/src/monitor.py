#!/usr/bin/env python3
"""
Server Monitor - Envia estatísticas do servidor para o display STM32

Uso:
    python server_monitor.py /dev/ttyUSB0
    python server_monitor.py /dev/ttyACM0 -i 5  # Intervalo de 5 segundos

Deploy para o servidor:
    scp server_monitor.py debian@192.168.1.50:/home/debian/server_monitor/
    Senha: 1221

Instalar dependências no servidor:
    pip install psutil pyserial

Executar como serviço (opcional):
    sudo cp server_monitor.service /etc/systemd/system/
    sudo systemctl enable server_monitor
    sudo systemctl start server_monitor

Nota: Para SMB funcionar, execute como root ou adicione usuário ao grupo root.
"""

import argparse
import subprocess
import time
from pathlib import Path

import psutil
import serial

# Constants
SECONDS_PER_DAY = 86400
SECONDS_PER_HOUR = 3600
SECONDS_PER_MINUTE = 60
MILLIDEGREE_DIVISOR = 1000
BITS_PER_BYTE = 8
KILO = 1000
MB = 1024 * 1024
GB = 1024 * 1024 * 1024


def get_ip_address() -> str:
    """Obtém o IP da interface de rede local (ignora VPN)."""
    try:
        interfaces = psutil.net_if_addrs()
        for iface_name, addrs in interfaces.items():
            if iface_name.startswith(('lo', 'tun', 'nordlynx', 'wg', 'docker', 'br-', 'veth')):
                continue
            if iface_name.startswith(('eth', 'enp', 'eno', 'ens')):
                for addr in addrs:
                    if addr.family.name == 'AF_INET':
                        return addr.address
        
        for iface_name, addrs in interfaces.items():
            if iface_name.startswith(('lo', 'tun', 'nordlynx', 'wg', 'docker', 'br-', 'veth')):
                continue
            for addr in addrs:
                if addr.family.name == 'AF_INET':
                    ip = addr.address
                    if ip.startswith(('192.168.', '10.', '172.16.', '172.17.', '172.18.', '172.19.', '172.2', '172.30.', '172.31.')):
                        return ip
        
        return "0.0.0.0"
    except (OSError, AttributeError):
        return "0.0.0.0"


def get_cpu_temp() -> str:
    """Obtém temperatura da CPU."""
    temp_paths = [
        Path("/sys/class/thermal/thermal_zone0/temp"),
        Path("/sys/class/hwmon/hwmon0/temp1_input"),
        Path("/sys/class/hwmon/hwmon1/temp1_input"),
    ]

    for path in temp_paths:
        try:
            if path.exists():
                temp = int(path.read_text().strip()) / MILLIDEGREE_DIVISOR
                return f"{temp:.0f}"
        except (OSError, ValueError):
            continue

    try:
        temps = psutil.sensors_temperatures()
        for entries in temps.values():
            if entries:
                return f"{entries[0].current:.0f}"
    except (OSError, AttributeError):
        pass

    return "--"


def get_uptime() -> str:
    """Obtém uptime formatado."""
    try:
        uptime_seconds = time.time() - psutil.boot_time()
        days = int(uptime_seconds // SECONDS_PER_DAY)
        hours = int((uptime_seconds % SECONDS_PER_DAY) // SECONDS_PER_HOUR)
        minutes = int((uptime_seconds % SECONDS_PER_HOUR) // SECONDS_PER_MINUTE)

        if days > 0:
            return f"{days}d {hours}h {minutes}m"
        if hours > 0:
            return f"{hours}h {minutes}m"
        return f"{minutes}m"
    except OSError:
        return "--"


def get_load_average() -> tuple[str, str, str]:
    """Obtém load average."""
    try:
        load1, load5, load15 = psutil.getloadavg()
        return f"{load1:.2f}", f"{load5:.2f}", f"{load15:.2f}"
    except OSError:
        return "--", "--", "--"


def get_network_speed(interval: float = 1.0) -> tuple[str, str]:
    """Obtém velocidade de rede em kbps."""
    if interval <= 0:
        return "--", "--"

    try:
        net1 = psutil.net_io_counters()
        time.sleep(interval)
        net2 = psutil.net_io_counters()
        
        rx_kbps = (net2.bytes_recv - net1.bytes_recv) * BITS_PER_BYTE / KILO / interval
        tx_kbps = (net2.bytes_sent - net1.bytes_sent) * BITS_PER_BYTE / KILO / interval

        return f"{rx_kbps:.0f}", f"{tx_kbps:.0f}"
    except (OSError, AttributeError):
        return "--", "--"


def get_disk_io(interval: float = 0.5) -> tuple[str, str]:
    """Obtém taxa de leitura/escrita do disco em MB/s."""
    if interval <= 0:
        return "--", "--"

    try:
        io1 = psutil.disk_io_counters()
        time.sleep(interval)
        io2 = psutil.disk_io_counters()

        read_mbps = (io2.read_bytes - io1.read_bytes) / MB / interval
        write_mbps = (io2.write_bytes - io1.write_bytes) / MB / interval

        return f"{read_mbps:.1f}", f"{write_mbps:.1f}"
    except (OSError, AttributeError):
        return "--", "--"


def get_process_count() -> str:
    """Conta número de processos rodando."""
    try:
        return str(len(psutil.pids()))
    except OSError:
        return "--"


def get_samba_clients() -> str:
    """Conta clientes únicos conectados via Samba (SMB)."""
    try:
        # Tenta com sudo primeiro, depois sem
        for cmd in [["sudo", "-n", "smbstatus", "-b"], ["smbstatus", "-b"]]:
            try:
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    timeout=5,
                    check=False,
                )
                if result.returncode == 0:
                    break
            except FileNotFoundError:
                continue
        else:
            return "0"

        # Conta IPs únicos nas linhas de conexão
        clients: set[str] = set()
        in_connections = False
        
        for line in result.stdout.split("\n"):
            line = line.strip()
            
            # Detecta início da seção de conexões
            if "----" in line:
                in_connections = True
                continue
            
            if not in_connections or not line:
                continue
            
            # Formato: PID Username Group Machine (ipaddr) Protocol
            # Ou: PID Username Group Machine Protocol
            parts = line.split()
            if len(parts) >= 4 and parts[0].isdigit():
                # Procura por IP entre parênteses ou usa o campo machine
                for part in parts:
                    if part.startswith("(") and part.endswith(")"):
                        ip = part[1:-1]
                        clients.add(ip)
                        break
                else:
                    # Usa o campo machine (índice 3)
                    clients.add(parts[3])

        return str(len(clients))

    except (OSError, subprocess.TimeoutExpired):
        return "--"


def get_smb_connections() -> str:
    """Conta conexões SMB via porta 445 (alternativa ao smbstatus)."""
    try:
        connections = psutil.net_connections(kind="tcp")
        smb_conns = set()
        for c in connections:
            if c.status == "ESTABLISHED" and c.laddr and c.raddr:
                # Porta 445 = SMB, 139 = NetBIOS
                if c.laddr.port in (445, 139):
                    smb_conns.add(c.raddr.ip)
        return str(len(smb_conns))
    except (OSError, psutil.AccessDenied):
        return "--"


def get_incoming_connections() -> str:
    """Conta conexões TCP entrantes (exclui SMB para não duplicar)."""
    try:
        connections = psutil.net_connections(kind="tcp")
        incoming = sum(
            1 for c in connections 
            if c.status == "ESTABLISHED" 
            and c.raddr
            and c.laddr
            and c.laddr.port < 49152
            and c.laddr.port not in (445, 139)  # Exclui SMB
        )
        return str(incoming)
    except (OSError, psutil.AccessDenied):
        return "--"


def collect_stats() -> dict[str, str]:
    """Coleta todas as estatísticas do sistema."""
    cpu_pct = psutil.cpu_percent(interval=0.5)

    mem = psutil.virtual_memory()
    ram_used = mem.used // MB
    ram_total = mem.total // MB
    ram_pct = mem.percent

    swap = psutil.swap_memory()
    swap_used = swap.used // MB
    swap_total = swap.total // MB
    swap_pct = swap.percent

    try:
        disk = psutil.disk_usage("/srv")
    except OSError:
        disk = psutil.disk_usage("/")
    disk_used = disk.used // GB
    disk_total = disk.total // GB
    disk_pct = disk.percent

    load1, load5, load15 = get_load_average()
    net_rx, net_tx = get_network_speed(0.5)
    disk_read, disk_write = get_disk_io(0.5)

    return {
        "ip": get_ip_address(),
        "up": get_uptime(),
        "load_1": load1,
        "load_5": load5,
        "load_15": load15,
        "cpu_pct": f"{cpu_pct:.0f}",
        "cpu_temp": get_cpu_temp(),
        "ram_used": str(ram_used),
        "ram_total": str(ram_total),
        "ram_pct": f"{ram_pct:.0f}",
        "swap_used": str(swap_used),
        "swap_total": str(swap_total),
        "swap_pct": f"{swap_pct:.0f}",
        "disk_used": str(disk_used),
        "disk_total": str(disk_total),
        "disk_pct": f"{disk_pct:.0f}",
        "net_rx": net_rx,
        "net_tx": net_tx,
        "disk_read": disk_read,
        "disk_write": disk_write,
        "proc_count": get_process_count(),
        "smb_clients": get_smb_connections(),  # Usa método via porta
        "conn_in": get_incoming_connections(),
    }


def send_to_display(ser: serial.Serial, stats: dict[str, str]) -> None:
    """Envia estatísticas para o display via serial."""
    for key, value in stats.items():
        line = f"{key}={value}\n"
        ser.write(line.encode())
        time.sleep(0.01)
    
    ser.write(b"END\n")


def main() -> None:
    parser = argparse.ArgumentParser(description="Monitor de servidor para display STM32")
    parser.add_argument("port", help="Porta serial (ex: /dev/ttyUSB0)")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("-i", "--interval", type=float, default=2.0, help="Intervalo de atualização em segundos (default: 2)")
    args = parser.parse_args()
    
    print(f"Conectando em {args.port} @ {args.baud} baud...")
    
    ser: serial.Serial | None = None
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        print(f"Conectado! Atualizando a cada {args.interval}s")
        print("Pressione Ctrl+C para sair\n")
        
        while True:
            stats = collect_stats()
            
            print(
                f"\r[{time.strftime('%H:%M:%S')}] "
                f"CPU: {stats['cpu_pct']}% @ {stats['cpu_temp']}C | "
                f"RAM: {stats['ram_pct']}% | "
                f"LOAD: {stats['load_1']} | "
                f"SMB: {stats['smb_clients']} | "
                f"CONN: {stats['conn_in']}      ",
                end="",
            )

            send_to_display(ser, stats)
            time.sleep(args.interval)
            
    except serial.SerialException as e:
        print(f"Erro na porta serial: {e}")
    except KeyboardInterrupt:
        print("\n\nEncerrado pelo usuário.")
    finally:
        if ser is not None:
            ser.close()


if __name__ == "__main__":
    main()

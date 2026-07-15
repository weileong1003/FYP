import re
import sys
import os

if len(sys.argv) < 2:
    print("Usage: python3 parse_log.py <logfile>")
    sys.exit(1)

LOG_FILE = sys.argv[1]

if not os.path.exists(LOG_FILE):
    print(f"ERROR: File '{LOG_FILE}' not found.")
    sys.exit(1)

CLOCK_SECOND = 128
SIM_TIME_SEC = 2040

with open(LOG_FILE, "r", encoding="utf-8", errors="ignore") as f:
    data = f.read()

client_tx_final = {}
client_energy_final = {}

for line in data.split('\n'):
    match = re.search(r'CLIENT_STATS\s+node=(\d+)\s+TX=(\d+)\s+ENERGY=([\d.]+)', line)
    if match:
        node = int(match.group(1))
        tx = int(match.group(2))
        energy = float(match.group(3))
        client_tx_final[node] = tx
        client_energy_final[node] = energy

num_clients = len(client_tx_final)

final_rx_match = re.search(r'FINAL\s+RX\s*=\s*(\d+)', data)
final_delay_match = re.search(r'FINAL\s+Avg\s+Delay\s*=\s*(\d+)\s*ticks', data)
final_energy_match = re.search(r'FINAL\s+Energy.*?TOTAL=([\d.]+)', data)

final_rx = int(final_rx_match.group(1)) if final_rx_match else 0
final_delay_ticks = int(final_delay_match.group(1)) if final_delay_match else 0
server_energy = float(final_energy_match.group(1)) if final_energy_match else 0.0

dio_count = len(re.findall(r'sending a (?:multicast|unicast)-DIO with rank 256', data))
dis_count = len(re.findall(r'received a DIS', data))
dao_count = len(re.findall(r'received a DAO from', data))

total_control = dio_count + dis_count + dao_count

total_tx = sum(client_tx_final.values())
pdr = (final_rx / total_tx * 100.0) if total_tx > 0 else 0.0

avg_delay_sec = final_delay_ticks / CLOCK_SECOND

all_client_energy = list(client_energy_final.values())
total_energy_all_nodes = sum(all_client_energy) + server_energy
avg_energy_mj_s = total_energy_all_nodes / SIM_TIME_SEC

avg_tx_per_client = total_tx / num_clients if num_clients > 0 else 0.0
control_overhead = (total_control / total_tx * 100.0) if total_tx > 0 else 0.0

print("=" * 55)
print("        RPL PERFORMANCE ANALYSIS RESULTS")
print("=" * 55)
print(f"Log file            : {LOG_FILE}")
print(f"Number of clients   : {num_clients}")
print("-" * 55)
print(f"Total TX (all cli)  : {total_tx}")
print(f"Total RX (server)   : {final_rx}")
print(f"Avg TX per client   : {avg_tx_per_client:.2f}")
print(f"PDR (%)             : {pdr:.2f} %")
print("-" * 55)
print(f"Avg E2E Delay       : {final_delay_ticks} ticks")
print(f"Avg E2E Delay       : {avg_delay_sec:.4f} seconds")
print("-" * 55)
print(f"Total Energy (all)  : {total_energy_all_nodes:.2f} mJ")
print(f"Avg Energy per sec  : {avg_energy_mj_s:.4f} mJ/s")
print("-" * 55)
print(f"Control Messages :")
print(f"  DIO sent          : {dio_count}")
print(f"  DIS rcvd          : {dis_count}")
print(f"  DAO rcvd          : {dao_count}")
print(f"  TOTAL control     : {total_control}")
print(f"Control Overhead    : {control_overhead:.2f} %")
print("-" * 55)

if num_clients == 0:
    print("WARNING: No CLIENT_STATS found.")
if final_rx == 0:
    print("WARNING: FINAL RX is 0.")
if total_control == 0:
    print("WARNING: No RPL control messages found for root.")
print("=" * 55)

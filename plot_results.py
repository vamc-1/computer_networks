import pandas as pd
import matplotlib.pyplot as plt

# --- Load UDP results ---
udp_df = pd.read_csv("udp_results.csv")

# --- Load TCP results (skip repeated headers) ---
tcp_df = pd.read_csv("tcp_results.csv", comment="T")

# Rename columns just in case
tcp_df.columns = ["TotalData(MB)", "ChunkSize(KB)", "Throughput(Mbps)"]

# Convert datatypes
tcp_df = tcp_df.astype({"TotalData(MB)": int,
                        "ChunkSize(KB)": int,
                        "Throughput(Mbps)": float})

# --- Plot UDP ---
plt.figure(figsize=(10,6))
plt.plot(udp_df["Size(Bytes)"], udp_df["Throughput(Mbps)"],
         marker='o', linestyle='-', color='blue', label="UDP")

# --- Plot TCP (x-axis = ChunkSize(KB)) ---
plt.plot(tcp_df["ChunkSize(KB)"] * 1024,  # convert KB → Bytes
         tcp_df["Throughput(Mbps)"],
         marker='s', linestyle='--', color='red', label="TCP")

# --- Labels and formatting ---
plt.xlabel("Packet/Chunk Size (Bytes)")
plt.ylabel("Throughput (Mbps)")
plt.title("UDP vs TCP Throughput")
plt.legend()
plt.grid(True)

# Save to PNG
plt.savefig("udp_tcp_comparison.png")

# Uncomment if GUI available
# plt.show()

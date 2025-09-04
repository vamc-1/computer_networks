import pandas as pd
import matplotlib.pyplot as plt

# Load CSV file
df = pd.read_csv("udp_results.csv")

# Plot line graph
plt.figure(figsize=(10,6))
plt.plot(df["Size(Bytes)"], df["Throughput(Mbps)"], marker='o', linestyle='-', color='b', label="UDP Throughput")

# Labels and title
plt.xlabel("Packet Size (Bytes)")
plt.ylabel("Throughput (Mbps)")
plt.title("UDP Performance: Packet Size vs Throughput")
plt.legend()
plt.grid(True)

# Save to file (works in WSL/VM)
plt.savefig("udp_throughput.png")

# If you have GUI support, uncomment below:
# plt.show()

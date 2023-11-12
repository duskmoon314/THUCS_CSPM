import csv
import argparse

import polars as pl
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()

parser.add_argument(
    "input_file",
    help="Path to the input file",
    default="bandwidth.csv",
    type=str,
)
parser.add_argument(
    "output_file",
    help="Path to the output file",
    default="bandwidth.png",
    type=str,
)

args = parser.parse_args()

# Read the CSV file
#
# size(Byte), read(us), write(us)
data = pl.read_csv(args.input_file)

data = data.select(
    "size",
    (pl.col("size") / pl.col("read") * 2.1).alias("read"),
    (pl.col("size") / pl.col("write") * 2.1).alias("write"),
)

# Plot the data
fig, ax = plt.subplots()
ax.plot(data["size"], data["read"], label="Read", marker=".")
ax.plot(data["size"], data["write"], label="Write", marker=".")

# Add vertical lines for L1, L2, L3 cache sizes
# L1: 32KB
# L2: 1MB
# L3: 27.5MB
ax.axvline(x=32 * 1024, color="black", linestyle=":")
ax.axvline(x=1024 * 1024, color="black", linestyle=":")
ax.axvline(x=27.5 * 1024 * 1024, color="black", linestyle=":")

ax.set_xlabel("Array Size (Byte)")
ax.set_xscale("log", base=2)
ax.set_ylabel("Bandwidth (GB/s)")
# ax.set_yscale("log", base=10)
ax.set_title("Bandwidth")
ax.legend()
plt.savefig(args.output_file)

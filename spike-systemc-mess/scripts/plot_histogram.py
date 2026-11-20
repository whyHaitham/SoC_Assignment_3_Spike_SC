#!/usr/bin/env python3
"""Plot the histogram of inter-cache-miss time from cache_miss_log.csv.

The gaps are computed from ``time_ns`` after keeping only the data-cache
misses (the memory requests of the MESS kernel), so instruction-cache misses
never split an interval.  At 1 GHz, 1 ns equals 1 CPU cycle, so the histogram
uses one bin per nanosecond and a logarithmic y-axis.
"""
import argparse
import csv
import statistics
from collections import Counter
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("csv_file", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--title", default="")
    args = parser.parse_args()

    with args.csv_file.open(newline="") as stream:
        times = sorted(int(row["time_ns"]) for row in csv.DictReader(stream)
                       if row["cache"] == "dcache")
    gaps = [b - a for a, b in zip(times, times[1:])]
    if not gaps:
        raise SystemExit("fewer than two data-cache misses in the log")

    lo, hi = min(gaps), max(gaps)
    top = Counter(gaps).most_common(3)
    fig, ax = plt.subplots(figsize=(10, 5.8))
    ax.hist(gaps, bins=[x - 0.5 for x in range(lo, hi + 2)], log=True,
            color="#3a6ea5", edgecolor="#1f3d5c", linewidth=0.3)
    pad = max(1.0, 0.015 * (hi - lo))
    ax.set_xlim(lo - pad, hi + pad)
    ax.margins(y=0.15)
    ax.grid(axis="y", alpha=0.3)
    ax.set_xlabel("Time between consecutive data-cache misses "
                  "(ns  =  CPU cycles at 1 GHz)")
    ax.set_ylabel("Number of occurrences (log scale)")
    ax.set_title("Inter-cache-miss time histogram"
                 + (f"\n{args.title}" if args.title else ""), fontsize=11)
    box = (f"intervals : {len(gaps):,}\n"
           f"mean      : {statistics.fmean(gaps):.1f} ns\n"
           f"median    : {statistics.median(gaps):.0f} ns\n"
           f"min / max : {lo} / {hi} ns\n"
           "most common gaps:\n"
           + "\n".join(f"  {g:>5} ns  x{c:,}" for g, c in top))
    ax.text(0.98, 0.97, box, transform=ax.transAxes, ha="right", va="top",
            fontsize=8.5, family="monospace", multialignment="left",
            bbox=dict(boxstyle="round", facecolor="white", alpha=0.85,
                      edgecolor="#999999"))
    fig.tight_layout()
    fig.savefig(args.output, dpi=170)

    print(f"data-cache misses: {len(times)}, intervals: {len(gaps)}, "
          f"min {lo} ns, max {hi} ns, mean {statistics.fmean(gaps):.1f} ns")
    print("most common gaps: " + ", ".join(f"{g} ns x{c}" for g, c in top))
    print(f"wrote {args.output}")


if __name__ == "__main__":
    main()

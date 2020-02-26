#!/usr/bin/python3

import matplotlib.pyplot as plt
import pandas as pd
import sys

if __name__ == "__main__":
  if(len(sys.argv) < 2):
    print("Usage: latency_analysis.py csv_filename")
    sys.exit(0)

  dataFrame = pd.read_csv(sys.argv[1], sep=',')
  print(dataFrame["write"])
  print(dataFrame.columns)

  plt.hist([dataFrame["write"], dataFrame["roundtrip"], dataFrame["take"]],
    bins=20, range=(0, 1000), stacked=True, color=['r', 'g', 'b'])

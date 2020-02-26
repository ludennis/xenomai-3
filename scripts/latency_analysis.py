#!/usr/bin/python3

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys

if __name__ == "__main__":
  if(len(sys.argv) < 2):
    print("Usage: latency_analysis.py csv_filename")
    sys.exit(0)

  dataFrame = pd.read_csv(sys.argv[1], sep=',')

  dataFrame["total"] = dataFrame.sum(axis = 1)

  for i, column in enumerate(dataFrame.columns):
    plt.subplot(len(dataFrame.columns), 1, i+1)
    plt.plot(np.arange(dataFrame.shape[0]), dataFrame[column])
    plt.axhline(y=80, color='r', linestyle='-')
    plt.title(column)
    plt.ylabel("time (us)")

  plt.show()

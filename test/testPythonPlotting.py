import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')) # Up 1 directory

import smartPlotMessage as sp
import time


if __name__ == "__main__":
    sp.createFlushThread(100)
    sp.networkConfigure("x.x.x.x", 2000)

    for i in range(100):
        sp.plotList_1D(range(i), 1e6, -1, "python", "num")
        sp.timePlot_2D_Int(i, 1e6, -1, "2d", "2d")
        time.sleep(0.1)
    time.sleep(1)
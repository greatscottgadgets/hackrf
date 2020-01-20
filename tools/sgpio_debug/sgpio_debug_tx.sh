#!/bin/sh
python3 create_tx_counter.py
hackrf_transfer -R -t tx_counter.bin

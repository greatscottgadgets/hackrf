# HTime Tool

The HTime Tool is a host application for accessing the API of the HTime
extension.

## Tool help

```
Manage HTime for HackRF: ticks, seconds, A/D trigger, clocks.

Usage:
      -h, --help: this help
      -d, --divisor <v>: set divisor to <v> val next pps
      -D, --Divisor <v>: set divisor to <v> val for one pps
      -f, --set_clk_freq <v>: set sync clock freq to <v>
      -k, --ticks : read ticks counter now
      -K, --Ticks : set ticks counter now
      -l, --trig_hold <0/1>: set trig hold state (1: sample next PPS, active one PPS later; 0: active next PPS)
      -p, --pps_out <0/1>: enable/disable pps output on next pps
      -r, --trig_delay <v>: set trig delay to <v> val next pps
      -s, --seconds <v>: set seconds counter now to <v> value
      -S, --Seconds <v>: as -s but in sync to next pps
      -t, --get_seconds: read seconds counter now
      -y, --mcu_clk_sync <0/1>: enable/disable mcu clock sync

Examples:
      hackrf_time -s 1234     # set seconds counter to 1234 now
      hackrf_time -l 1        # enable trigger hold on next PPS
      hackrf_time -p 0        # disable PPS output on next PPS
      hackrf_time -y 1        # enable MCU sync mode

v0.1.0 20250215 F.P. <mxgbot@gmail.com>
```

Each tool option corresponds to one API function of HTime. For a more
detailed explanation of each function, see
[HTime USB API](htime_api.md).

## Source

The tool source code is at
[host/hackrf-tools/src/hackrf_time.c](../host/hackrf-tools/src/hackrf_time.c).

---

<p xmlns:cc="http://creativecommons.org/ns#" xmlns:dct="http://purl.org/dc/terms/"><span property="dct:title">HTime documentation</span> by <span property="cc:attributionName">Fabrizio Pollastri</span> is licensed under <a href="https://creativecommons.org/licenses/by-sa/4.0/?ref=chooser-v1" target="_blank" rel="license noopener noreferrer" style="display:inline-block;">CC BY-SA 4.0</a></p>

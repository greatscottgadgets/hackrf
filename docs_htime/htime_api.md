# HTime USB API

This API allows access to all the firmware features of HTime. An example
of application can be seen in the
[HTime Tool source](../host/hackrf-tools/src/hackrf_time.c) and its
[usage documentation](htime_tool.md).

---

**`hackrf_time_set_divisor_next_pps(device, divisor)`**

Set **divisor** value (uint32_t) into the ticks counter at the next PPS.
With the HackRF MCU working @ 200 MHz, the ticks counter must be set to
200000000-1 to obtain a counting period of 1 second. **device** is a
pointer to the HackRF device.

---

**`hackrf_time_set_divisor_one_pps(device, divisor)`**

Set **divisor** value (uint32_t) into the ticks counter from the next PPS
for only one counter cycle then restore the previous divisor value.
**device** is a pointer to the HackRF device.

---

**`hackrf_time_set_trig_delay_next_pps(device, trig_delay)`**

Set **trig_delay** value (uint32_t) as the sampling trigger delay at next
PPS. The trigger delay is from the start of the second (PPS leading edge)
in tick units. **device** is a pointer to the HackRF device.

---

**`hackrf_time_get_seconds_now(device, &seconds)`**

Get the value of the second counter immediately into **seconds**
(* int64_t). **device** is a pointer to the HackRF device.

---

**`hackrf_time_set_seconds_now(device, seconds)`**

Set the **seconds** value (int64_t) immediately into the second counter.
**device** is a pointer to the HackRF device.

---

**`hackrf_time_set_seconds_next_pps(device, seconds)`**

Set the **seconds** value (int64_t) at next PPS into the second counter.
**device** is a pointer to the HackRF device.

---

**`hackrf_time_get_ticks_now(device, &ticks)`**

Get the value of the tick counter immediately into **ticks**
(* uint32_t). **device** is a pointer to the HackRF device.

---

**`hackrf_time_set_ticks_now(device, ticks)`**

Set the **ticks** value (uint32_t) immediately into the tick counter.
**device** is a pointer to the HackRF device.

---

**`hackrf_time_set_clk_freq(device, clk_freq)`**

Set the **clk_freq** value (double) as the sampling rate synchronized to
the MCU clock. Must be a value near (+-100 Hz) 10 MHz. **device** is a
pointer to the HackRF device.

---

**`hackrf_time_set_mcu_clk_sync(device, enable)`**

Set the **enable** value (uint8_t) of the MCU synchronized clock mode.
When this mode is on, all relevant clocks are kept in sync to avoid phase
shifts and frequency drifts. **device** is a pointer to the HackRF
device.

---

**`hackrf_time_set_trig_hold_enable_next_pps(device, enable)`**

Set the **enable** value (uint8_t) of trigger hold at the next PPS
leading edge. When enabled, the trigger trailing edge action is disabled,
so the trigger output can stay active across the second boundary
(long capture crossing second border). When disabled, the trigger
trailing edge is cleared as usual, just after next PPS leading edge.
**device** is a pointer to the HackRF device.

---

**`hackrf_time_set_pps_out_enable_next_pps(device, enable)`**

Set the **enable** value (uint8_t) of PPS output at the next PPS leading
edge. When enabled, regular PPS output pulses are generated.
When disabled, PPS output is kept low (inactive). **device** is a pointer
to the HackRF device.

---

<p xmlns:cc="http://creativecommons.org/ns#" xmlns:dct="http://purl.org/dc/terms/"><span property="dct:title">HTime documentation</span> by <span property="cc:attributionName">Fabrizio Pollastri</span> is licensed under <a href="https://creativecommons.org/licenses/by-sa/4.0/?ref=chooser-v1" target="_blank" rel="license noopener noreferrer" style="display:inline-block;">CC BY-SA 4.0</a></p>

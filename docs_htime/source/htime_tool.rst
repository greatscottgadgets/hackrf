==========
HTime Tool
==========

The Htime Tool is a host application for accessing the API of the HTime extension.


Tool help
~~~~~~~~~

.. code::

  Manage HTime for HackRF: ticks, seconds, A/D trigger, clocks.

  Usage:
        -h, --help: this help
        -d, --divisor <v>: set divisor to <v> val next pps
        -D, --Divisor <v>: set divisor to <v> val for one pps
        -f, --set_clk_freq <v>: set sync clock freq to <v>
        -k, --ticks : read ticks counter now
        -K, --Ticks : set ticks counter now
        -r, --trig_delay <v>: set trig delay to <v> val next pps
        -s, --seconds <v>: set seconds counter now to <v> value
        -S, --Seconds <v>: as -s but in sync to next pps
        -t, --get_seconds: read seconds counter now
        -y, --mcu_clk_sync <0/1>: enable/disable mcu clock sync

  Examples:
        hackrf_time -s 1234     # set seconds counter to 1234 now
        hackrf_time -y 1        # enable MCU sync mode

  v0.1.0 20250215 F.P. <mxgbot@gmail.com>

Each tool option corresponds to one API function of HTime. For a more detailed
explanation of each function, see "HTime USB API" in :doc:`htime`.


.. raw:: html

 <p xmlns:cc="http://creativecommons.org/ns#" xmlns:dct="http://purl.org/dc/terms/"><span property="dct:title">HTime documentation</span> by <span property="cc:attributionName">Fabrizio Pollastri</span> is licensed under <a href="https://creativecommons.org/licenses/by-sa/4.0/?ref=chooser-v1" target="_blank" rel="license noopener noreferrer" style="display:inline-block;">CC BY-SA 4.0<img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/cc.svg?ref=chooser-v1" alt=""><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/by.svg?ref=chooser-v1" alt=""><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/sa.svg?ref=chooser-v1" alt=""></a></p>


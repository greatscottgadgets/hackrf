Expansion Interface
~~~~~~~~~~~~~~~~~~~

The common HackRF expansion interface consists of headers P20, P22, and P28. These headers are present on both HackRF Pro and HackRF One, and support hardware add-ons including PortaPack and Opera Cake.

P20 GPIO
^^^^^^^^

Providing access to GPIO, ADC, RTC, and power.

.. list-table :: 
  :header-rows: 1
  :widths: 1 1 

  * - Pin 
    - Function
  * - 1   
    - VBAT
  * - 2   
    - RTC_ALARM (One) / PB_5 (Pro)
  * - 3   
    - VCC (One) / 3V3AUX (Pro)
  * - 4   
    - WAKEUP
  * - 5   
    - GPIO3_8
  * - 6   
    - GPIO3_0 (One) / GPIO3_9 (Pro)
  * - 7   
    - GPIO3_10
  * - 8   
    - GPIO3_11
  * - 9   
    - GPIO3_12
  * - 10  
    - GPIO3_13
  * - 11  
    - GPIO3_14
  * - 12  
    - GPIO3_15
  * - 13  
    - GND
  * - 14  
    - ADC0_6
  * - 15  
    - GND
  * - 16  
    - ADC0_2
  * - 17  
    - VBUSCTRL
  * - 18  
    - ADC0_5
  * - 19  
    - GND
  * - 20  
    - ADC0_0
  * - 21  
    - VBUS
  * - 22  
    - VIN



P22 I2S
^^^^^^^

I2S, SPI, I2C, UART, GPIO, and clocks.

.. list-table :: 
  :header-rows: 1
  :widths: 1 1 

  * - Pin     
    - Function
  * - 1   
    - CLKOUT (One) / P2 (Pro)
  * - 2   
    - CLKIN
  * - 3   
    - RESET
  * - 4   
    - GND
  * - 5   
    - I2C1_SCL
  * - 6   
    - I2C1_SDA
  * - 7   
    - SPIFI_MISO (One) / PB_1 (Pro)
  * - 8   
    - SPIFI_SCK (One) / PB_3 (Pro)
  * - 9   
    - SPIFI_MOSI (One) / PA_4 (Pro)
  * - 10  
    - GND
  * - 11  
    - VCC (One) / 3V3AUX (Pro)
  * - 12  
    - I2S0_RX_SCK (One) / PA_3 (Pro)
  * - 13  
    - I2S0_RX_SDA (One) / I2S0_TX_SDA (Pro)
  * - 14  
    - I2S0_RX_MCLK (One) / PB_0 (Pro)
  * - 15  
    - I2S0_RX_WS
  * - 16  
    - I2S0_TX_SCK
  * - 17  
    - I2S0_TX_MCLK
  * - 18  
    - GND
  * - 19  
    - U0_RXD
  * - 20  
    - U0_TXD
  * - 21  
    - P2_9
  * - 22  
    - P2_13
  * - 23  
    - P2_8
  * - 24  
    - SDA
  * - 25  
    - CLK6 (One) / AUX_CLK2 (Pro)
  * - 26  
    - SCL



P28 SD
^^^^^^

SDIO, GPIO, clocks, and CPLD.

.. list-table :: 
  :header-rows: 1
  :widths: 1 1 

  * - Pin     
    - Function
  * - 1   
    - VCC (One) / 3V3AUX (Pro)
  * - 2   
    - GND
  * - 3   
    - SD_CD
  * - 4   
    - SD_DAT3
  * - 5   
    - SD_DAT2
  * - 6   
    - SD_DAT1
  * - 7   
    - SD_DAT0
  * - 8   
    - SD_VOLT0
  * - 9   
    - SD_CMD
  * - 10  
    - SD_POW
  * - 11  
    - SD_CLK
  * - 12  
    - GND
  * - 13  
    - GCK2 (One) / P5_6 (Pro)
  * - 14  
    - GCK1 (One) / P5_7 (Pro)
  * - 15  
    - Trigger out: B1AUX14 (One) / TRIGGER.OUT (Pro)
  * - 16  
    - Trigger in: B1AUX13 (One) / TRIGGER.IN (Pro)
  * - 17  
    - CPLD_TCK
  * - 18  
    - BANK2F3M2 (One) / PE_0 (Pro)
  * - 19  
    - CPLD_TDI (One) / I2S0_RX_SDA (Pro)
  * - 20  
    - BANK2F3M6 (One) / P9_1 (Pro)
  * - 21  
    - BANK2F3M12 (One) / P5_3 (Pro)
  * - 22  
    - BANK2F3M4 (One) / P1_7 (Pro)

P9 Baseband (HackRF One)
^^^^^^^^^^^^^^^^^^^^^^^^

A direct analog interface to the high speed dual ADC and dual DAC.

.. list-table ::
  :header-rows: 1
  :widths: 1 1

  * - Pin
    - Function
  * - 1
    - GND
  * - 2
    - GND
  * - 3
    - GND
  * - 4
    - RXBBQ-
  * - 5
    - RXBBI-
  * - 6
    - RXBBQ+
  * - 7
    - RXBBI+
  * - 8
    - GND
  * - 9
    - GND
  * - 10
    - TXBBI-
  * - 11
    - TXBBQ+
  * - 12
    - TXBBI+
  * - 13
    - TXBBQ-
  * - 14
    - GND
  * - 15
    - GND
  * - 16
    - GND

Additional unpopulated headers and test points are available for test and development, but they may be incompatible with some enclosure or expansion options.

Refer to the schematics and component documentation for more information.

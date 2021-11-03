Expansion Interface
~~~~~~~~~~~~~~~~~~~

The HackRF One expansion interface consists of headers P9, P20, P22, and P28. These four headers are installed on the commercial HackRF One from Great Scott Gadgets.



P9 Baseband 
^^^^^^^^^^^

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
    - RTC_ALARM
  * - 3   
    - VCC
  * - 4   
    - WAKEUP
  * - 5   
    - GPIO3_8
  * - 6   
    - GPIO3_0
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
    - CLKOUT
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
    - SPIFI_MISO
  * - 8   
    - SPIFI_SCK
  * - 9   
    - SPIFI_MOSI
  * - 10  
    - GND
  * - 11  
    - VCC
  * - 12  
    - I2S0_RX_SCK
  * - 13  
    - I2S_RX_SDA
  * - 14  
    - I2S0_RX_MCLK
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
    - CLK6
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
    - VCC
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
    - GCK2
  * - 14  
    - GCK1
  * - 15  
    - B1AUX14
  * - 16  
    - B1AUX13
  * - 17  
    - CPLD_TCK
  * - 18  
    - BANK2F3M2
  * - 19  
    - CPLD_TDI
  * - 20  
    - BANK2F3M6
  * - 21  
    - BANK2F3M12
  * - 22  
    - BANK2F3M4

Additional unpopulated headers and test points are available for test and development, but they may be incompatible with some enclosure or expansion options.

Refer to the schematics and component documentation for more information.
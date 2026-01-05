#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

from amaranth import Elaboratable, Module, Instance, Signal, ClockSignal

# References:
# [1] LATTICE ICE™ Technology Library, Version 3.0, August, 2016
# [2] iCE40™ LP/HX/LM Family Handbook, HB1011 Version 01.2, November 2013

class SPIDeviceInterface(Elaboratable):
    
    def __init__(self, port):
        # I/O port.
        self.port        = port

        # Data I/O.
        self.word_in     = Signal(8)
        self.word_out    = Signal(8)
        self.word_in_stb = Signal()

        # Status flags.
        self.busy        = Signal()


    def elaborate(self, platform):
        m = Module()

        spi_adr  = Signal(8, init=0b1000)   # address
        spi_dati = Signal(8)                # data input
        spi_dato = Signal(8)                # data output
        spi_rw   = Signal()                 # selects between read or write (high = write)
        spi_stb  = Signal()                 # strobe must be asserted to start a read/write
        spi_ack  = Signal()                 # ack that the transfer is done (read valid, write ack)

        # SB_SPI interface is documented in [1].
        sb_spi_params = {
            # SPI port connections.
            "o_SO":     self.port.cipo.o,
            "o_SOE":    self.port.cipo.oe,
            "i_SI":     self.port.copi.i,
            "i_SCKI":   self.port.clk.i,
            "i_SCSNI": ~self.port.cs.i,  # chip select is inverted due to PinsN
            # Internal signaling.
            "i_SBCLKI": ClockSignal("sync"),
            "i_SBSTBI": spi_stb,
            "i_SBRWI":  spi_rw,
            "o_SBACKO": spi_ack,
        }
        sb_spi_params |= { f"i_SBADRI{i}": spi_adr[i]  for i in range(8) }
        sb_spi_params |= { f"i_SBDATI{i}": spi_dati[i] for i in range(8) }
        sb_spi_params |= { f"o_SBDATO{i}": spi_dato[i] for i in range(8) }

        m.submodules.sb_spi = sb_spi = Instance("SB_SPI", **sb_spi_params)
        
        # Register addresses (from [2]).
        SPI_ADDR_SPICR0  = 0b1000  # SPI Control Register 0
        SPI_ADDR_SPICR1  = 0b1001  # SPI Control Register 1
        SPI_ADDR_SPICR2  = 0b1010  # SPI Control Register 2
        SPI_ADDR_SPIBR   = 0b1011  # SPI Clock Prescale
        SPI_ADDR_SPISR   = 0b1100  # SPI Status Register
        SPI_ADDR_SPITXDR = 0b1101  # SPI Transmit Data Register
        SPI_ADDR_SPIRXDR = 0b1110  # SPI Receive Data Register
        SPI_ADDR_SPICSR  = 0b1111  # SPI Master Chip Select Register

        # Initial values for programming registers ([2]).
        registers_init = {
            SPI_ADDR_SPICR2: 0b00000110,  # CPOL=1 CPHA=1 mode, MSB first
            SPI_ADDR_SPICR1: 0b10000000,  # Enable SPI
        }

        # De-assert strobe signals unless explicitly asserted.
        m.d.sync += spi_stb.eq(0)
        m.d.sync += self.word_in_stb.eq(0)

        with m.FSM():

            # Register initialization.
            for i, (address, value) in enumerate(registers_init.items()):
                with m.State(f"INIT{i}"):
                    m.d.sync += [
                        spi_adr  .eq(address),
                        spi_dati .eq(value),
                        spi_stb  .eq(1),
                        spi_rw   .eq(1),
                    ]
                    with m.If(spi_ack):
                        m.d.sync += spi_stb.eq(0)
                        if i+1 < len(registers_init):
                            m.next = f"INIT{i+1}"
                        else:
                            m.next = "WAIT"
            
            with m.State("WAIT"):
                m.d.sync += [
                    spi_adr .eq(SPI_ADDR_SPISR),
                    spi_stb .eq(1),
                    spi_rw  .eq(0),
                ]
                with m.If(spi_ack):
                    m.d.sync += spi_stb.eq(0)
                    # bit 3 = RRDY, data is available to read
                    # bit 4 = TRDY, transmit data is empty
                    # bit 6 = BUSY, chip select is asserted (low)
                    # bit 7 = TIP, transfer in progress
                    m.d.sync += self.busy.eq(spi_dato[6])
                    with m.If(spi_dato[7] & spi_dato[4]):
                        m.next = "SPI_TRANSMIT"
                    with m.Elif(spi_dato[3]):
                        m.next = "SPI_READ"

            with m.State("SPI_READ"):
                m.d.sync += [
                    spi_adr .eq(SPI_ADDR_SPIRXDR),
                    spi_stb .eq(1),
                    spi_rw  .eq(0),
                ]
                with m.If(spi_ack):
                    m.d.sync += [
                        spi_stb          .eq(0),
                        self.word_in     .eq(spi_dato),
                        self.word_in_stb .eq(1),
                    ]
                    m.next = "WAIT"
                        
            with m.State("SPI_TRANSMIT"):
                m.d.sync += [
                    spi_adr  .eq(SPI_ADDR_SPITXDR),
                    spi_dati .eq(self.word_out),
                    spi_stb  .eq(1),
                    spi_rw   .eq(1),
                ]
                with m.If(spi_ack):
                    m.d.sync += spi_stb.eq(0)
                    m.next = "WAIT"

        return m


class SPICommandInterface(Elaboratable):
    """ Wrapper of SPIDeviceInterface that splits data sequences into phases.

    I/O signals:
        O: command       -- the command read from the SPI bus
        O: command_ready -- a new command is ready

        O: word_received -- the most recent word received
        O: word_complete -- strobe indicating a new word is present on word_in
        I: word_to_send  -- the word to be loaded; latched in on next word_complete and while cs is low
    """

    def __init__(self, port):

        # I/O port.
        self.interface      = SPIDeviceInterface(port)

        # Command I/O.
        self.command        = Signal(8)
        self.command_ready  = Signal()

        # Data I/O
        self.word_received  = Signal(8)
        self.word_to_send   = Signal.like(self.word_received)
        self.word_complete  = Signal()


    def elaborate(self, platform):

        m = Module()

        # Attach our SPI interface.
        m.submodules.interface = interface = self.interface

        # De-assert our control signals unless explicitly asserted.
        m.d.sync += [
            self.command_ready.eq(0),
            self.word_complete.eq(0)
        ]

        m.d.comb += interface.word_out.eq(self.word_to_send)

        with m.FSM():

            with m.State("COMMAND_PHASE"):
                with m.If(interface.word_in_stb):
                    m.d.sync += [
                        self.command        .eq(interface.word_in),
                        self.command_ready  .eq(1),
                    ]
                    m.next = "DATA_PHASE"
                
                # Do not advance if chip select is deasserted.
                with m.If(~interface.busy):
                    m.next = "COMMAND_PHASE"

            with m.State("DATA_PHASE"):
                with m.If(interface.word_in_stb):
                    m.d.sync += self.word_received.eq(interface.word_in)
                    m.d.sync += self.word_complete.eq(1)
                    m.next = "DUMMY_PHASE"

                # Do not advance if chip select is deasserted.
                with m.If(~interface.busy):
                    m.next = "COMMAND_PHASE"

            # The SB_SPI block always returns 0xFF for the second byte, so at least one
            # dummy byte must be added to retrieve valid data. This behavior is shown in
            # Figure 22-16, "Minimally Specified SPI Transaction Example," from [2].
            with m.State("DUMMY_PHASE"):
                with m.If(~interface.busy):
                    m.next = "COMMAND_PHASE"

        return m


class SPIRegisterInterface(Elaboratable):
    """ SPI device interface that allows for register reads and writes via SPI.
    The SPI transaction format matches:

        in:  WAAAAAAA[...] VVVVVVVV[...] DDDDDDDD[...]
        out: XXXXXXXX[...] XXXXXXXX[...] RRRRRRRR[...]

    Where:
        W = write bit; a '1' indicates that the provided value is a write request
        A = all bits of the address
        V = value to be written into the register, if W is set
        R = value to be read from the register

    Other I/O ports are added dynamically with add_register().
    """

    def __init__(self, port):
        """
        Parameters:
            address_size       -- the size of an address, in bits; recommended to be one bit
                                  less than a binary number, as the write command is formed by adding a one-bit
                                  write flag to the start of every address
            register_size      -- The size of any given register, in bits.
        """

        self.address_size  = 7
        self.register_size = 8

        #
        # Internal details.
        #

        # Instantiate an SPI command transciever submodule.
        self.interface = SPICommandInterface(port)

        # Create a new, empty dictionary mapping registers to their signals.
        self.registers = {}

        # Create signals for each of our register control signals.
        self._is_write = Signal()
        self._address  = Signal(self.address_size)


    def _ensure_register_is_unused(self, address):
        """ Checks to make sure a register address isn't in use before issuing it. """

        if address in self.registers:
            raise ValueError("can't add more than one register with address 0x{:x}!".format(address))


    def add_sfr(self, address, *, read=None, write_signal=None, write_strobe=None, read_strobe=None):
        """ Adds a special function register to the given command interface.

        Parameters:
            address       -- the register's address, as a big-endian integer
            read          -- a Signal or integer constant representing the
                             value to be read at the given address; if not provided, the default
                             value will be read
            read_strobe   -- a Signal that is asserted when a read is completed; if not provided,
                             the relevant strobe will be left unconnected
            write_signal  -- a Signal set to the value to be written when a write is requested;
                             if not provided, writes will be ignored
            write_strobe  -- a Signal that goes high when a value is available for a write request
         """

        assert address < (2 ** self.address_size)
        self._ensure_register_is_unused(address)

        # Add the register to our collection.
        self.registers[address] = {
            'read': read,
            'write_signal': write_signal,
            'write_strobe': write_strobe,
            'read_strobe': read_strobe,
            'elaborate': None,
        }


    def add_read_only_register(self, address, *, read, read_strobe=None):
        """ Adds a read-only register.

        Parameters:
            address       -- the register's address, as a big-endian integer
            read          -- a Signal or integer constant representing the
                             value to be read at the given address; if not provided, the default
                             value will be read
            read_strobe   -- a Signal that is asserted when a read is completed; if not provided,
                             the relevant strobe will be left unconnected
        """
        self.add_sfr(address, read=read, read_strobe=read_strobe)



    def add_register(self, address, *, value_signal=None, size=None, name=None, read_strobe=None,
        write_strobe=None, init=0):
        """ Adds a standard, memory-backed register.

            Parameters:
                address       -- the register's address, as a big-endian integer
                value_signal  -- the signal that will store the register's value; if omitted
                                 a storage register will be created automatically
                size          -- if value_signal isn't provided, this sets the size of the created register
                init          -- if value_signal isn't provided, this sets the reset value of the created register
                read_strobe   -- a Signal to be asserted when the register is read; ignored if not provided
                write_strobe  -- a Signal to be asserted when the register is written; ignored if not provided

            Returns:
                value_signal  -- a signal that stores the register's value; which may be the value_signal arg,
                                 or may be a signal created during execution
        """
        self._ensure_register_is_unused(address)

        # Generate a name for the register, if we don't already have one.
        name = name if name else "register_{:x}".format(address)

        # Generate a backing store for the register, if we don't already have one.
        if value_signal is None:
            size = self.register_size if (size is None) else size
            value_signal = Signal(size, name=name, init=init)

        # If we don't have a write strobe signal, create an internal one.
        if write_strobe is None:
            write_strobe = Signal(name=name + "_write_strobe")

        # Create our register-value-input and our write strobe.
        write_value  = Signal.like(value_signal, name=name + "_write_value")

        # Create a generator for a the fragments that will manage the register's memory.
        def _elaborate_memory_register(m):
            with m.If(write_strobe):
                m.d.sync += value_signal.eq(write_value)

        # Add the register to our collection.
        self.registers[address] = {
            'read': value_signal,
            'write_signal': write_value,
            'write_strobe': write_strobe,
            'read_strobe': read_strobe,
            'elaborate': _elaborate_memory_register,
        }

        return value_signal


    def _elaborate_register(self, m, register_address, connections):
        """ Generates the hardware connections that handle a given register. """

        #
        # Elaborate our register hardware.
        #

        # Create a signal that goes high iff the given register is selected.
        register_selected = Signal(name="register_address_matches_{:x}".format(register_address))
        m.d.comb += register_selected.eq(self._address == register_address)

        # Our write signal is always connected to word_received; but it's only meaningful
        # when write_strobe is asserted.
        if connections['write_signal'] is not None:
            m.d.comb += connections['write_signal'].eq(self.interface.word_received)

        # If we have a write strobe, assert it iff:
        #  - this register is selected
        #  - the relevant command is a write command
        #  - we've just finished receiving the command's argument
        if connections['write_strobe'] is not None:
            m.d.comb += [
                connections['write_strobe'].eq(self._is_write & self.interface.word_complete & register_selected)
            ]

        # Create essentially the same connection with the read strobe.
        if connections['read_strobe'] is not None:
            m.d.comb += [
                connections['read_strobe'].eq(~self._is_write & self.interface.word_complete & register_selected)
            ]

        # If we have any additional code that assists in elaborating this register, run it.
        if connections['elaborate']:
            connections['elaborate'](m)


    def elaborate(self, platform):
        m = Module()

        # Attach our SPI interface.
        m.submodules.interface = self.interface

        # Split the command into our "write" and "address" signals.
        m.d.comb += [
            self._is_write.eq(self.interface.command[-1]),
            self._address .eq(self.interface.command[:-1])
        ]

        # Create the control/write logic for each of our registers.
        for address, connections in self.registers.items():
            self._elaborate_register(m, address, connections)

        # Build the logic to select the 'to_send' value, which is selected
        # from all of our registers according to the selected register address.
        with m.Switch(self._address):
            for address, connections in self.registers.items():
                if connections['read'] is not None:
                    with m.Case(address):
                        # Hook up the word-to-send signal to the read value for the relevant
                        # register.
                        m.d.comb += self.interface.word_to_send.eq(connections['read'])

        return m

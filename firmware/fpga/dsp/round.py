#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

def convergent_round(value, discarded_bits):
    retained = value[discarded_bits:]
    discarded = value[:discarded_bits]
    msb_discarded = discarded[-1]
    rest_discarded = discarded[:-1]
    lsb_retained = retained[0]
    # Round up:
    # - If discarded > 0.5
    # - If discarded == 0.5 and retained is odd
    round_up = msb_discarded & (rest_discarded.any() | lsb_retained)
    return retained + round_up

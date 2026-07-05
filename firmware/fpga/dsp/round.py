#
# This file is part of HackRF.
#
# Copyright (c) 2025 Great Scott Gadgets <info@greatscottgadgets.com>
# SPDX-License-Identifier: BSD-3-Clause

def convergent_round(value, discarded_bits):
    shape = value.shape()
    if discarded_bits > shape.width - shape.signed:
        raise ValueError(f'cannot discard {discarded_bits} bits from a value with {shape.width - shape.signed} non-sign bits')
    retained = value[discarded_bits:]
    discarded = value[:discarded_bits]
    msb_discarded = discarded[-1]
    rest_discarded = discarded[:-1]
    lsb_retained = retained[0] if len(retained) else 0
    # Round up:
    # - If discarded > 0.5
    # - If discarded == 0.5 and retained is odd
    round_up = msb_discarded & (rest_discarded.any() | lsb_retained)
    if len(retained) == 0:
        # the returned result's width should always be 1 more than retained,
        # and Amaranth would produce 2 bits if we did the addition
        return round_up
    return retained + round_up

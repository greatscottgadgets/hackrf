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
    if shape.signed:
        retained = retained.as_signed()
    if len(retained) - shape.signed == 0:
        # the returned result's width should always be 1 more than retained,
        # but in this case Amaranth addition produces 2 bits more
        value = (retained + round_up)[:-1]
        return value.as_signed() if shape.signed else value
    return retained + round_up

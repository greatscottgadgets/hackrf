#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2012 Jared Boone
#
# This file is part of HackRF.
#
# This is a free hardware design; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#  
# This design is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#  
# You should have received a copy of the GNU General Public License
# along with this design; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

# This program is used to verify the logic on the Lollipop board, to
# make sure control of the various RF paths is correct.

class Component(object):
    def __init__(self, **kwargs):
        self.state = kwargs
        
    def __repr__(self):
        state_key = ''.join(
            map(str,
                map(int,
                    (self.state[input] for input in self.inputs)
                )
            )
        )
        if state_key not in self.states:
            return 'Invalid'
        else:
            return self.functions[self.states[state_key]]

class SKY13317(Component):
    inputs = (
        'V1',
        'V2',
        'V3',
    )
    
    states = {
        # V1, V2, V3
        '100': 'RFC to RF1',
        '010': 'RFC to RF2',
        '001': 'RFC to RF3',
    }

class SKY13351(Component):
    inputs = (
        'VCTL1',
        'VCTL2',
    )
    
    states = {
        # VCTL1, VCTL2
        '01': 'INPUT to OUTPUT1',
        '10': 'INPUT to OUTPUT2',
    }

class U2_4(SKY13351):
    name = 'U2/4'
    functions = {
        'INPUT to OUTPUT1': 'tx bandpass',
        'INPUT to OUTPUT2': 'tx mixer'
    }

class U6_9(SKY13351):
    name = 'U6/9'
    functions = {
        'INPUT to OUTPUT1': 'tx lowpass',
        'INPUT to OUTPUT2': 'tx highpass',
    }

class U3(SKY13317):
    name = 'U3'
    functions = {
        'RFC to RF1': 'tx highpass',
        'RFC to RF2': 'tx lowpass',
        'RFC to RF3': 'tx bandpass',
    }

class U7(SKY13351):
    name = 'U7'
    functions = {
        'INPUT to OUTPUT1': 'rx switch',
        'INPUT to OUTPUT2': 'tx path',
    }

class U10(SKY13351):
    name = 'U10'
    functions = {
        'INPUT to OUTPUT1': 'tx/rx switch',
        'INPUT to OUTPUT2': 'rx antenna',
    }

class U15(SKY13317):
    name = 'U15'
    functions = {
        'RFC to RF1': 'rx bandpass',
        'RFC to RF2': 'rx highpass',
        'RFC to RF3': 'rx lowpass',
    }

class U12_14(SKY13351):
    name = 'U12/14'
    functions = {
        'INPUT to OUTPUT1': 'rx lowpass',
        'INPUT to OUTPUT2': 'rx highpass',
    }

class U16_18(SKY13351):
    name = 'U16/18'
    functions = {
        'INPUT to OUTPUT1': 'rx mixer',
        'INPUT to OUTPUT2': 'rx bandpass',
    }

def compute_logic(**inputs):
    outputs = dict(inputs)
    outputs['swtxb2'] = not inputs['swtxb1']
    outputs['swrxb2'] = not inputs['swrxb1']
    outputs['swtxa2'] = not inputs['swtxa1']
    outputs['swrxa2'] = not inputs['swrxa1']
    outputs['swd2'] = not inputs['swd1']
    outputs['swrxv2'] = outputs['swrxb2'] and outputs['swrxa1']
    outputs['swrxv3'] = outputs['swrxb2'] and outputs['swrxa2']
    outputs['swtxv1'] = outputs['swtxa1'] and outputs['swtxb1']
    outputs['swtxv2'] = outputs['swtxa2'] and outputs['swtxb1']
    
    # Force boolean True/False (result of "not" operator) to 1 or 0.
    for key in outputs:
        outputs[key] = int(outputs[key])
    
    return outputs

def print_signals(signals):
    print(', '.join(('%s=%s' % (name, signals[name]) for name in sorted(signals))))

def print_circuit_state(signals):
    components = (
        U2_4(VCTL1=signals['swtxb1'], VCTL2=signals['swtxb2']),
        U6_9(VCTL1=signals['swtxa1'], VCTL2=signals['swtxa2']),
        U3(V1=signals['swtxv1'], V2=signals['swtxv2'], V3=signals['swtxb2']),
        U7(VCTL1=signals['swd2'], VCTL2=signals['swd1']),
        U10(VCTL1=signals['swd2'], VCTL2=signals['swd1']),
        U15(V1=signals['swrxb1'], V2=signals['swrxv2'], V3=signals['swrxv3']),
        U12_14(VCTL1=signals['swrxa1'], VCTL2=signals['swrxa2']),
        U16_18(VCTL1=signals['swrxb1'], VCTL2=signals['swrxb2'])
    )
    
    for component in components:
        print('%s: %s' % (component.name, component))

def make_bits_from_numbers(i, bit_count):
    return [int(c) for c in bin(i)[2:].zfill(bit_count)]

print('Transmit')
print('========')
print
for i in range(4):
    inputs = {
        'swtxb1': (i >> 1) & 1,
        'swtxa1': (i >> 0) & 1,
        'swrxa1': 0,
        'swrxb1': 0,
        'swd1': 0,
    }
    
    outputs = compute_logic(**inputs)
    print_signals(outputs)
    print_circuit_state(outputs)
    
    print

print('Receive')
print('========')
print
for i in range(4):
    inputs = {
        'swtxb1': 0,
        'swtxa1': 0,
        'swrxa1': (i >> 1) & 1,
        'swrxb1': (i >> 0) & 1,
        'swd1': 0,
    }
    
    outputs = compute_logic(**inputs)
    print_signals(outputs)
    print_circuit_state(outputs)
    
    print

#!/usr/bin/env python3

import argparse
import math
import random
import sys
from coojasim import Cooja, UDGMRadioMedium


def get_distance(mote, x, y):
    dx = abs(x - mote.position.x)
    dy = abs(y - mote.position.y)
    return math.sqrt(dx ** 2 + dy ** 2)


args = sys.argv[1:]
p = argparse.ArgumentParser()
p.add_argument('-i', dest='input', required=True)
p.add_argument('-o', dest='output', required=True)
p.add_argument('--topology', dest='topology', default=None)
p.add_argument('--min-distance', dest='min_distance', type=int, default=0)
try:
    conopts = p.parse_args(args)
except Exception as e:
    sys.exit(f"Illegal arguments: {str(e)}")

c = Cooja(conopts.input)
motes = []
radio_medium = c.sim.radio_medium
tx_range = radio_medium.transmitting_range if isinstance(radio_medium, UDGMRadioMedium) else 50.0
max_range = tx_range * len(c.sim.get_motes())
print(f"Using tx range {tx_range} meters with max multihop range of {max_range} meters.")
# Avoid warnings about uninitialized variables
x = y = sx = sy = 0

if conopts.min_distance > 0 and conopts.min_distance >= tx_range:
    print(f'Minimal distance between nodes is too large for communication range')
    sys.exit("Too large minimal distance")

promote_multihop = True
if conopts.topology == 'spread':
    promote_multihop = False

for m in c.sim.get_motes():
    if not motes:
        # Simply use first mote with its original position
        motes.append(m)
        sx = m.position.x
        sy = m.position.y
        print(f"first node {m.mote_id} has position {sx}, {sy}")
    else:
        done = False
        while not done:
            if promote_multihop:
                x = random.uniform(sx - max_range / 2 + 1, sx + max_range / 2 - 1)
                y = random.uniform(sy + 1, sy + max_range - 1)
            else:
                x = random.uniform(sx - max_range + 1, sx + max_range - 1)
                y = random.uniform(sy - max_range + 1, sy + max_range - 1)
            for p in motes:
                distance = get_distance(p, x, y)
                if distance < tx_range:
                    done = True
                    break
            if done and conopts.min_distance > 0:
                for p in motes:
                    distance = get_distance(p, x, y)
                    if distance < conopts.min_distance:
                        done = False
                        break

        m.position.x = x
        m.position.y = y
        motes.append(m)
        print(f"setting position for mote {m.mote_id:>3} to {m.position.x}, {m.position.y}")

c.save(conopts.output)

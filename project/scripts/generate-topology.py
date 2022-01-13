#!/usr/bin/env python3

import argparse
import math
import os
import random
import sys
from coojasim import Cooja, UDGMRadioMedium


def get_distance(mote, x, y):
    dx = abs(x - mote.position.x)
    dy = abs(y - mote.position.y)
    return math.sqrt(dx ** 2 + dy ** 2)


def main():
    args = sys.argv[1:]
    p = argparse.ArgumentParser()
    p.add_argument('-i', dest='input', required=True)
    p.add_argument('-o', dest='output', required=True)
    p.add_argument('-c', dest='count', type=int, default=1)
    p.add_argument('--generated-seed', dest='generated_seed', action='store_true', default=False)
    p.add_argument('--topology', dest='topology', default=None)
    p.add_argument('--min-distance', dest='min_distance', type=int, default=0)
    try:
        conopts = p.parse_args(args)
    except Exception as e:
        sys.exit(f"Illegal arguments: {str(e)}")

    c = Cooja(conopts.input)

    if conopts.generated_seed:
        # Tell Cooja to generate a new random seed each time simulation runs
        c.sim.random_seed.set_generated()

    radio_medium = c.sim.radio_medium
    tx_range = radio_medium.transmitting_range if isinstance(radio_medium, UDGMRadioMedium) else 50.0
    max_range = tx_range * len(c.sim.get_motes())
    print(f"Using tx range {tx_range} meters with max multihop range of {max_range} meters.")

    if conopts.min_distance > 0 and conopts.min_distance >= tx_range:
        print(f'Minimal distance between nodes is too large for communication range')
        sys.exit("Too large minimal distance")

    promote_multihop = True
    if conopts.topology == 'spread':
        promote_multihop = False

    output_file = conopts.output

    for i in range(0, conopts.count):
        x = y = sx = sy = 0
        motes = []

        if conopts.count > 1:
            output_file = f'{os.path.splitext(conopts.output)[0]}-{i + 1:05}.csc'
        if os.path.exists(output_file):
            print(f'Warning: the output file {output_file} already exists - skipping generation')
            continue
        print(f'Generating {output_file}')

        if not conopts.generated_seed:
            # Set a new random seed for each new simulation
            c.sim.random_seed.set_seed(random.randint(0, 0x7fffffff))

        for m in c.sim.get_motes():
            if not motes:
                # Simply use first mote with its original position
                motes.append(m)
                sx = m.position.x
                sy = m.position.y
                print(f"position for first mote {m.mote_id:>5} is {sx:20.15f},{sy:20.15f}")
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
                print(f"setting position for mote {m.mote_id:>3} to {m.position.x:20.15f},{m.position.y:20.15f}")

        c.save(output_file)


if __name__ == '__main__':
    main()

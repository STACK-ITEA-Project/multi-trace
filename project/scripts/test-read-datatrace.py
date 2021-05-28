#!/usr/bin/env python3

import re

import coojatrace
from humanfriendly.tables import format_pretty_table


def main():
    trace = coojatrace.main()

    # Show summary
    # trace.print_summary()

    network_events = trace.get_events(event_type='network', description='steady-state')
    network_stable_time = network_events[0].time if network_events else 0

    data = []
    p = re.compile(r'.*DATA: (.+)$')
    # Only look at mote output from after the network is stable
    # Note that first statistics counters should not be counted as they might
    # include data from before the network was stable.
    for o in trace.get_mote_output(start_time=network_stable_time):
        m = p.match(o.message)
        if m:
            data.append([o.time, o.mote_id] + [int(v) for _k, v in (g.split(':') for g in m.group(1).split(','))])

    # Print 10 first values
    column_names = ['Time', 'Mote', 'Seq', 'Rank', 'Version', 'DIS-R', 'DIS-S', 'DIO-R', 'DIO-S',
                    'DAO-R', 'RPL-total-sent']
    print(format_pretty_table(data[:10], column_names))


if __name__ == '__main__':
    main()

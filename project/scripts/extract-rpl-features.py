#!/usr/bin/env python3
import csv
import re
import sys

import coojatrace
from humanfriendly.tables import format_pretty_table


def write_csv(trace, csv_file, columns, data):
    if trace.is_file(csv_file):
        print(f'Warning: the CSV file "{csv_file}" already exists!', file=sys.stderr)
    else:
        with trace.get_log_writer(csv_file) as f:
            w = csv.writer(f, delimiter=';', quotechar='"')
            w.writerow(columns)
            for d in data:
                w.writerow(d)


def main():
    trace = coojatrace.main()

    network_events = trace.get_events(event_type='network', description='steady-state')
    network_stable_time = network_events[0].time if network_events else 0

    motes = {}
    data = []
    p = re.compile(r'.*DATA: (.+)$')
    # Only look at mote output from after the network is stable
    # Note that first statistics counters should not be counted as they might
    # include data from before the network was stable.
    for o in trace.get_mote_output(start_time=network_stable_time):
        m = p.match(o.message)
        if m:
            row = [o.time, o.mote_id] + [int(v) for _k, v in (g.split(':') for g in m.group(1).split(','))]
            last = motes[o.mote_id] if o.mote_id in motes else None
            motes[o.mote_id] = row
            # Use relative counters since last known statistics instead of aggregated
            if last:
                row = row[:5] + [x - y for x, y in zip(row[5:], last[5:])]
            else:
                row = row[:5] + [0] * 6
            data.append(row)

    # Print 20 first values
    column_names = ['Time', 'Mote', 'Seq', 'Rank', 'Version', 'DIS-R', 'DIS-S', 'DIO-R', 'DIO-S',
                    'DAO-R', 'RPL-total-sent']
    print(format_pretty_table(data[:20], column_names))
    if len(data) > 20:
        print(f"Only showing 20 first rows - remaining {len(data) - 20} rows not shown.")

    # Save statistics to CSV file
    write_csv(trace, 'rpl-statistics.csv', column_names, data)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3

import re

import coojatrace
from humanfriendly.tables import format_pretty_table


def main():
    trace = coojatrace.main()

    # Show summary
    # trace.print_summary()

    data = []
    p = re.compile(r'.*DATA: (.+)$')
    for o in trace.get_mote_output():
        m = p.match(o.message)
        if m:
            data.append([o.time, o.mote_id] + [int(v) for _k, v in (g.split(':') for g in m.group(1).split(','))])

    # Print 10 first values
    column_names = ['Time', 'Mote', 'Seq', 'Rank', 'Version', 'DIS-R', 'DIS-S', 'DIO-R', 'DIO-S',
                    'DAO-R', 'RPL-total-sent']
    print(format_pretty_table(data[:10], column_names))


if __name__ == '__main__':
    main()

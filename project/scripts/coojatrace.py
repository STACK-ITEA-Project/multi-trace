#!/usr/bin/env python3

import argparse
import re
import sys
from humanfriendly.tables import format_pretty_table


def _read_log(filename, callback, max_errors=0):
    line_number = 0
    errors = 0
    with open(filename, "r") as f:
        for line in f.readlines():
            line = line.strip()
            line_number += 1
            # Ignore comments for now
            if not line.startswith("#"):
                try:
                    callback(line)
                except ParseException as e:
                    e.line = line
                    e.line_number = line_number
                    e.filename = filename
                    if errors < max_errors:
                        print(e, file=sys.stderr)
                        errors += 1
                    else:
                        raise e


class ParseException(Exception):
    line = ''
    line_number = 0
    filename = ''

    def __init__(self, message):
        self.message = message
        super().__init__(message)

    def __str__(self):
        if self.filename:
            return f'{self.message} at {self.filename}:{self.line_number}: "{self.line}"'
        return str(self.message)


class Mote:
    mote_id = None
    address = None
    output = None
    transmissions = None
    discovered = None

    def __init__(self, mote_id, discovered):
        self.mote_id = mote_id
        self.output = []
        self.transmissions = []
        self.discovered = discovered


class Event:
    time = None
    event_type = None
    description = None

    def __init__(self, line):
        data = line.split('\t', 2)
        if len(data) < 3:
            raise ParseException("Failed to parse event data")
        self.time = int(data[0])
        self.event_type = data[1]
        self.description = data[2]


class MoteOutput:
    time = None
    mote_id = None
    message = None

    def __init__(self, line):
        data = line.split('\t', 2)
        if len(data) < 3:
            raise ParseException("Failed to parse mote output")
        self.time = int(data[0])
        self.mote_id = int(data[1])
        self.message = data[2]


class RadioTransmission:
    time_start = None
    time_end = None
    radio_channel = None
    mote_id = None
    receivers = None
    interfered = None
    interferedNoneDestinations = None
    data = None

    def __init__(self, line):
        data = line.split('\t', 7)
        if len(data) < 8:
            raise ParseException("Failed to parse radio transmission")
        self.time_start = int(data[0])
        self.time_end = int(data[1])
        self.radio_channel = int(data[2])
        self.mote_id = int(data[3])
        self.receivers = data[4]
        self.interfered = data[5]
        self.interferedNoneDestinations = data[6]
        self.data = data[7]


class CoojaTrace:
    motes = None
    events = None
    mote_output = None
    transmissions = None

    def __init__(self, trace_name):
        self.motes = {}
        self.mote_output = []
        self.events = []
        self.transmissions = []

        m = re.match(r'(.+-dt-\d+)(-.+[.].+)?$', trace_name)
        if not m:
            sys.exit(f"File name not matching Cooja data trace: '{trace_name}'")
        self.data_trace_name = m.group(1)
        # radio_log = self.data_trace_name + '-radio-log.pcap'

        _read_log(self.data_trace_name + '-event-output.log', self._process_events, max_errors=1)
        _read_log(self.data_trace_name + '-mote-output.log', self._process_mote_output, max_errors=1)
        _read_log(self.data_trace_name + '-radio-medium.log', self._process_radio_medium, max_errors=1)

        # Get address from 'Tentative link-local IPv6 address: fe80::222:22:22:22'
        p = re.compile(r'.*Tentative link-local IPv6 address: fe80(::[\d:a-f]+)$')
        for o in self.mote_output:
            m = p.match(o.message)
            if m:
                mote = self.add_mote(o.mote_id, o.time)
                mote.address = m.group(1)

    def _process_events(self, line):
        self.events.append(Event(line))

    def _process_mote_output(self, line):
        output = MoteOutput(line)
        self.add_mote(output.mote_id, output.time).output.append(output)
        self.mote_output.append(output)

    def _process_radio_medium(self, line):
        t = RadioTransmission(line)
        self.add_mote(t.mote_id, t.time_start).transmissions.append(t)
        self.transmissions.append(t)

    def get_mote_output(self, regex=None):
        if regex:
            p = re.compile(regex)
            return [v for v in self.mote_output if p.match(v.message)]
        return list(self.mote_output)

    def add_mote(self, mote_id, time):
        if mote_id in self.motes:
            return self.motes[mote_id]
        mote = Mote(mote_id, time)
        self.motes[mote_id] = mote
        return mote

    def get_mote(self, mote_id):
        return self.motes[mote_id] if mote_id in self.motes else None

    def print_summary(self):
        print(f"Data Traces from {self.data_trace_name}")
        print(f"  {len(self.motes)} motes, {len(self.events)} events, "
              f"{len(self.mote_output)} log outputs, {len(self.transmissions)} radio transmissions.")
        print()
        data = list(map(lambda e: [e.time, e.event_type, e.description], self.events))
        columns = ['Time', 'Event', 'Description']
        print(format_pretty_table(data, columns))
        data = list(map(lambda e: [e.mote_id, e.address, len(e.output), len(e.transmissions)], self.motes.values()))
        columns = ['Mote', 'Address', 'Log Outputs', 'Transmissions']
        print(format_pretty_table(data, columns))


def main(parser=None):
    if not parser:
        parser = argparse.ArgumentParser()
    parser.add_argument('input')
    try:
        conopts = parser.parse_args(sys.argv[1:])
    except Exception as e:
        sys.exit(f"Illegal arguments: {str(e)}")
    trace = None
    try:
        trace = CoojaTrace(conopts.input)
    except Exception as e:
        print(e)
        sys.exit(f"Failed to parse Cooja traces: {conopts.input}")
    return trace


if __name__ == '__main__':
    main()

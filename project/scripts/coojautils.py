import errno
import gzip
import os
import sys


def read_log(filename, callback, max_errors=0):
    line_number = 0
    errors = 0
    with LogReader(filename) as f:
        for line in f.readlines():
            line = line.strip()
            line_number += 1
            # Ignore comments for now
            if not line.startswith('#'):
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


class LogReader(object):
    def __init__(self, filename, mode='rt'):
        self.filename = filename
        self.mode = mode
        self.is_gzip = False

    def __enter__(self):
        if os.path.isfile(self.filename):
            self.is_gzip = self.filename.endswith('.gz')
        elif os.path.isfile(self.filename + '.gz'):
            self.is_gzip = True
            self.filename += '.gz'
        else:
            raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT), self.filename)
        self.fd = gzip.open(self.filename, self.mode) if self.is_gzip else open(self.filename, self.mode)
        return self.fd

    def __exit__(self, exc_type, exc_value, tb):
        self.fd.close()


class LogWriter(object):
    def __init__(self, filename, mode='wt'):
        self.filename = filename
        self.mode = mode
        self.is_gzip = filename.endswith('.gz')

    def __enter__(self):
        self.fd = gzip.open(self.filename, self.mode) if self.is_gzip else open(self.filename, self.mode)
        return self.fd

    def __exit__(self, exc_type, exc_value, tb):
        self.fd.close()


class ParseException(Exception):

    def __init__(self, message, filename=None, line_number=0, line=None):
        self.message = message
        self.filename = filename
        self.line_number = line_number
        self.line = line if line else ''
        super().__init__(message)

    def __str__(self):
        if self.filename:
            return f'{self.message} at {self.filename}:{self.line_number}: "{self.line}"'
        return str(self.message)

#!/usr/bin/env python3

import sys
import os
import time
from subprocess import Popen, PIPE, STDOUT

# get the path of this example
SELF_PATH = os.path.dirname(os.path.abspath(__file__))
# find path to Contiki-NG
CONTIKI_PATH = os.path.dirname(os.path.dirname(os.path.dirname(SELF_PATH)))

cooja_jar = os.path.normpath(os.path.join(CONTIKI_PATH, "tools", "cooja", "dist", "cooja.jar"))
cooja_output = 'COOJA.testlog'
cooja_log = 'COOJA.log'

#######################################################
# Run a child process and get its output

def run_subprocess(args, input_string):
    retcode = -1
    stdoutdata = ''
    try:
        proc = Popen(args, stdout = PIPE, stderr = STDOUT, stdin = PIPE, shell = True)
        (stdoutdata, stderrdata) = proc.communicate(input_string)
        if not stdoutdata:
            stdoutdata = ''
        if stderrdata:
            stdoutdata += stderrdata
        retcode = proc.returncode
    except OSError as e:
        sys.stderr.write("run_subprocess OSError:" + str(e))
    except CalledProcessError as e:
        sys.stderr.write("run_subprocess CalledProcessError:" + str(e))
        retcode = e.returncode
    except Exception as e:
        sys.stderr.write("run_subprocess exception:" + str(e))
    finally:
        return (retcode, stdoutdata)

#############################################################
# Run a single instance of Cooja on a given simulation script

def execute_test(cooja_file):
    # cleanup
    try:
        os.rm(cooja_output)
    except:
        pass

    target_basename = cooja_file
    if target_basename.endswith('.csc.gz'):
        target_basename = target_basename[:-7]
    elif target_basename.endswith('.csc'):
        target_basename = target_basename[:-4]
    simulation_id = str(round(time.time() * 1000))
    target_basename += '-dt-' + simulation_id
    target_output = target_basename + '-cooja.testlog'
    target_log_output = target_basename + '-cooja.log'

    # TODO simulation_id should be an argument to Cooja
    filename = os.path.join(SELF_PATH, cooja_file)
    args = " ".join(["java -jar ", cooja_jar, "-nogui=" + filename, "-contiki=" + CONTIKI_PATH])
    sys.stdout.write("  Running Cooja, args={}\n".format(args))

    start_time = time.perf_counter_ns()
    (retcode, output) = run_subprocess(args, '')
    end_time = time.perf_counter_ns()
    with open(cooja_log, 'a') as f:
        f.write(f'\nSimulation execution time: {end_time - start_time} ns.\n')
    if retcode != 0:
        sys.stderr.write(f"Failed, retcode={retcode}, output: {output}\n")
        return False

    sys.stdout.write("  Checking for output...")

    is_done = False
    os.rename(cooja_output, target_output)
    os.rename(cooja_log, target_log_output)
    with open(target_output, "r") as f:
        for line in f.readlines():
            line = line.strip()
            if line == "TEST OK":
                sys.stdout.write(" done.\n")
                is_done = True
                continue

    if not is_done:
        sys.stdout.write("  test failed.\n")
        return False

    sys.stdout.write(f" test done in {round((end_time - start_time) / 1000000)} milliseconds.\n")
    return True

#######################################################
# Run the application

def main():
    if not os.access(cooja_jar, os.R_OK):
        print(f'The file "{cooja_jar}" does not exist, did you build Cooja?')
        sys.exit(1)

    for input_file in sys.argv[1:]:
        if not os.access(input_file, os.R_OK):
            print(f'Simulation script "{input_file}" does not exist')
            sys.exit(1)

        print(f'Using simulation script "{input_file}"')
        if not execute_test(input_file):
            sys.exit(1)

    print('Done. No more simulation files specified.')

#######################################################

if __name__ == '__main__':
    main()

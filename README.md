# Multi-Trace

The Multi-Trace Generation tool is a collection of scripts and
additions to the Cooja Network Simulator to get more detailed tracing
of IoT networks.

## Getting started

You need a toolchain for Contiki-NG to compile code:
https://github.com/contiki-ng/contiki-ng/wiki/Toolchain-installation-on-Linux

You also need Java 11 and Python 3.

First checkout all submodules if you have not already done this.

```
git submodule update --init --recursive
```

Quickstart of an example simulation with GUI.

```
./run-cooja-gui.sh applications/rpl-udp-ids/rpl-udp-ids-cooja.csc
```

Quickstart of an example simulation without GUI. The simulation must
include a configured timeout configured to run the simulation without GUI.

```
./run-cooja-nogui.sh applications/rpl-udp-ids/simulations/rpl-udp-base-15-attack-blackhole-random.csc
```

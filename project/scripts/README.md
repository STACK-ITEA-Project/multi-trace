## Scripts to parse and generate Cooja simulation files

Example generating a new simulation with randomized topology that is spread around
a sink node:
```
./test-randomized-pos.py -i ../rpl-udp-ids/rpl-udp-cooja-topology.csc -o ../rpl-udp-ids/test.csc --topology spread
```

Example generating a new simulation with randomized topology that is
somewhat biased for more multihop:
```
./test-randomized-pos.py -i ../rpl-udp-ids/rpl-udp-cooja-topology.csc -o ../rpl-udp-ids/test.csc
```

Example generating a new simulation with randomized topology with extra
constraint that nodes should be at least 40 meters from each other.
```
./test-randomized-pos.py -i ../rpl-udp-ids/rpl-udp-cooja-topology.csc -o ../rpl-udp-ids/test.csc --min-distance 40
```

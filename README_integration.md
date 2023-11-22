# Integrating flooding attack detection implementation on Contiki-NG/Cooja environments

The flooding attack simulation scipt and each node's behavior is implemented based on the [multi-trace repository on the STACK github](https://github.com/STACK-ITEA-Project/multi-trace).

## High level overview of our framework
Our framework consits of four parts:
1. Communication of RPL-statistics within client and server.
    - Clients send RPL-statistics to the server using UDP. (Tested with mtype385)
    - Server node aggregates statistics received from clients. (Tested with mtype463)
2. Simple bucketing algorithm
    - To address the possibility where the statistics data are dropped due to bad network status, server performs a simple bucketing algorithm.
3. DIS-repeating attack
    - As a flooding attack method, we implemented the DIS-repeating algorithm that is executable through cmd in Cooja. The flooding attack method will be repeatedly turned on and off in a static period of time.
4. MLP-based attack detection model
    - Based on the bucketing algorithm, server runs the MLP-based attack detection model once it decides that the data is ready to be fed to the model.
    - The weight of the MLP model is hard-coded in the code.

## Implementation details and integration guidelines
1. All our changes and implementations are bounded under `/applications/flooding-attack-demo` and `/services/flooding-attack`.
2. The communications of RPL-statistics and bucketing algorithms are implemented on `/applications/flooding-attack-demo/udp-client.c` and `/applications/flooding-attack-demo/udp-server.c`. All the parts that we added is bracketed by `/* Implementation by Yonsei */` and `/* __Implementation by Yonsei */` to make the integration easy.
3. The DIS-repeating attack is implemented accross `/applications/flooding-attack-demo/udp-client.c`, `/services/flooding-attack/` and `/applications/flooding-attack-demo/simulations/test-disrepeat-base-20.csc` This should be integrated also to let the attack detection algorithm work well.
4. The MLP-based attack detection model is implemented on `/applications/flooding-attack-demo/model-mlp.[c, h]`. To integrate these, one only need to add the codes in the same folder with `udp-server.c` and add `PROJECT_SOURCEFILES += mlp.c` to `Makefiles`.

## Selecting the correct model weights for each topology
Our detection model currently does not support either flexible length input nor static length input for different number of motes. So if the number of motes changes, one should load different model weights. To do so, two following parts must be changed.
1. Number of clients (except server): FDATK_NCLI in `udp-server.c`
2. Model weights header file: `#include "model_weights/[].h"`
    - For the simulation file `test-disrepeat-base-20`, the weight header file is `rise-19.h`
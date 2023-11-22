# Integrating flooding attack detection implementation on Contiki-NG/Cooja environments

The flooding attack simulation scipt and each node's behavior is implemented based on the [multi-trace repository on the STACK github](https://github.com/STACK-ITEA-Project/multi-trace).

## High level overview of our framework
Our framework consits of four parts:
1. Communication of RPL-statistics within client and server.
    - Clients send RPL-statistics to the server using UDP instead of logging (initial way).
    - Server aggregates each statistics received from clients. 
2. Simple bucketing algorithm
    - To address the possibility where the statistics data are dropped due to bad network status, server performs a simple bucketing algorithm.
3. DIS-repeating attack
    - As a flooding attack method, we implemented the DIS-repeating algorithm that is executable through cmd in Cooja.
4. MLP-based attack detection model
    - Relying on the bucketing algorithm, server runs the MLP-based attack detection model once it decides that the data is ready to be fed to the model.
    - The weight of the MLP model is hard-coded in the code.

## Implementation details and integration guidelines
1. All our changes and implementations are bounded under `/applications/flooding-attack-demo` and `/services/flooding-attack`.
2. The communications of RPL-statistics and bucketing algorithms are implemented on `/applications/flooding-attack-demo/udp-client.c` and `/applications/flooding-attack-demo/udp-server.c`. All the parts that we added is bracketed by `/* Implementation by Yonsei */` and `/* __Implementation by Yonsei */` to make the integration easy.
3. The DIS-repeating attack is implemented accross `/applications/flooding-attack-demo/udp-client.c`, `/services/flooding-attack/` and `/applications/flooding-attack-demo/simulations/test-disrepeat-base.csc` This should be integrated also to let the attack detection algorithm work well.
4. The MLP-based attack detection model is implemented on `/applications/flooding-attack-demo/flooding-attack-detection-model-mlp.[c, h]`. To integrate these, one only need to add the codes in the same folder with `udp-server.c` and add `PROJECT_SOURCEFILES += flooding-attack-detection-model-mlp.c` to `Makefiles`.
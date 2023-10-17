CONTIKI_PROJECT = udp-client udp-server 
CONTIKI=../../tools/contiki-ng

all: $(CONTIKI_PROJECT)


PROJECT_SOURCEFILES += flooding-attack-detection-model-mlp.c
MODULES_REL += ../../services/flooding-attack
MODULES += $(CONTIKI_NG_SERVICES_DIR)/shell

include $(CONTIKI)/Makefile.include

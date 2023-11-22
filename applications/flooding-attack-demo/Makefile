CONTIKI_PROJECT = udp-server udp-client
CONTIKI=../../tools/contiki-ng

all: $(CONTIKI_PROJECT)


PROJECT_SOURCEFILES += model.c
MODULES_REL += ../../services/flooding-attack
MODULES += $(CONTIKI_NG_SERVICES_DIR)/shell

include $(CONTIKI)/Makefile.include

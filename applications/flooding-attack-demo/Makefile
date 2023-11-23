CONTIKI_PROJECT = udp-server udp-client
CONTIKI=../../tools/contiki-ng

all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += model.c
MODULES_REL += ../../services/flooding-attack
MODULES += $(CONTIKI_NG_SERVICES_DIR)/shell

IN_DEVICE_DETECTION ?= 1

ifeq ($(IN_DEVICE_DETECTION),1)
  CFLAGS += -DIN_DEVICE_DETECTION=1
  MODULES_REL += in-device-detection ../../services/network-attacks
endif

include $(CONTIKI)/Makefile.include

CONTIKI_PROJECT = sender

MODULES += os/services/shell

all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += msg_send.c

MODULES += dev/sensor/collect

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

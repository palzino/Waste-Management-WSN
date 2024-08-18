CONTIKI_PROJECT = bin control
all: $(CONTIKI_PROJECT)

CONTIKI=../..

TARGET_LIBFILES = -lm


include $(CONTIKI)/Makefile.include

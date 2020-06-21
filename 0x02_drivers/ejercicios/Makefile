obj-m := nulo.o azar.o letras123.o
KVERSION := $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build SUBDIRS=$(shell pwd) modules

clean:
	make -C /lib/modules/$(KVERSION)/build SUBDIRS=$(shell pwd) clean

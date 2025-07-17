ARCH	?= x86_64
SYSROOT	?= /opt/$(ARCH)-hlos
CC      ?= $(SYSROOT)/bin/$(ARCH)-hlos-gcc

export CC

all: test

test:
	@make -C test

.PHONY: all test
